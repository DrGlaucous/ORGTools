#pragma once

#include "Main.h"

#define READ_LE16(p) ((p[1] << 8) | p[0]); p += 2
#define READ_LE32(p) ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]); p += 4


//info that applies to a single track
typedef struct TRACKINFO
{

	unsigned short note_num{};//{} initializes the variable, so we can count on it being 0
	unsigned short frequency{};//note skew value (default, 1000)
	unsigned char wave_no{};//insturment
	unsigned char pipi{};//pipi value (I THINK it's a boolean?)



} TRACKINFO;

//info that applies to an entire ORG (used for iterating through every note in the tracks)
typedef struct ORGFILES
{
	TRACKINFO tracks[MAXTRACK];
	
	bool IsOrg{};//is the file we're looking at an ORG?

	//time signature marks
	unsigned char bar{};
	unsigned char dot{};

	unsigned short wait{};//tempo
	unsigned int totalNotes{};



}ORGFILES;

//each note gets this tag in some functions
typedef struct NOTEDATA
{
	unsigned int x{};
	unsigned char y{};
	unsigned char length{};
	unsigned char volume{};
	unsigned char pan{};
	unsigned char trackPrio{};//what track did this note come from? (used for track priority)

}NOTEDATA;


int gcd(int a, int b);
int LeastCommonMultiple(int num1, int num2);
void StretchSong(unsigned char* memfile, char bpmStretch, char dotStretch);
bool VerifyFile(const char* path);














