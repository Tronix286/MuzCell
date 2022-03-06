/*
 *  Copyright (C) 2002-2013  The DOSBox Team
 *  Copyright (C) 2013-2014  bjt, elianda
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * ------------------------------------------
 * SoftMPU by bjt - Software MPU-401 Emulator
 * ------------------------------------------
 *
 * Based on original midi.c from DOSBox
 *
 */

/* SOFTMPU: Moved exported functions & types to header */
#include "export.h"
//#include <math.h>

/* SOFTMPU: Don't need these includes */
/*#include <assert.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <algorithm>

#include "SDL.h"

#include "dosbox.h"
#include "cross.h"
#include "support.h"
#include "setup.h"
#include "mapper.h"
#include "pic.h"
#include "hardware.h"
#include "timer.h"*/

/* SOFTMPU: Additional defines, typedefs etc. for C */
typedef unsigned long Bit32u;
typedef int Bits;

#define SYSEX_SIZE 1024
#define RAWBUF  1024

/* SOFTMPU: Note tracking for RA-50 */
#define MAX_TRACKED_CHANNELS 16
#define MAX_TRACKED_NOTES 8

#define MAX_MCELL_CHANNELS 6

#define DEFAULT_VOLUME 127

static char* MIDI_welcome_msg = "\xf0\x41\x10\x16\x12\x20\x00\x00    SoftMPU v1.9    \x24\xf7"; /* SOFTMPU */

static Bit8u MIDI_note_off[3] = { 0x80,0x00,0x00 }; /* SOFTMPU */

static Bit8u MIDI_evt_len[256] = {
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x00
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x10
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x20
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x30
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x40
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x50
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x60
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // 0x70

  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  // 0x80
  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  // 0x90
  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  // 0xa0
  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  // 0xb0

  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  // 0xc0
  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  // 0xd0

  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  // 0xe0

  0,2,3,2, 0,0,1,0, 1,0,1,1, 1,0,1,0   // 0xf0
};


// Volume
static Bit8u atten[128] = {
                 0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
                 3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,
                 5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,
                 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,
                 9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,
                 11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,
                 13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,
                 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        };

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

Bit8u freqtable[128] = {
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

unsigned short periodtable[128] = {
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
62467,58961,55651,52528,49580,46797,44171,41691,39352,37143,35058,33091,
31233,29480,27826,26264,24790,23399,22085,20846,19676,18571,17529,16545,
15617,14740,13913,13132,12395,11699,11043,10423, 9838, 9286, 8765, 8273,
 7808, 7370, 6956, 6566, 6197, 5850, 5521, 5211, 4919, 4643, 4382, 4136,
 3904, 3685, 3478, 3283, 3099, 2925, 2761, 2606, 2459, 2321, 2191, 2068,
 1952, 1843, 1739, 1641, 1549, 1462, 1380, 1303, 1230, 1161, 1096, 1034,
  976,  921,  870,  821,  775,  731,  690,  651,  615,  580,  548,  517,
  488,  461,  435,  410,  387,  366,  345,  326,  307,  290,  274,  259,
  244,  230,  217,  205,  194,  183,  173,  163,  154,  145,  137,  129,
  122,  115,  109,  103,   97,   91,   86,   81};

// Logic channel - first chip/second chip
static unsigned char ChanReg[6] =  {000,001,002,004,005,006};


/* SOFTMPU: Note tracking for RA-50 */
typedef struct {
        Bit8u used;
        Bit8u next;
        Bit8u notes[MAX_TRACKED_NOTES];
} channel;

channel tracked_channels[MAX_TRACKED_CHANNELS];

static struct {
        Bitu mpuport;
        Bitu sbport;
        Bitu mcellport;
        Bitu serialport;
        Bitu status;
        Bitu cmd_len;
        Bitu cmd_pos;
        Bit8u cmd_buf[8];
        Bit8u rt_buf[8];
        struct {
                Bit8u buf[SYSEX_SIZE];
                Bitu used;
                Bitu usedbufs;
                Bitu delay;
                Bit32u start;
        } sysex;
        bool fakeallnotesoff;
        bool available;
        /*MidiHandler * handler;*/ /* SOFTMPU */
} midi;

typedef struct {
        Bit8u ch;               //channel
        Bit8u note;             //note
        Bit8u priority;         //note priority
} mid_channel;

Bit8u chVolumes[MAX_TRACKED_CHANNELS];

mid_channel mcell_synth[MAX_MCELL_CHANNELS];    // MuzCell synth

unsigned short channelpitch[16];


unsigned char DrumCh[2]; // Drum channels 0..1

unsigned char DrumIdx;

/* SOFTMPU: Sysex delay is decremented from PIC_Update */
Bitu MIDI_sysex_delay;

// Note priority
Bit8u NotePriority;

/* SOFTMPU: Also used by MPU401_ReadStatus */
OutputMode MIDI_output_mode;

/* SOFTMPU: Initialised in mpu401.c */
extern QEMMInfo qemm;


void _fastcall mcellReset(void)
{
   unsigned char i;
   _asm
    {
        mov  dx,midi.mcellport
        add  dx,0fh
        mov  al,80h
                        cmp     qemm.installed,1
                        jne     RUntrappedOUT
                        push    ax
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        pop     ax
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        RUntrappedOUT:
        out  dx,al      // disable GATE (turn off all channels)

        mov  cx,7       // clear 3nE-3n8 mixer control
        xor  al,al
clr_mixer:
        dec  dx
                        cmp     qemm.installed,1
                        jne     MXUntrappedOUT
                        push    ax
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        pop     ax
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        MXUntrappedOUT:
        out  dx,al
        loop clr_mixer

        mov  dx,midi.mcellport
        add  dx,3       // 1st vi53 3n3 mode reg
        mov  al,03eh
                        cmp     qemm.installed,1
                        jne     C1UntrappedOUT
                        push    ax
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        pop     ax
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        C1UntrappedOUT:
        out  dx,al
        mov  al,07eh
                        cmp     qemm.installed,1
                        jne     C2UntrappedOUT
                        push    ax
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        pop     ax
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        C2UntrappedOUT:
        out  dx,al
        mov  al,0beh
                        cmp     qemm.installed,1
                        jne     C3UntrappedOUT
                        push    ax
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        pop     ax
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        C3UntrappedOUT:
        out  dx,al

        add  dx,4       // 2nd vi53 3n3 mode reg
        mov  al,03eh
                        cmp     qemm.installed,1
                        jne     C4UntrappedOUT
                        push    ax
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        pop     ax
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        C4UntrappedOUT:
        out  dx,al
        mov  al,07eh
                        cmp     qemm.installed,1
                        jne     C5UntrappedOUT
                        push    ax
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        pop     ax
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        C5UntrappedOUT:
        out  dx,al
        mov  al,0beh
                        cmp     qemm.installed,1
                        jne     C6UntrappedOUT
                        push    ax
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        pop     ax
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        C6UntrappedOUT:
        out  dx,al

        add  dx,8       // 3nF enable GATE (turn on)
        xor  al,al
                        cmp     qemm.installed,1
                        jne     GTUntrappedOUT
                        push    ax
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        pop     ax
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        GTUntrappedOUT:
        out  dx,al
    }
        for (i=0;i<16;i++)
            channelpitch[i] = 8192;

        for (i=0;i<MAX_MCELL_CHANNELS;i++)
        {
                mcell_synth[i].ch=0;
                mcell_synth[i].note=0;
                mcell_synth[i].priority=0;
        }
        for (i=0;i<MAX_TRACKED_CHANNELS;i++)
        {
                chVolumes[i] = DEFAULT_VOLUME;
        }
        NotePriority = 0;
        DrumCh[0] = 0;
        DrumCh[1] = 0;
        DrumIdx = 0;
}

void mcellNoteOff(unsigned char voice)
{
_asm
   {
                xor   bh,bh
                mov   bl,voice
                mov   dx,midi.mcellport

                add   dx,8      // 3n8 + voice
                add   dx,bx
                mov   al,00h
                        cmp     qemm.installed,1
                        jne     VRegUntrappedOUT
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        VRegUntrappedOUT:
                out   dx,al
    }
}


void mcellSetVolume(Bit8u voice,Bit8u amplitudeLeft,Bit8u amplitudeRight)
{
}

void mcellSound(unsigned char voice,unsigned short freq, unsigned char mixer, Bit8u amplitudeLeft,Bit8u amplitudeRight)
{
_asm
   {
                xor   bh,bh
                xor   ch,ch
                mov   bl,voice

                mov   dx,midi.mcellport
                mov   cl,ChanReg[bx]    ; bx = true channel (0 - 5)

                add   dx,cx
                mov   ax,freq
                        cmp     qemm.installed,1
                        jne     Fr1UntrappedOUT
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        Fr1UntrappedOUT:
                out   dx,al
                mov   al,ah
                        cmp     qemm.installed,1
                        jne     Fr2UntrappedOUT
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        Fr2UntrappedOUT:
                out   dx,al

                mov   dx,midi.mcellport
                add   dx,8
                add   dx,bx
                mov   al,mixer
                        cmp     qemm.installed,1
                        jne     MXUntrappedOUT
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        MXUntrappedOUT:
                out   dx,al
    }
}


void mcellDrum(unsigned char DChan,unsigned char freq)
{
_asm
   {
                mov   dx,midi.mcellport
                xor   ah,ah
                mov   al,DChan
                add   dx,0eh
                add   dx,ax
                mov   al,freq
                or    al,10h
                        cmp     qemm.installed,1
                        jne     DUntrappedOUT
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        DUntrappedOUT:
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
                mov   dx,midi.mcellport
                xor   ah,ah
                mov   al,DrumIdx
                add   dx,ax
                //add dx,0eh
                xor   al,al
                        cmp     qemm.installed,1
                        jne     DUntrappedOUT
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        DUntrappedOUT:
                out   dx,al
   }
}

static void PlayMsg_SBMIDI(Bit8u* msg,Bitu len)
{
        /* Output a MIDI message to the hardware using SB-MIDI */
        /* Wait for WBS clear, then output a byte */
        _asm
        {
                        mov     bx,msg
                        mov     cx,len                  ; Assume len < 2^16
                        add     cx,bx                   ; Get end ptr
                        mov     dx,midi.sbport
                        add     dx,0Ch                  ; Select DSP write port
        NextByte:       cmp     bx,cx
                        je      End
        WaitWBS:        cmp     qemm.installed,1
                        jne     WaitWBSUntrappedIN
                        push    bx
                        mov     ax,01A00h               ; QPI_UntrappedIORead
                        call    qemm.qpi_entry
                        mov     al,bl
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        WaitWBSUntrappedIN:
                        in      al,dx
                        or      al,al
                        js      WaitWBS
                        mov     al,038h                 ; Normal mode MIDI output
                        cmp     qemm.installed,1
                        jne     WaitWBSUntrappedOUT
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        WaitWBSUntrappedOUT:
                        out     dx,al
        WaitWBS2:       cmp     qemm.installed,1
                        jne     WaitWBS2UntrappedIN
                        push    bx
                        mov     ax,01A00h               ; QPI_UntrappedIORead
                        call    qemm.qpi_entry
                        mov     al,bl
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        WaitWBS2UntrappedIN:
                        in      al,dx
                        or      al,al
                        js      WaitWBS2
                        mov     al,[bx]
                        cmp     qemm.installed,1
                        jne     WaitWBS2UntrappedOUT
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        WaitWBS2UntrappedOUT:
                        out     dx,al
                        inc     bx
                        jmp     NextByte

                        ; Nothing more to send
        End:            nop
        }
};

static void PlayMsg_Serial(Bit8u* msg,Bitu len)
{
        /* Output a MIDI message to a serial port */
        /* Wait for transmit register clear, then output a byte */
        _asm
        {
                        mov     bx,msg
                        mov     cx,len                  ; Assume len < 2^16
                        add     cx,bx                   ; Get end ptr
                        mov     dx,midi.serialport
        NextByte:       add     dx,05h                  ; Select line status register
                        cmp     bx,cx
                        je      End
        WaitTransmit:   cmp     qemm.installed,1
                        jne     WaitTransmitUntrappedIN
                        push    bx
                        mov     ax,01A00h               ; QPI_UntrappedIORead
                        call    qemm.qpi_entry
                        mov     al,bl
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        WaitTransmitUntrappedIN:
                        in      al,dx
                        and     al,040h                 ; Shift register empty?
                        jz      WaitTransmit
                        sub     dx,05h                  ; Select transmit data register
                        mov     al,[bx]
                        cmp     qemm.installed,1
                        jne     WaitTransmitUntrappedOUT
                        push    bx
                        mov     bl,al                   ; bl = value
                        mov     ax,01A01h               ; QPI_UntrappedIOWrite
                        call    qemm.qpi_entry
                        pop     bx
                        _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                        ; Effectively skips next instruction
        WaitTransmitUntrappedOUT:
                        out     dx,al
                        inc     bx
                        jmp     NextByte

                        ; Nothing more to send
        End:            nop
        }
};

/*
float noteToFreq(unsigned char n, unsigned short pitch) {
    float a = 440; // frequency of A (coomon value is 440Hz)
//    return (a / 32) * pow(2, (((float)n - 9) / 12));
      return a * pow(2,(((float)n-69)/12)+((int)(pitch-8192)/4096*12));
}
*/

static void PlayMsg_MCELL(Bit8u* msg,Bitu len)
{
  Bit8u command = msg[0];
  Bit8u commandMSB  = command & 0xF0;
  Bit8u midiChannel = command & 0x0F;
  Bit8u octave;
  Bit8u noteVal;
  Bit8u i;
  Bit8u j;
  Bit8u voice;

  Bit8u mixer;
  Bit8u notefreq;
  Bit8u period;

  if (commandMSB == 0x80) //Note off
  {
    Bit8u note = msg[1];

    if (midiChannel == 9)
    {
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
    
    // We run out of voices, ignore note off command
    if(voice==MAX_MCELL_CHANNELS)
        {
                return;
        }

    // decrease priority for all notes greater than current
    for (i=0; i<MAX_MCELL_CHANNELS; i++)
        if (mcell_synth[i].priority > mcell_synth[voice].priority )
                mcell_synth[i].priority = mcell_synth[i].priority - 1;


    if (NotePriority != 0) NotePriority--;

    mcellNoteOff(voice);
    mcell_synth[voice].note = 0;
    mcell_synth[voice].priority = 0;
    } //if midichannel
  }
  else if (commandMSB == 0x90) //Note on
  {
    Bit8u note = msg[1];
    Bit8u velo = msg[2];

    if (velo != 0)
    {

        if (midiChannel == 9)
        {
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
        } //midiChannel == 9
        else
        {
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
                Bit8u min_prior = mcell_synth[0].priority;

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

        }

        mcell_synth[voice].ch = midiChannel;
        mcell_synth[voice].note = note;
        mcell_synth[voice].priority = NotePriority;


        velo = (chVolumes[midiChannel]*velo)/127;

        mixer = 0x28 | (((7-octavetable[note])+1) & 7);

//        notefreq = (unsigned short)noteToFreq(note,channelpitch[midiChannel]);

//        if (notefreq == 0) notefreq = 1;
//        period = 1021429 / notefreq;

        mcellSound(voice,periodtable[note],mixer,atten[velo],atten[velo]);

        //mcell_synth[voice].volume = velo;

        } // if midichannel

    }
    else if (velo == 0) //Turn Off note
        {
        if (midiChannel == 9)
        {
                mcellDrumOff(note);
        } //midiChannel ==9
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


        // We run out of voices, ignore note off command
        if(voice==MAX_MCELL_CHANNELS)
        {
                return;
        }

        // decrease priority for all notes greater than current
        for (i=0; i<MAX_MCELL_CHANNELS; i++)
                if (mcell_synth[i].priority > mcell_synth[voice].priority )
                        mcell_synth[i].priority = mcell_synth[i].priority - 1;

        if (NotePriority != 0) NotePriority--;

        mcellNoteOff(voice);
        mcell_synth[voice].note = 0;
        mcell_synth[voice].priority = 0;
        }
        }
  }
  else if (commandMSB == 0xA0) // Key pressure
  {
    //getSerialByte();
    //getSerialByte();
  }
  else if (commandMSB == 0xB0) // Control change
  {
    Bit8u controller = msg[1];
    Bit8u value = msg[2];

    //if (controller == 0x01) setDetune(value);
    if (controller == 0x07) //set main volume
    {
                //MasterVolume = value;
                chVolumes[midiChannel] = value;

                for (i=0;i<MAX_MCELL_CHANNELS;i++)
                  if ((mcell_synth[i].note != 0) && (mcell_synth[i].ch == midiChannel))
                        {
                                mcellSetVolume(i,atten[value],atten[value]);
                                //mcell_synth[i].volume = value;
                        }
    }
    else if ((controller==121) || (controller==123)) // All Sound/Notes Off
    {
                if (controller==121)
                {
                        chVolumes[midiChannel] = DEFAULT_VOLUME;
                        NotePriority = 0;
                }

                for (i=0;i<MAX_MCELL_CHANNELS;i++)
                  if (mcell_synth[i].note != 0)
                        {
                          mcellNoteOff(i);
                          mcell_synth[i].note = 0;
                          mcell_synth[i].priority = 0;
                        }
    }
  }
  else if (commandMSB == 0xC0) // Program change
  {
    //byte program = getSerialByte();
  }
  else if (commandMSB == 0xD0) // Channel pressure
  {
    //byte pressure = getSerialByte();
  }
  else if (commandMSB == 0xE0) // Pitch bend
  {
    //byte pitchBendLSB = getSerialByte();
    //byte pitchBendMSB = getSerialByte();
  }

/*
  for (i=0;i<MAX_MCELL_CHANNELS;i++)
    if (mcell_synth[i].note != 0)
        {
                noteVal = mcell_synth[i].volume-1;
                if (noteVal != 0)
                        {
                                cmsSetVolume(i,atten[noteVal],atten[noteVal]);
                                mcell_synth[i].volume = noteVal;
                        }
                else
                        {
                                cmsNoteOff(i);
                                mcell_synth[i].volume = 0;
                                mcell_synth[i].note = 0;
                        }
        }
*/
}

static void PlayMsg(Bit8u* msg,Bitu len)
{
        switch (MIDI_output_mode)
        {
        case M_MPU401:
                /* Output a MIDI message to the hardware */
                /* Wait for DRR clear, then output a byte */
                _asm
                {
                                mov     bx,msg
                                mov     cx,len                  ; Assume len < 2^16
                                add     cx,bx                   ; Get end ptr
                                mov     dx,midi.mpuport
                NextByte:       cmp     bx,cx
                                je      End
                                inc     dx                      ; Set cmd port
                WaitDRR:        cmp     qemm.installed,1
                                jne     WaitDRRUntrappedIN
                                push    bx
                                mov     ax,01A00h               ; QPI_UntrappedIORead
                                call    qemm.qpi_entry
                                mov     al,bl
                                pop     bx
                                _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                                ; Effectively skips next instruction
                WaitDRRUntrappedIN:
                                in      al,dx
                                test    al,040h
                                jnz     WaitDRR
                                dec     dx                      ; Set data port
                                mov     al,[bx]
                                cmp     qemm.installed,1
                                jne     WaitDRRUntrappedOUT
                                push    bx
                                mov     bl,al                   ; bl = value
                                mov     ax,01A01h               ; QPI_UntrappedIOWrite
                                call    qemm.qpi_entry
                                pop     bx
                                _emit   0A8h                    ; Emit test al,(next opcode byte)
                                                                ; Effectively skips next instruction
                WaitDRRUntrappedOUT:
                                out     dx,al
                                inc     bx
                                jmp     NextByte

                                ; Nothing more to send
                End:            nop
                }
                break;
        case M_SBMIDI:
                return PlayMsg_SBMIDI(msg,len);
        case M_SERIAL:
                return PlayMsg_Serial(msg,len);
        case M_CELL:
                return PlayMsg_MCELL(msg,len);
        default:
                break;
        }
};

/* SOFTMPU: Fake "All Notes Off" for Roland RA-50 */
static void FakeAllNotesOff(Bitu chan)
{
        Bitu note;
        channel* pChan;

        MIDI_note_off[0] &= 0xf0;
        MIDI_note_off[0] |= (Bit8u)chan;

        pChan=&tracked_channels[chan];

        for (note=0;note<pChan->used;note++)
        {
                MIDI_note_off[1]=pChan->notes[note];
                PlayMsg(MIDI_note_off,3);
        }

        pChan->used=0;
        pChan->next=0;
}

void MIDI_RawOutByte(Bit8u data) {
        channel* pChan; /* SOFTMPU */

        if (midi.sysex.start && MIDI_sysex_delay) {
                _asm
                {
                                ; Bit 4 of port 061h toggles every 15.085us
                                ; Use this to time the remaining sysex delay
                                mov     ax,MIDI_sysex_delay
                                mov     bx,17                   ; Assume 4kHz RTC
                                mul     bx                      ; Convert to ticks, result in ax
                                mov     cx,ax
                                in      al,061h
                                and     al,010h                 ; Get initial value
                                mov     bl,al
                TestPort:       in      al,061h
                                and     al,010h
                                cmp     al,bl
                                je      TestPort                ; Loop until toggled
                                xor     bl,010h                 ; Invert
                                loop    TestPort
                                mov     MIDI_sysex_delay,0      ; Set original delay to zero
                }
                /*Bit32u passed_ticks = GetTicks() - midi.sysex.start;
                if (passed_ticks < midi.sysex.delay) SDL_Delay(midi.sysex.delay - passed_ticks);*/ /* SOFTMPU */
        }

        /* Test for a realtime MIDI message */
        if (data>=0xf8) {
                midi.rt_buf[0]=data;
                PlayMsg(midi.rt_buf,1);
                return;
        }
        /* Test for a active sysex tranfer */
        if (midi.status==0xf0) {
                if (!(data&0x80)) {
                        /* SOFTMPU: Large sysex support */
                        /*if (midi.sysex.used<(SYSEX_SIZE-1))*/ midi.sysex.buf[midi.sysex.used++] = data;

                        if (midi.sysex.used==SYSEX_SIZE)
                        {
                                PlayMsg(midi.sysex.buf, SYSEX_SIZE);
                                midi.sysex.used = 0;
                                midi.sysex.usedbufs++;
                        }
                        return;
                } else {
                        midi.sysex.buf[midi.sysex.used++] = 0xf7;

                        if ((midi.sysex.start) && (midi.sysex.usedbufs == 0) && (midi.sysex.used >= 4) && (midi.sysex.used <= 9) && (midi.sysex.buf[1] == 0x41) && (midi.sysex.buf[3] == 0x16)) {
                                /*LOG(LOG_ALL,LOG_ERROR)("MIDI:Skipping invalid MT-32 SysEx midi message (too short to contain a checksum)");*/ /* SOFTMPU */
                        } else {
                                /*LOG(LOG_ALL,LOG_NORMAL)("Play sysex; address:%02X %02X %02X, length:%4d, delay:%3d", midi.sysex.buf[5], midi.sysex.buf[6], midi.sysex.buf[7], midi.sysex.used, midi.sysex.delay);*/
                                PlayMsg(midi.sysex.buf, midi.sysex.used); /* SOFTMPU */
                                if (midi.sysex.start) {
                                        if (midi.sysex.usedbufs == 0 && midi.sysex.buf[5] == 0x7F) {
                                            /*midi.sysex.delay = 290;*/ /* SOFTMPU */ // All Parameters reset
                                            MIDI_sysex_delay = 290*(RTCFREQ/1000);
                                        } else if (midi.sysex.usedbufs == 0 && midi.sysex.buf[5] == 0x10 && midi.sysex.buf[6] == 0x00 && midi.sysex.buf[7] == 0x04) {
                                            /*midi.sysex.delay = 145;*/ /* SOFTMPU */ // Viking Child
                                            MIDI_sysex_delay = 145*(RTCFREQ/1000);
                                        } else if (midi.sysex.usedbufs == 0 && midi.sysex.buf[5] == 0x10 && midi.sysex.buf[6] == 0x00 && midi.sysex.buf[7] == 0x01) {
                                            /*midi.sysex.delay = 30;*/ /* SOFTMPU */ // Dark Sun 1
                                            MIDI_sysex_delay = 30*(RTCFREQ/1000);
                                        } else MIDI_sysex_delay = ((((midi.sysex.usedbufs*SYSEX_SIZE)+midi.sysex.used)/2)+2)*(RTCFREQ/1000); /*(Bitu)(((float)(midi.sysex.used) * 1.25f) * 1000.0f / 3125.0f) + 2;
                                        midi.sysex.start = GetTicks();*/ /* SOFTMPU */
                                }
                        }

                        /*LOG(LOG_ALL,LOG_NORMAL)("Sysex message size %d",midi.sysex.used);*/ /* SOFTMPU */
                        /*if (CaptureState & CAPTURE_MIDI) {
                                CAPTURE_AddMidi( true, midi.sysex.used-1, &midi.sysex.buf[1]);
                        }*/ /* SOFTMPU */
                }
        }
        if (data&0x80) {
                midi.status=data;
                midi.cmd_pos=0;
                midi.cmd_len=MIDI_evt_len[data];
                if (midi.status==0xf0) {
                        midi.sysex.buf[0]=0xf0;
                        midi.sysex.used=1;
                        midi.sysex.usedbufs=0;
                }
        }
        if (midi.cmd_len) {
                midi.cmd_buf[midi.cmd_pos++]=data;
                if (midi.cmd_pos >= midi.cmd_len) {
                        /*if (CaptureState & CAPTURE_MIDI) {
                                CAPTURE_AddMidi(false, midi.cmd_len, midi.cmd_buf);
                        }*/ /* SOFTMPU */

                        if (midi.fakeallnotesoff)
                        {
                                /* SOFTMPU: Test for "Note On" */
                                if ((midi.status&0xf0)==0x90)
                                {
                                        if (midi.cmd_buf[2]>0)
                                        {
                                                pChan=&tracked_channels[midi.status&0x0f];
                                                pChan->notes[pChan->next++]=midi.cmd_buf[1];
                                                if (pChan->next==MAX_TRACKED_NOTES) pChan->next=0;
                                                if (pChan->used<MAX_TRACKED_NOTES) pChan->used++;
                                        }

                                        PlayMsg(midi.cmd_buf,midi.cmd_len);
                                }
                                /* SOFTMPU: Test for "All Notes Off" */
                                else if (((midi.status&0xf0)==0xb0) &&
                                         (midi.cmd_buf[1]>=0x7b) &&
                                         (midi.cmd_buf[1]<=0x7f))
                                {
                                        FakeAllNotesOff(midi.status&0x0f);
                                }
                                else
                                {
                                        PlayMsg(midi.cmd_buf,midi.cmd_len);
                                }
                        }
                        else
                        {
                                PlayMsg(midi.cmd_buf,midi.cmd_len);
                        }
                        midi.cmd_pos=1;         //Use Running status
                }
        }
}

bool MIDI_Available(void)  {
        return midi.available;
}

/* SOFTMPU: Initialisation */
void MIDI_Init(Bitu mpuport,Bitu mcellport,Bitu sbport,Bitu serialport,OutputMode outputmode,bool delaysysex,bool fakeallnotesoff){
        Bitu i; /* SOFTMPU */
        midi.sysex.delay = 0;
        midi.sysex.start = 0;
        MIDI_sysex_delay = 0; /* SOFTMPU */

        if (delaysysex==true)
        {
                midi.sysex.start = 1; /*GetTicks();*/ /* SOFTMPU */
                /*LOG_MSG("MIDI:Using delayed SysEx processing");*/ /* SOFTMPU */
        }
        midi.mpuport=mpuport;
        midi.mcellport=mcellport;
        midi.sbport=sbport;
        midi.serialport=serialport;
        midi.status=0x00;
        midi.cmd_pos=0;
        midi.cmd_len=0;
        midi.fakeallnotesoff=fakeallnotesoff;
        midi.available=true;
        MIDI_output_mode=outputmode;

        /* SOFTMPU: Display welcome message on MT-32 */
//        for (i=0;i<30;i++)
//        {
//                MIDI_RawOutByte(MIDI_welcome_msg[i]);
//        }

        mcellReset();

        /* SOFTMPU: Init note tracking */
        for (i=0;i<MAX_TRACKED_CHANNELS;i++)
        {
                tracked_channels[i].used=0;
                tracked_channels[i].next=0;
        }
}

/* DOSBox initialisation code */
#if 0
class MIDI:public Module_base{
public:
        MIDI(Section* configuration):Module_base(configuration){
                Section_prop * section=static_cast<Section_prop *>(configuration);
                const char * dev=section->Get_string("mididevice");
                std::string fullconf=section->Get_string("midiconfig");
                /* If device = "default" go for first handler that works */
                MidiHandler * handler;
//              MAPPER_AddHandler(MIDI_SaveRawEvent,MK_f8,MMOD1|MMOD2,"caprawmidi","Cap MIDI");
                midi.sysex.delay = 0;
                midi.sysex.start = 0;
                if (fullconf.find("delaysysex") != std::string::npos) {
                        midi.sysex.start = GetTicks();
                        fullconf.erase(fullconf.find("delaysysex"));
                        LOG_MSG("MIDI:Using delayed SysEx processing");
                }
                std::remove(fullconf.begin(), fullconf.end(), ' ');
                const char * conf = fullconf.c_str();
                midi.status=0x00;
                midi.cmd_pos=0;
                midi.cmd_len=0;
                if (!strcasecmp(dev,"default")) goto getdefault;
                handler=handler_list;
                while (handler) {
                        if (!strcasecmp(dev,handler->GetName())) {
                                if (!handler->Open(conf)) {
                                        LOG_MSG("MIDI:Can't open device:%s with config:%s.",dev,conf);
                                        goto getdefault;
                                }
                                midi.handler=handler;
                                midi.available=true;
                                LOG_MSG("MIDI:Opened device:%s",handler->GetName());
                                return;
                        }
                        handler=handler->next;
                }
                LOG_MSG("MIDI:Can't find device:%s, finding default handler.",dev);
getdefault:
                handler=handler_list;
                while (handler) {
                        if (handler->Open(conf)) {
                                midi.available=true;
                                midi.handler=handler;
                                LOG_MSG("MIDI:Opened device:%s",handler->GetName());
                                return;
                        }
                        handler=handler->next;
                }
                /* This shouldn't be possible */
        }
        ~MIDI(){
                if(midi.available) midi.handler->Close();
                midi.available = false;
                midi.handler = 0;
        }
};


static MIDI* test;
void MIDI_Destroy(Section* /*sec*/){
        delete test;
}
void MIDI_Init(Section * sec) {
        test = new MIDI(sec);
        sec->AddDestroyFunction(&MIDI_Destroy,true);
}
#endif

/* DOSBox MIDI handler code */
#if 0
class MidiHandler;

MidiHandler * handler_list=0;

class MidiHandler {
public:
        MidiHandler() {
                next=handler_list;
                handler_list=this;
        };
        virtual bool Open(const char * /*conf*/) { return true; };
        virtual void Close(void) {};
        virtual void PlayMsg(Bit8u * /*msg*/) {};
        virtual void PlaySysex(Bit8u * /*sysex*/,Bitu /*len*/) {};
        virtual const char * GetName(void) { return "none"; };
        virtual ~MidiHandler() { };
        MidiHandler * next;
};

MidiHandler Midi_none;

/* Include different midi drivers, lowest ones get checked first for default */

#if defined(MACOSX)

#include "midi_coremidi.h"
#include "midi_coreaudio.h"

#elif defined (WIN32)

#include "midi_win32.h"

#else

#include "midi_oss.h"

#endif

#if defined (HAVE_ALSA)

#include "midi_alsa.h"

#endif
#endif /* if 0 */
