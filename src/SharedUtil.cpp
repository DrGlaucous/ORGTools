//contains utilities used by all ORGTools
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>


#include "File.h"
#include "SharedUtil.h"
#include "Main.h"



//for iterating through a single org's tracks (stores note count of each track)
TRACKINFO tracks[MAXTRACK];



//used to determine the new beat structure of the song
int gcd(int a, int b) {
	//gcd: greatest common difference
	if (b == 0)
		return a;
	return gcd(b, a % b);
}
int LeastCommonMultiple(int num1, int num2)
{
	return (num1 * num2) / gcd(num1, num2);

}


//change the timing of a song, give it the pointer to the file and the stretch values
void StretchSong(unsigned char* memfile, char bpmStretch, char dotStretch)
{

	//takes the signature and adjusts it accordingly
	memfile[8] = memfile[8] * bpmStretch;
	memfile[9] = memfile[9] * dotStretch;

	//adjusts tempo (bytes 6 and 7)
	short newTempo = (memfile[7] << 8) | memfile[6];//read data into our short
	newTempo = newTempo / (bpmStretch * dotStretch);//set the tempo relative to the stretch value
	for (unsigned int i = 0; i < 2; ++i)//push the updated values back to the data
		memfile[i + 6] = newTempo >> (8 * i);


	//we need to change the X value of all the notes and the leingth of all the notes
	//we need to use note_num to determine the note ammount in each track



	memfile += 18;//end of header data
	//iterate through all tracks and get the number of notes in each one
	for (int i = 0; i < MAXTRACK; i++)
	{
		memfile += 4;//jump to note_num value

		tracks[i].note_num = READ_LE16(memfile);
	}

	//iterate through all tracks to append new note values
	for (int i = 0; i < MAXTRACK; i++)
	{

		//this makes the new x values
		for (int j = 0; j < tracks[i].note_num; ++j)//for each note
		{

			int writeOut = READ_LE32(memfile);//get x value and multiply it by the total stretch value
			writeOut *= (bpmStretch * dotStretch);

			memfile -= 4;//undo the previous advance

			for (unsigned int i = 0; i < 4; ++i)//write to the memfile
				memfile[i] = writeOut >> (8 * i);

			memfile += 4;//advance to next value


		}

		memfile += tracks[i].note_num;//jump over Y value and go to length value

		//this makes new length values
		for (int j = 0; j < tracks[i].note_num; ++j)//for each note
		{
			char writeOut = memfile[0];//get the value of the leingth
			writeOut *= (bpmStretch * dotStretch);

			memfile[0] = writeOut;
			++memfile;//advance to next number


		}

		memfile += (tracks[i].note_num * 2);//skip to end of data
	}

}


//checks (and removes) any "s or 's around the file path
//(this function won't see as much use in the GUI application)
void CheckForQuote(std::string* inpath)
{
	std::string path = *inpath;

	//check to see if the path is enclosed in " or '
	if (path.size() > 0)
	{
		if (
			((*path.begin() == '\"') &&
				(*(path.end() - 1) == '\"')) ||
			((*path.begin() == '\'') &&
				(*(path.end() - 1) == '\''))
			)
		{
			path = path.substr(1, path.size() - 2);//remove the enclosing ""s from the input
		}
	}

	*inpath = path;


}





//char pass[7] = "Org-01";
//char pass2[7] = "Org-02";	// Pipi
//char pass3[7] = "Org-03";
#define PASS0 "Org-01"
#define PASS1 "Org-02"
#define PASS2 "Org-03"

//tells if the file is an ORG or not
bool VerifyFile(const char* path)
{



	FILE* file = fopen(path, "rb");

	if (file == NULL)
	{
		return false;
	}

	char header[6];

	fread(header, 1, 6, file);//get header

	if (memcmp(header, PASS0, 6) != 0 &&
		memcmp(header, PASS1, 6) != 0 &&
		memcmp(header, PASS2, 6) != 0
		)//see if the header matches either orgv1, orgv2, or orgv3
	{
		return false;
	}

	fclose(file);

	return true;
}





















