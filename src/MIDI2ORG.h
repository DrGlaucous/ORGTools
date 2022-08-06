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
	char Path[PATH_LENGTH]{};

	bool ForceSimplify{};//MIDI addresses are much bigger than ORG addresses. The program will automatically try to find the gcd, but if the files are still big, this forces loss of definition in favor of a more managable file
	int SimplestNote = 1;//put the denominator of the smallest accurately positioned note here (I.E 1/4 note == 4, 1/2 note == 2)

	bool HasDrumChannel{};
	int DrumChannel = 9;//this channel will be processed differently from the others

}MIDICONV;





//options set in the GUI
typedef struct MIDI2ORGOPTIONS
{
	MIDICONV BackendOptions{};//all of our options are already defined here

	//I decided to keep this out of the MIDICONV struct because this variable is ONLY used by the handler and not the backend
	char PathOLD[PATH_LENGTH]{};

	int IsMIDI{};

	//used to tell the backend to actually start
	bool BeginCopy{};
	//tells the window that the process finished
	bool FinishedCopy{};
	//tells the UI that MIDI2ORG is running a process
	bool RunningCopy{};

	bool Success{};//did the copy engine perform the operation correctly?

	//MIDICONV needs values that are 2^x. this is x
	int ForceSimplifyExponet = 1;



	//used to report feedback in the MIDI2ORG menu
	char* TerminalText;
	//va_list TerminalArgs;



}MIDI2ORGOPTIONS;


extern MIDI2ORGOPTIONS MidiConvertParams;

void HandleMIDI2ORGBackend(void);














