#ifndef mcell_h_sentinel
#define mcell_h_sentinel

#define MAX_MCELL_CHANNELS 6

typedef struct {
        unsigned char note;
        unsigned char priority;
        unsigned char ch;
        unsigned char voice;
} mid_channel;

void mcellProgChange(int channel, int program);

void mcell_pitchwheel(unsigned short oplport, int channel, int pitchwheel);

void mcellReset(unsigned short port);

void mcellNoteOn(unsigned char channel, unsigned char note, unsigned char velocity);

void mcellNoteOff(unsigned char channel, unsigned char note);

void mcellController(unsigned char channel, unsigned char id, unsigned char val);

#endif
