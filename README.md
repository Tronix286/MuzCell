# MuzCell
 Newly made ISA8 sound card. Replica to "Music Sintez Cell" from old soviet "Agat" PC
 
![render](hardware/v1_0_0/render3.PNG)

## Prologue
The prototype of this sound card is a "musical cell" from the old Soviet computer "Agat" (partially Apple II compatible). More information about it here: http://agatcomp.ru/agat/Hardware/SoundNCL/jzs52.shtml (in Russian). So, this sound card is based on original Agat's "musical cell" circuit but the necessary changes have been made to work on the PC ISA8 bus. Also, some components have been replaced with more affordable ones, the timer channel with the IRQ handler was removed because the PC already has its own timer as part of the motherboard.

## Description
This sound card based on two i8253 timers IC (K580VI53 rus). Each i8253 contains three counters channels, two i8253 provide six independed channels. 
*Six counters are masters for the tonal channels. 
*Six tone channel switching units - provide a smooth attack and signal transmission from timers / counters to bandpass filters.
*Passive bandpass RC filters - extract parts of the spectrum in tone channels.
*Noise generator and drum generators (a group of three CMOS IC) - form two drum channels.

Comming sooon.....
