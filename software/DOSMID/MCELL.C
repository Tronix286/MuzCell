#ifdef MCELL

//#define MCELL_DEBUG 1

#ifdef MCELL_DEBUG
#include <stdio.h>
#include <stdarg.h>
#endif

#include "MCELL.H"
//#include <math.h>

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

static unsigned short freqtable[128] = {
    8,    9,    9,   10,   10,   11,   12,   12,   13,   14,   15,   15,
   16,   17,   18,   19,   21,   22,   23,   24,   26,   28,   29,   31,
   33,   35,   37,   39,   41,   44,   46,   49,   52,   55,   58,   62,
   65,   69,   73,   78,   82,   87,   92,   98,  104,  110,  117,  123,
  131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247,
  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,
  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
 8372, 8870, 9397, 9956,10548,11175,11840,12544};

static unsigned short pitchtable[256] = {                    /* pitch wheel */
         29193U,29219U,29246U,29272U,29299U,29325U,29351U,29378U,  /* -128 */
         29405U,29431U,29458U,29484U,29511U,29538U,29564U,29591U,  /* -120 */
         29618U,29644U,29671U,29698U,29725U,29752U,29778U,29805U,  /* -112 */
         29832U,29859U,29886U,29913U,29940U,29967U,29994U,30021U,  /* -104 */
         30048U,30076U,30103U,30130U,30157U,30184U,30212U,30239U,  /*  -96 */
         30266U,30293U,30321U,30348U,30376U,30403U,30430U,30458U,  /*  -88 */
         30485U,30513U,30541U,30568U,30596U,30623U,30651U,30679U,  /*  -80 */
         30706U,30734U,30762U,30790U,30817U,30845U,30873U,30901U,  /*  -72 */
         30929U,30957U,30985U,31013U,31041U,31069U,31097U,31125U,  /*  -64 */
         31153U,31181U,31209U,31237U,31266U,31294U,31322U,31350U,  /*  -56 */
         31379U,31407U,31435U,31464U,31492U,31521U,31549U,31578U,  /*  -48 */
         31606U,31635U,31663U,31692U,31720U,31749U,31778U,31806U,  /*  -40 */
         31835U,31864U,31893U,31921U,31950U,31979U,32008U,32037U,  /*  -32 */
         32066U,32095U,32124U,32153U,32182U,32211U,32240U,32269U,  /*  -24 */
         32298U,32327U,32357U,32386U,32415U,32444U,32474U,32503U,  /*  -16 */
         32532U,32562U,32591U,32620U,32650U,32679U,32709U,32738U,  /*   -8 */
         32768U,32798U,32827U,32857U,32887U,32916U,32946U,32976U,  /*    0 */
         33005U,33035U,33065U,33095U,33125U,33155U,33185U,33215U,  /*    8 */
         33245U,33275U,33305U,33335U,33365U,33395U,33425U,33455U,  /*   16 */
         33486U,33516U,33546U,33576U,33607U,33637U,33667U,33698U,  /*   24 */
         33728U,33759U,33789U,33820U,33850U,33881U,33911U,33942U,  /*   32 */
         33973U,34003U,34034U,34065U,34095U,34126U,34157U,34188U,  /*   40 */
         34219U,34250U,34281U,34312U,34343U,34374U,34405U,34436U,  /*   48 */
         34467U,34498U,34529U,34560U,34591U,34623U,34654U,34685U,  /*   56 */
         34716U,34748U,34779U,34811U,34842U,34874U,34905U,34937U,  /*   64 */
         34968U,35000U,35031U,35063U,35095U,35126U,35158U,35190U,  /*   72 */
         35221U,35253U,35285U,35317U,35349U,35381U,35413U,35445U,  /*   80 */
         35477U,35509U,35541U,35573U,35605U,35637U,35669U,35702U,  /*   88 */
         35734U,35766U,35798U,35831U,35863U,35895U,35928U,35960U,  /*   96 */
         35993U,36025U,36058U,36090U,36123U,36155U,36188U,36221U,  /*  104 */
         36254U,36286U,36319U,36352U,36385U,36417U,36450U,36483U,  /*  112 */
         36516U,36549U,36582U,36615U,36648U,36681U,36715U,36748U}; /*  120 */

mid_channel mcell_synth[MAX_MCELL_CHANNELS];    // MCELL synth

unsigned char DrumCh[2]; // Drum channels 0..1
unsigned char DrumIdx;

// MCELL IO port address
unsigned short mcellPort;

// Logic channel - first chip/second chip
static unsigned char ChanReg[6] =  {000,001,002,004,005,006};

unsigned short channelpitch[16];
unsigned char channelinstr[16];

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
        {
            channelinstr[i] = 0;
            channelpitch[i] = 8192;
        }

        for (i=0;i<MAX_MCELL_CHANNELS;i++)
        {
                mcell_synth[i].note=0;
                mcell_synth[i].priority=0;
                mcell_synth[i].ch = 0;
                mcell_synth[i].voice = 0;
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

void mcellAllDrumOff(void)
{
_asm
   {
                mov   dx,mcellPort
                add   dx,0eh
                xor   al,al
                out   dx,al

                inc dx
                out   dx,al
   }
}
// ****
// High-level CMS synth procedures
// ****

/*
float noteToFreq(unsigned char n, unsigned short pitch) {
    float a = 440; // frequency of A (coomon value is 440Hz)
//    return (a / 32) * pow(2, (((float)n - 9) / 12));
      return a * pow(2,(((float)n-69)/12)+((int)(pitch-8192)/4096*12));
}
*/

/* assign a new instrument to emulated MIDI channel */
void mcellProgChange(int channel, int program) {

    if (channel == 9) return; /* do not allow to change channel 9, it is for percussions only */

    channelinstr[channel] = program;
}

void mcell_pitchwheel(unsigned short oplport, int channel, int pitchwheel)
{
  unsigned char i;
  unsigned short notefreq;
  unsigned short period;
  unsigned char mixer;
  unsigned char note;
  int pitch;

    channelpitch[channel] = pitchwheel;
    for(i=0; i<MAX_MCELL_CHANNELS; i++)
    {
        if ((mcell_synth[i].ch==channel) && (mcell_synth[i].note != 0))
        {
          note = mcell_synth[i].note;
          mixer = 0x28 | (((7-octavetable[note])+1) & 7);

          //notefreq = (unsigned short)noteToFreq(note,pitchwheel);
          //if (notefreq < 16) notefreq = 16;
          //period = 1021429 / notefreq;

         pitch = pitchwheel;
         if (pitch != 0) {
           if (pitch > 127) {
              pitch = 127;
           } else if (pitch < -128) {
              pitch = -128;
           }
         }

         notefreq = ((unsigned long)freqtable[note] * pitchtable[pitch + 128]) >> 15;

         if (notefreq < 16) notefreq = 16;
         period = 1021429 / notefreq;
         mcellSound(mcell_synth[i].voice,period,mixer);
        }
    }
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
    mcell_synth[voice].ch = 0;
    mcell_synth[voice].voice = 0;
  }
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
  int pitch;

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
                     mixer = 4;
                     DrumIdx = 0;
                     break;
                case 36:        // Bass Drum 1
                     mixer = 6;
                     DrumIdx = 0;
                     break;
                case 37:        // Side Stick
                     mixer = 1;
                     DrumIdx = 0;
                     break;
                case 38:                // Acoustic Snare
                             mixer = 1;
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
                     mixer = 0;
                     DrumIdx = 0;
                     break;
                case 42:        // Closed Hi Hat
                     mixer = 1;
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
//        if ((channelinstr[channel] !=0) & (channelinstr[channel] < 7))
//            mixer = 0x18;
//        else
            mixer = 0x28;
        mixer |= ((7-octavetable[note])+1) & 7;

#ifdef MCELL_DEBUG
//       debug_log("note %u : octave %u : map0-7 %u\n",note,octave,map(octave, 0, 12, 0, 7));
#endif

        pitch = channelpitch[channel];
        if (pitch != 0) {
           if (pitch > 127) {
              pitch = 127;
           } else if (pitch < -128) {
              pitch = -128;
           }
        }

        notefreq = ((unsigned long)freqtable[note] * pitchtable[pitch + 128]) >> 15;

//        period = 1021429 / notefreq;
        //notefreq = (unsigned short)noteToFreq(note,channelpitch[channel]);
        if (notefreq < 16) notefreq = 16;
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
        mcell_synth[voice].ch = channel;
        mcell_synth[voice].voice = voice;

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
                                mcell_synth[i].ch = 0;
                                mcell_synth[i].voice = 0;
                        }
                mcellAllDrumOff();
                DrumCh[0] = 0;
                DrumCh[1] = 0;
    }
}

#endif
