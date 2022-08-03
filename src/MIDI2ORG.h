#pragma once


#include <stddef.h>
#include <stdio.h>

#include "SharedUtil.h"


//data that we glean from the MIDI file
typedef struct MIDI_NOTEDATA
{
	int TimeStart{};
	int TimeRelative{};
	//int Length;
	int Pitch{};
	int Channel{};
	//int Pan;
	char Volume{};
	bool Status{}; //on/off
}MIDI_NOTEDATA;

//data that we push to the ORG file (different from the ORGFILES struct!)
typedef struct ORGNOTEDATA
{
	int TimeStart{};
	int Length{};
	unsigned char Pitch{};
	unsigned char Volume{};
	bool LengthSet{};//tell us if we found the stopping point of this note
	bool EventType{};//true == start, false == stop

}ORGNOTEDATA;


//options passed from the backend to the engine (bundled into a struct for simplicity)
typedef struct MIDICONV
{
	const char* Path;

	bool ForceSimplify;//MIDI addresses are much bigger than ORG addresses. The program will automatically try to find the gcd, but if the files are still big, this forces loss of definition in favor of a more managable file
	int SimplestNote;//put the denominator of the smallest accurately positioned note here (I.E 1/4 note == 4, 1/2 note == 2)

	bool HasDrumChannel;
	int DrumChannel;//this channel will be processed differently from the others

}MIDICONV;





//options set in the GUI
typedef struct MIDI2ORGOPTIONS
{




}MIDI2ORGOPTIONS;



















