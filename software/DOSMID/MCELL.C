#ifdef MCELL

//#define MCELL_DEBUG 1

#ifdef MCELL_DEBUG
#include <stdio.h>
#include <stdarg.h>
#endif

#include "MCELL.H"
#include <math.h>

#ifdef MCELL_DEBUG
void debug_log(const char *fmt, ...)
{
   FILE *f_log;
   va_list ap;

        f_log = fopen("mcellog.log", "a");
        va_start(ap, fmt);
        vfprintf(f_log, fmt, ap);
        va_end(ap);
        fclose(f_log);
}
#endif

const unsigned char octavetable[128] = {                         /* note # */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                          /*  0 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                          /* 12 */
        0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,                          /* 24 */
        1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,                          /* 36 */
        2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,                          /* 48 */
        3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,                          /* 60 */
        4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5,                          /* 72 */
        5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6,                          /* 84 */
        6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,                          /* 96 */
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,                         /* 108 */
        7, 7, 7, 7, 7, 7, 7, 7};                                    /* 120 */

mid_channel mcell_synth[MAX_MCELL_CHANNELS];    // MCELL synth

unsigned char DrumCh[2]; // Drum channels 0..1
unsigned char DrumIdx;

// MCELL IO port address
unsigned short mcellPort;

// Logic channel - first chip/second chip
static unsigned char ChanReg[6] =  {000,001,002,004,005,006};

unsigned short channelpitch[16];

// Note priority
unsigned char NotePriority;

void mcellReset(unsigned short port)
{
   unsigned char i;
   mcellPort = port;
   _asm
    {
        mov  dx,mcellPort
        add  dx,0fh
        mov  al,80h
        out  dx,al      // disable GATE (turn off all channels)

        mov  cx,7       // clear 3nE-3n8 mixer control
        xor  al,al
clr_mixer:
        dec  dx
        out  dx,al
        loop clr_mixer

        mov  dx,mcellPort
        add  dx,3       // 1st vi53 3n3 mode reg
        mov  al,03eh
        out  dx,al
        mov  al,07eh
        out  dx,al
        mov  al,0beh
        out  dx,al

        add  dx,4       // 2nd vi53 3n3 mode reg
        mov  al,03eh
        out  dx,al
        mov  al,07eh
        out  dx,al
        mov  al,0beh
        out  dx,al

        add  dx,8       // 3nF enable GATE (turn on)
        xor  al,al
        out  dx,al
    }
        for (i=0;i<16;i++)
            channelpitch[i] = 8192;

        for (i=0;i<MAX_MCELL_CHANNELS;i++)
        {
                mcell_synth[i].note=0;
                mcell_synth[i].priority=0;
        }
        NotePriority = 0;
        DrumCh[0] = 0;
        DrumCh[1] = 0;
}

void mcellDisableVoice(unsigned char voice)
{
_asm
   {
                xor   bh,bh
                mov   bl,voice
                mov   dx,mcellPort      // FIXME !!!

                add   dx,8      // 3n8 + voice
                add   dx,bx
                mov   al,00h
                out   dx,al
    }
}


void mcellSound(unsigned char voice,unsigned short freq, unsigned char mixer)
{
_asm
   {
                xor   bh,bh
                xor   ch,ch
                mov   bl,voice

                mov   dx,mcellPort      //
                mov   cl,ChanReg[bx]    ; bx = true channel (0 - 5)

                add   dx,cx
                mov   ax,freq
                out   dx,al
                mov   al,ah
                out   dx,al

                mov   dx,mcellPort
                add   dx,8
                add   dx,bx
                mov   al,mixer
                out   dx,al
    }
}

void mcellDrum(unsigned char DChan,unsigned char freq)
{
_asm
   {
                mov   dx,mcellPort
                xor   ah,ah
                mov   al,DChan
                add   dx,0eh
                add   dx,ax
                mov   al,freq
                or    al,10h
                out   dx,al
   }
}

void mcellDrumOff(unsigned char note)
{
        DrumIdx = 0x0e;

       if (DrumCh[0] == note)
        {
         DrumIdx = 0x0e;
         DrumCh[0] = 0;
        }
        else
        if (DrumCh[1] == note)
        {
          DrumIdx = 0x0f;
          DrumCh[1] = 0;
        }

_asm
   {
                mov   dx,mcellPort
                xor   ah,ah
                mov   al,DrumIdx
                add   dx,ax
                //add dx,0eh
                xor   al,al
                out   dx,al
   }
}
// ****
// High-level CMS synth procedures
// ****

void mcell_pitchwheel(unsigned short oplport, int channel, int pitchwheel) {
  channelpitch[channel] = pitchwheel;
}

void mcellNoteOff(unsigned char channel, unsigned char note)
{
  unsigned char i;
  unsigned char voice;

  if (channel == 9)
  {
#ifdef MCELL_DEBUG
       debug_log("DRUM OFF note= %u\n",note);
#endif
        mcellDrumOff(note);
  }
  else
  {
    voice = MAX_MCELL_CHANNELS;
    for(i=0; i<MAX_MCELL_CHANNELS; i++)
    {
        if(mcell_synth[i].note==note)
        {
                voice = i;
                break;
        }
    }


    // Note not found, ignore note off command
    if(voice==MAX_MCELL_CHANNELS)
    {
#ifdef MCELL_DEBUG
        debug_log("not found Ch=%u,note=%u\n",channel & 0xFF,note & 0xFF);
#endif
        return;
    }

    // decrease priority for all notes greater than current
    for (i=0; i<MAX_MCELL_CHANNELS; i++)
        if (mcell_synth[i].priority > mcell_synth[voice].priority )
                mcell_synth[i].priority = mcell_synth[i].priority - 1;


    if (NotePriority != 0) NotePriority--;

    mcellDisableVoice(voice);
    mcell_synth[voice].note = 0;
    mcell_synth[voice].priority = 0;
  }
}

float noteToFreq(unsigned char n, unsigned short pitch) {
    float a = 440; // frequency of A (coomon value is 440Hz)
//    return (a / 32) * pow(2, (((float)n - 9) / 12));
      return a * pow(2,(((float)n-69)/12)+((int)(pitch-8192)/4096*12));
}

char map(char x, char in_min, char in_max, char out_min, char out_max)
{
  return ((x - in_min) * (out_max - out_min)) / ((in_max - in_min) + out_min);
}

void mcellNoteOn(unsigned char channel, unsigned char note, unsigned char velocity)
{
//  unsigned char octave;
//  unsigned char noteVal;
  unsigned char i;
  unsigned char voice;
//  unsigned char note_cms;
  unsigned short notefreq;
  unsigned short period;
  unsigned short mixer;

  if (channel == 9)
  {
    if (velocity != 0)
       {
#ifdef MCELL_DEBUG
       debug_log("DRUM ON note= %u\n",note);
#endif
         mixer = 0;
         DrumIdx = 0;
         switch (note) {
                case 35:        // Acoustic Bass Drum
                     mixer = 8;
                     DrumIdx = 0;
                     break;
                case 36:        // Bass Drum 1
                     mixer = 0;
                     DrumIdx = 0;
                     break;
                case 37:        // Side Stick
                     mixer = 1;
                     DrumIdx = 0;
                     break;
                case 38:                // Acoustic Snare
                             mixer = 0;
                             DrumIdx = 1;
                             break;
                case 39:        // Hand Clap
                     mixer = 2;
                     DrumIdx = 0;
                     break;
                case 40:        // Electric Snare
                     mixer = 3;
                     DrumIdx = 0;
                     break;
                case 41:        // Low Floor Tom
                     mixer = 4;
                     DrumIdx = 0;
                     break;
                case 42:        // Closed Hi Hat
                     mixer = 5;
                     DrumIdx = 0;
                     break;
                case 43:        // High Floor Tom
                     mixer = 6;
                     DrumIdx = 0;
                     break;
                case 44:        //
                     mixer = 7;
                     DrumIdx = 0;
                     break;
         }
         mcellDrum(DrumIdx,mixer);
         DrumCh[DrumIdx] = note;
       }
    else
       mcellDrumOff(note);
  }
  else
  if (velocity != 0)
  {
//      note_cms = note+1;

//        octave = (note_cms / 12) - 1; //Some fancy math to get the correct octave
//        noteVal = note_cms - ((octave + 1) * 12); //More fancy math to get the correct note

//        mixer = 0x28 | 7-(map(octave,1,12,1,7));
        mixer = 0x28 | (((7-octavetable[note])+1) & 7);
#ifdef MCELL_DEBUG
//       debug_log("note %u : octave %u : map0-7 %u\n",note,octave,map(octave, 0, 12, 0, 7));
#endif

        notefreq = (unsigned short)noteToFreq(note,channelpitch[channel]);
        period = 1021429 / notefreq;

#ifdef MCELL_DEBUG
//       debug_log("midi note %u : freq %u : period %u\n",note,notefreq,period);
#endif

        NotePriority++;

        voice = MAX_MCELL_CHANNELS;
        for(i=0; i<MAX_MCELL_CHANNELS; i++)
        {
                if(mcell_synth[i].note==0)
                {
                        voice = i;
                        break;
                }
        }


        // We run out of voices, find low priority voice
        if(voice==MAX_MCELL_CHANNELS)
        {
                unsigned char min_prior = mcell_synth[0].priority;

#ifdef MCELL_DEBUG
        debug_log("out of voice. NotePriority=%u\n",NotePriority);
                for (i=0; i<MAX_MCELL_CHANNELS; i++)
                  debug_log("%u ",mcell_synth[i].priority);
        debug_log("\n");
#endif
                // find note with min prioryty
                voice = 0;
                for (i=1; i<MAX_MCELL_CHANNELS; i++)
                  if (mcell_synth[i].priority < min_prior)
                  {
                        voice = i;
                        min_prior = mcell_synth[i].priority;
                  }

                // decrease all notes priority by one
                for (i=0; i<MAX_MCELL_CHANNELS; i++)
                  if (mcell_synth[i].priority != 0)
                        mcell_synth[i].priority = mcell_synth[i].priority - 1;

                // decrease current priority
                if (NotePriority != 0) NotePriority--;

#ifdef MCELL_DEBUG
        debug_log("find low priority voice %u, NotePriority=%u\n",voice,NotePriority);
                for (i=0; i<MAX_MCELL_CHANNELS; i++)
                  debug_log("%u ",mcell_synth[i].priority);
        debug_log("\n");
#endif
        }

        mcellSound(voice,period,mixer);

        mcell_synth[voice].note = note;
        mcell_synth[voice].priority = NotePriority;

  }
  else
        mcellNoteOff(channel,note);
}

void mcellController(unsigned char channel, unsigned char id, unsigned char val)
{
  unsigned char i;
    if ((id==121) || (id==123)) // All Sound/Notes Off
    {
                for (i=0;i<MAX_MCELL_CHANNELS;i++)
                  if (mcell_synth[i].note != 0)
                        {
                                mcellDisableVoice(i);
                                mcell_synth[i].note = 0;
                                mcell_synth[i].priority = 0;
                        }
    }
}

#endif
