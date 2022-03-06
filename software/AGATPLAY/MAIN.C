#include <dos.h>    /* REGS */
#include <stdlib.h>
#include <stdio.h>  /* printf() */
#include <conio.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include "m6502.h"
#include "timer.h"

#define MEM_SIZE 64000
#define IRQ_CLOCK 7980

//#define BE_VERBOSE

M6502 cpuregs;
byte *mem;
//byte *ROM;
char romfile[32]="playerr.o";
FILE *f;
long sz;

//vi53 load-preload
word period;
word period_ms;
byte rl;

byte key_pressed;

byte enable_timer;
//clock_t start_time;
unsigned long start_time;

/* waits for a keypress and return it. Returns 0 for extended keystroke, then
   function must be called again to return scan code. */
int getkey(void) {
  union REGS regs;
  int res;
  regs.h.ah = 0x08;
  int86(0x21, &regs, &regs);
  res = regs.h.al;
  if (res == 0) { /* extended key - poll again */
    regs.h.ah = 0x08;
    int86(0x21, &regs, &regs);
    res = regs.h.al | 0x100;
  }
  return(res);
}

/* poll the keyboard, and return the next input key if any, or -1 */
int getkey_ifany(void) {
  union REGS regs;
  regs.h.ah = 0x0B;
  int86(0x21, &regs, &regs);
  if (regs.h.al == 0xFF) {
    return(getkey());
  } else {
    return(-1);
  }
}

/*; –егистры карты
; первый таймер
TIMER1_CH0	.equ	$C090
TIMER1_CH1	.equ	$C091
TIMER1_CH2	.equ	$C092
TIMER1_CTL	.equ	$C093

; второй таймер
TIMER2_CH0	.equ	$C094
TIMER2_CH1	.equ	$C095
TIMER2_CH2	.equ	$C096
TIMER2_CTL	.equ	$C097

; 5 регистров тональных каналов
CONTROL_CHN1	.equ	$C098
CONTROL_CHN2	.equ	$C099
CONTROL_CHN3	.equ	$C09A
CONTROL_CHN4	.equ	$C09B
CONTROL_CHN5	.equ	$C09C

; 2 регистра ударных каналов
CONTROL_DRUM1	.equ	$C09D
CONTROL_DRUM2	.equ	$C09E

; регистр управлени€ прерывани€ми
; D7 = 1 запрет прерываний и работы таймеров
; D6 = 1 запрет прерываний от внешнего источника
CONTROL_IRQ	.equ	$C09F
*/

void Wr6502(register word Addr,register byte Value)
{
	switch (Addr)
	{
		case 0xC096:
			#ifdef BE_VERBOSE
			printf("IRQ_INTERVAL %2X\n",Value);
			#endif
                        if (rl==0)
				{
					rl=1;
					period = Value;
				}
			else
				{
					rl = 0;
					period = (Value << 8) | period;
					if (period == 0) period = 1;
					period_ms = 1000 / (IRQ_CLOCK / period);
					#ifdef BE_VERBOSE
					printf("PERIOD = %X ms = %u\n",period,period_ms);
					#endif
				}
			break;
		case 0xC090:
		case 0xC091:
		case 0xC092:
		case 0xC093:
			#ifdef BE_VERBOSE
			printf("TIMER1 port 0x%2X = %2X\n",Addr-0xBD90,Value);
			#endif
			outp(Addr-0xBD90, Value);
			break;
		case 0xC094:
		case 0xC095:
		case 0xC097:
			#ifdef BE_VERBOSE
			printf("TIMER2 port 0x%2X = %2X\n",Addr-0xBD90,Value);
			#endif
			outp(Addr-0xBD90, Value);
			break;
		case 0xC098:
		case 0xC099:
		case 0xC09A:
		case 0xC09B:
		case 0xC09C:
			#ifdef BE_VERBOSE
			printf("CONTROL port 0x%2X = %2X\n",Addr-0xBD90,Value);
			#endif
			outp(Addr-0xBD90, Value);
			break;
		case 0xC09D:
		case 0xC09E:
			#ifdef BE_VERBOSE
			printf("DRUM port 0x%2X = %2X\n",Addr-0xBD8F,Value);
			#endif
			outp(Addr-0xBD8F, Value);
			break;
		case 0xC09F:
			#ifdef BE_VERBOSE
			printf("CONTROL_IRQ = %2X\n",Value);
			#endif
			if (Value & 0x80)
			{

			   enable_timer = 0;
			}
			else
			{
			   #ifdef BE_VERBOSE
			   printf("---timer enabled\n");
			   #endif
			   //enable_timer = 1;
			   //start_time = clock();
			   timer_reset();
			   timer_read(&start_time);
			   enable_timer = 1;
			}
			break;
	}
	mem[Addr] = Value;
}
/*
; јдреса устройств
KEYBOARD	.equ	$C000
KBDSTROBE	.equ	$C010
*/
byte Rd6502(register word Addr)
{
	if ((Addr == 0xC000) & (key_pressed == 1))
	  return 0x80;
	else
	  return mem[Addr];
}

byte Loop6502(register M6502 *R)
{
byte ret;
unsigned long curtime;

	ret = INT_NONE;
	if (getkey_ifany() != -1)
		key_pressed = 1;

	if (enable_timer)
	{
		timer_read(&curtime);
		//if ((clock()-start_time) * 1000 / CLOCKS_PER_SEC > period_ms)
		if ((curtime-start_time) / 858 > period_ms)
		{
			#ifdef BE_VERBOSE
			printf("!!IRQ\n");
			#endif
			//start_time = clock();
			timer_reset();
			timer_read(&start_time);
			return INT_IRQ;
		}
	}
        //udelay(100);
        return ret;
}

byte Patch6502(register byte Op,register M6502 *R)
{
	printf("*** Program stopped at $%x\n", R->PC.W);
	return 0;
}

int main(int argc, char **argv) 
{
 printf("AGATPLAY v1.0 (c) Tronix 2022\n\n");
 mem=malloc(MEM_SIZE);
 if (mem==NULL)
 {
	printf("unable to allocate memory\n");
	exit(1);
 }
 rl =0;
 period = 0;
 period_ms = 0;
 key_pressed = 0;

 if (argc < 2)
 {
   printf("agatplay <music.mus>\n");
   exit(1);
 }
	memset(mem,0,MEM_SIZE);
	cpuregs.IPeriod = 100;
	Reset6502(&cpuregs);

 //read rom
 f=fopen(romfile,"rb");
 if (!f) {
	printf("6502 binary module PLAYERR.O not found\n");
	exit(1);
 }
// ROM=(byte *)malloc(0x8000);

 fseek(f, 0L, SEEK_END);
 sz = ftell(f);
 fseek(f, 0L, SEEK_SET);

 fread(mem+0xe000,sz,1,f);
 fclose(f);

 timer_init();

 printf("PLAYERR.O loaded, start 6502 emu, preparing tables....\n");

 cpuregs.PC.W = 0xe700;
// cpuregs.Trace = 1;
 cpuregs.Trap = 0xe85b;
 cpuregs.IPeriod = 10000;

 Run6502(&cpuregs);

  printf
  (
    "BRKPT AT PC: [%02X]   AT SP: [%02X %02X %02X]\n",
    Rd6502(cpuregs.PC.W),
    Rd6502(0x0100+(byte)(cpuregs.S+1)),
    Rd6502(0x0100+(byte)(cpuregs.S+2)),
    Rd6502(0x0100+(byte)(cpuregs.S+3))
  );

 printf("Loading music file to 0x8000...\n");
 if (argc>1) strcpy(romfile,argv[1]);

 f=fopen(romfile,"rb");
 if (!f) {
	printf("file not found!\n");
	exit(1);
 }

 fseek(f, 0L, SEEK_END);
 sz = ftell(f);
 fseek(f, 0L, SEEK_SET);

 fread(mem+0x8000,sz,1,f);
 fclose(f);

  /* initialize the high resolution timer */
  //timer_init();

 printf("Playing %s now. Press any key to terminate...\n",romfile);

 cpuregs.PC.W = 0xe000;
 cpuregs.IPeriod = 1000;
// cpuregs.IAutoReset = 1;
 //cpuregs.Trace = 1;
 cpuregs.Verbose = 1;
 cpuregs.Trap = 0xe064;

 Run6502(&cpuregs);

/*
 f = fopen( "dump.mem" , "w" );
 fwrite(mem,MEM_SIZE,1,f);
 fclose(f);
*/
 return(0);
}
