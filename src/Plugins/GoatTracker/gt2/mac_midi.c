
#include <CoreMIDI/MIDIServices.h>
#include <stdio.h>

#include "goattrk2.h"

//#define DUMP_MIDI_PACKETS

static int lastnote = -1;
static int midievent_pending = 0;

static int polyphonic = 0;
static int polyphony_notes[3] = { -1, -1, -1 };

int GetPolyphonicMode()
{
	return polyphonic;
}

void SetPolyphonicMode(int flag)
{
	polyphonic = flag;
}


int GoatTrackerNoteFromMidiNote(int inMidiNote)
{
	int gt_note = inMidiNote - 0x0c;
	if (gt_note < 0)
		return -1;
	
	gt_note = FIRSTNOTE + gt_note;
	
	if (gt_note > LASTNOTE)
		return -1;

	return gt_note;
}


static void	sMacMidiPacketCallback(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
	MIDIPacket *packet = (MIDIPacket *)pktlist->packet;	// remove const (!)
	unsigned int j, p;
	int newnote;
	int offset;
	
	for (j = 0; j < pktlist->numPackets; ++j)
	{
#ifdef DUMP_MIDI_PACKETS
		int i;
		for (i = 0; i < packet->length; ++i)
		{
			printf("%02X ", packet->data[i]);
		}
		
		printf("\n");
#endif
		
		offset = 0;
		while(offset < (packet->length - 2))
		{
			if ((packet->data[offset] & 0xf0) == 0x90)
			{
				if (polyphonic)
				{
					if (packet->data[offset + 2] > 0x00)
					{
						newnote = GoatTrackerNoteFromMidiNote(packet->data[offset + 1]);
						
						for (p = 0; p < 3; p++)
						{
							if (polyphony_notes[p] == -1)
							{
								playtestnote(newnote, einum, p);
								polyphony_notes[p] = packet->data[offset + 1];
								break;
							}
						}
					}
					else if (packet->data[offset + 2] == 0x00)
					{
						for (p = 0; p < 3; p++)
						{
							if (packet->data[offset + 1] == polyphony_notes[p])
							{
								releasenote(p);
								polyphony_notes[p] = -1;
								break;
							}
						}
					}
					
				}
				else
				{
					if (packet->data[offset + 2] > 0x00)
					{
						newnote = GoatTrackerNoteFromMidiNote(packet->data[offset + 1]);

#ifdef DUMP_MIDI_PACKETS
						printf("note: 0x%02x\n", newnote);
#endif
						
						playtestnote(newnote, einum, epchn);
						lastnote = packet->data[offset + 1];
					}
					else if (packet->data[offset + 2] == 0x00 && lastnote == packet->data[offset + 1])
					{
						releasenote(epchn);
						lastnote = -1;
					}

					midievent_pending = 1;
				}
				
			}
			
			offset += 3;
		}
			
		packet = MIDIPacketNext(packet);
	}
}


MIDIPortRef inPort = NULL;
int foundSources = 0;

int MidiEventPending()
{
	return 0;
	
	
	int num_sources = MIDIGetNumberOfSources();
	if (num_sources != foundSources)
	{
		foundSources = num_sources;
		int i;
		for (i = 0; i < foundSources; ++i)
		{
			MIDIEndpointRef src = MIDIGetSource(i);
			MIDIPortConnectSource(inPort, src, NULL);
		}
	}
	
	if (midievent_pending)
	{
		midievent_pending = 0;
		return 1;
	}
	
	return 0;
}

int GetMidiNote()
{
	if (lastnote == -1)
		return -1;
	
	return GoatTrackerNoteFromMidiNote(lastnote);
}



void InitializeMacMidi()
{
	return;
	
	// create client and ports
	MIDIClientRef client = NULL;
	MIDIClientCreate(CFSTR("GoatTracker"), NULL, NULL, &client);
	
	MIDIInputPortCreate(client, CFSTR("Input port"), sMacMidiPacketCallback, NULL, &inPort);
	
	// open connections from all sources
	foundSources = MIDIGetNumberOfSources();
	//printf("%d sources\n", foundSources);
	int i;
	for (i = 0; i < foundSources; ++i)
	{
		MIDIEndpointRef src = MIDIGetSource(i);
		MIDIPortConnectSource(inPort, src, NULL);
	}
}
