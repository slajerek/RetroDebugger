

#ifndef _MAC_MIDI_H_
#define _MAC_MIDI_H_ 

extern void InitializeMacMidi();
extern int GetMidiNote();
extern int MidiEventPending();

extern int GetPolyphonicMode();
extern void SetPolyphonicMode(int flag);

#endif