#pragma once


#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif


#define VERSION_NO "1.0.0a"
#define PATH_LENGTH 256
#define MAXTRACK 16
#define MAXCHANNEL 16//MIDI's max supported insturment channels (each track can have this ammount)
#define TERMINAL_MAX_BUFFER 128

//gives tab options for the ImGUI demo and shows perfomrance window during MIDI processing
//#define DEBUG_MODE



























