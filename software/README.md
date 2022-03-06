# MuzCell software

* MUZ_TEST - Diagnostics and test MuzCell;
* DOSMID - Modified version of DOSMID MIDI player to support MuzCell output;
* AGATPLAY - Player for original Agat's music files *.MUS

## MUZ_TEST

Simple diagnostics software for auto detect MuzCell IO port and control channels/mixer.
Use PgUp/PgDwn to increase/decrease tone channels frequency;
Spacebar to check or unchech bits in corntrol registers;
Esc to quit.

## DOSMID

http://dosmid.sourceforge.net/

DOSMID a low-requirements MIDI player for DOS (C) Mateusz Vistelink. 
This is a modified version with MuzCell music card support.

New DOSMid options:

```
/mcell[=XXX]  Force dosmid to use MuzCell on port XXX.  The port part 
is optional, that means you can use "/mcell" to just force MuzCell 
usage on default 300h port.
```

## AGATPLAY

Player for original Agat's music *.MUS files.

Usage: 
```
AGATPLAY <music.MUS>
```
