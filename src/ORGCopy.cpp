#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>

#include "Main.h"
#include "SharedUtil.h"
#include "ORGCopy.h"
#include "File.h"


//options for the orgCopy engine to follow
ORGCopyOPTIONS OrgCopyParams{};

//info for both the copied and pasted ORGs
ORGFILES orgs[2];


bool SortFunction(NOTEDATA one, NOTEDATA two)
{
	return (one.x < two.x);

}

//converts the QWERTY drum tracks into an int usable by the other functions
//like the de-quoter, this one won't see as much use when we have a GUI to work with
int ParseLetterInput(const char* input)//this function will just look at the first letter in the pointer (not a problem if you only give it one letter)
{
	switch (tolower(*input))
	{
		break;
	case 'q':
		return 9;
		break;
	case 'w':
		return 10;
		break;
	case 'e':
		return 11;
		break;
	case 'r':
		return 12;
		break;
	case 't':
		return 13;
		break;
	case 'y':
		return 14;
		break;
	case 'u':
		return 15;
		break;
	case 'i':
		return 16;
		break;
	default:
		return 0;

	}

}

//populates the ORGFILES
ORGFILES GetOrgInfo(const char* path)
{
	ORGFILES ThisOrg{};

	ThisOrg.IsOrg = VerifyFile(path);

	FILE* file = fopen(path, "rb");

	//return 0 if failed
	if (file == NULL)
	{
		memset(&ThisOrg, 0, sizeof(ThisOrg));
		return ThisOrg;
	}

	fseek(file, 6, SEEK_SET);//jump to tempo readings

	ThisOrg.wait = File_ReadLE16(file);//read tempo

	fread(&ThisOrg.bar, 1, 1, file);//read Beats per measure
	fread(&ThisOrg.dot, 1, 1, file);//read resolution

	fseek(file, 18, SEEK_SET);//jump to track headers

	for (int i = 0; i < MAXTRACK; ++i)
	{
		ThisOrg.tracks[i].frequency = File_ReadLE16(file);//read tempo
		ThisOrg.tracks[i].wave_no = File_ReadLE8(file);//read other things
		ThisOrg.tracks[i].pipi = File_ReadLE8(file);
		ThisOrg.tracks[i].note_num = File_ReadLE16(file);

		//tally total notes in the ORG
		ThisOrg.totalNotes += ThisOrg.tracks[i].note_num;
	}




	fclose(file);

	return ThisOrg;

}


bool CopyOrgData(std::string Path1, std::string Path2, unsigned int TrackCopy, unsigned int TrackPaste, bool MASH, int PrioFile)
{

	memset(orgs, 0, sizeof(orgs));//ensure default values are 0


	size_t file_size_1;//copied file
	size_t file_size_2;//copy to file
	unsigned char* memfile[4];

	memfile[0] = LoadFileToMemory(Path1.c_str(), &file_size_1);//track 1 (the one we are reading)
	memfile[1] = LoadFileToMemory(Path2.c_str(), &file_size_2);//track 2 (the one we are also reading, but with the intent of swapping info inside with stuff from file 1)
	memfile[2] = memfile[0] + 18;//used to restore the position of the pointers to the start of the file
	memfile[3] = memfile[1] + 18;

	//we need to break before we hit the fopen function call, because otherwise, it will erase the file
	//
	if (memfile[0] == NULL || memfile[1] == NULL)
	{
		return false;
	}


	//Path2 = Path2 + "2.org";//used for debugging
	FILE* file = fopen(Path2.c_str(), "wb");//"wb");//open new org file for writing


	int BLCM = LeastCommonMultiple(memfile[0][8], memfile[1][8]);//beats per measure
	int DLCM = LeastCommonMultiple(memfile[0][9], memfile[1][9]);//notes per beat
	StretchSong(memfile[0], BLCM / memfile[0][8], DLCM / memfile[0][9]);//find the multiplier by dividing the LCM by the value in question
	StretchSong(memfile[1], BLCM / memfile[1][8], DLCM / memfile[1][9]);


	//std::cout << (int)memfile[1][8];//beats per measure
	//std::cout << (int)memfile[1][9];//notes in each beat


	fwrite(memfile[1], 18, 1, file);//write the header

	memfile[1] += 18;//end of header data
	memfile[0] += 18;

	//copy the note count up to the copied track (because we don't need anything after this)
	for (unsigned int i = 0; i < TrackCopy; ++i)
	{
		orgs[0].tracks[i].note_num = (memfile[0][5] << 8) | memfile[0][4];
		memfile[0] += 6;
	}
	//iterate to the position of the copied track
	orgs[0].tracks[TrackCopy].note_num = (memfile[0][5] << 8) | memfile[0][4];//read the note ammount of this track, but do not advance, so the data can be written in the function below



	//rundown on how MASH works (TL;DR edition)
	//copy track 'from' to vector 0 and copy track 'to' to vector 1
	//jam both bits of track data into vector 2
	//sort vector 2 by x values
	//clean out any overlapping x values
	//done

	//if we are combining tracks, we have to do things a little differently
	if (MASH)
	{
		std::vector<NOTEDATA> TrackNotes[3];//the extra track is the mashed product of the first 2... we could probably simplify this and put ALL notes into the combined vector to begin with...
		NOTEDATA BufferNote;
		//int NoteCount[2];
		unsigned int TracksCP[2] = { TrackCopy, TrackPaste };//so we can iterate through them
		memset(&BufferNote, 0, sizeof(BufferNote));


		memfile[2] = memfile[0];//stash memfile2 at the copied track's header (so we can write it back later)
		memfile[3] = memfile[1];//stash memfile3 at the start of the header

		//WRITE all headers until the paste destination, then advance the pointer to the end of the header
		for (unsigned int i = 0; i < MAXTRACK; ++i)
		{
			if (i < TrackPaste)
			{
				fwrite(memfile[1], 6, 1, file);//write the headers of everything up to the target track
				memfile[3] += 6;//advance post-header pointer to next track (to finish header writeback after we perform MASH calculations)
			}


			orgs[1].tracks[i].note_num = (memfile[1][5] << 8) | memfile[1][4];//read the note ammount of this track
			memfile[1] += 6;//advance to next track

		}
		memfile[0] += ((size_t)(MAXTRACK - TrackCopy) * 6);//iterate to the end of the header




		//we moved the read memfile to the start of the conflicting track, so we will move the writing one there, too
		//will write out everything UP TO the conflicting track (and leave the pointer there so we can merge both data at that track)
		for (unsigned int i = 0; i < TrackPaste; ++i)
		{
			//fwrite(memfile[1], (8 * orgs[1].tracks[i].note_num), 1, file);//write the notes for this track (the header isnt done yet, we can't do this)

			memfile[1] += (8 * orgs[1].tracks[i].note_num);//move pointer forward

		}
		//send the memfile to the start of the track to be copied (recall that memfile0 is the one we read ONLY)
		for (unsigned int i = 0; i < TrackCopy; ++i)
		{
			memfile[0] += (orgs[0].tracks[i].note_num * 8);
		}




		//copy note data into our vectors (moves memfile pointers to the end of the data chunk)
		//for both tracks
		//the track copied last has the highest priority
		for (int i = 0; i < 2; ++i)
		{

			//for every note in the vector we are copying
			//x and trackID
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				//copy X to BufferNote
				BufferNote.x =
					memfile[i][3] << 24 |
					memfile[i][2] << 16 |
					memfile[i][1] << 8 |
					memfile[i][0];
				memfile[i] += 4;

				BufferNote.trackPrio = i;

				TrackNotes[i].push_back(BufferNote);//append vector
			}

			//now that every entry in the vector has been made, we can just go back through and adjust the values
			//y
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				//copy and advance pointer
				(TrackNotes[i].begin() + j)->y = memfile[i][0];
				memfile[i] += 1;

			}
			//length
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				(TrackNotes[i].begin() + j)->length = memfile[i][0];
				memfile[i] += 1;
			}
			//volume
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				(TrackNotes[i].begin() + j)->volume = memfile[i][0];
				memfile[i] += 1;
			}
			//pan
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{
				(TrackNotes[i].begin() + j)->pan = memfile[i][0];
				memfile[i] += 1;
			}

			//tie event notes to parents
			for (int j = 0; j < orgs[i].tracks[TracksCP[i]].note_num; ++j)
			{ 
				//find event notes
				if ((TrackNotes[i].begin() + j)->y == 0xFF)
				{
					//go backwards until we find a note whose length value extends over this one
					for (int backJ = j; backJ >= 0; --backJ)
					{
						//if note is NOT an event AND its x+length is >= to the current event note
						if ((TrackNotes[i].begin() + backJ)->y != 0xFF &&
							(TrackNotes[i].begin() + backJ)->length +
							(TrackNotes[i].begin() + backJ)->x >=
							(TrackNotes[i].begin() + j)->x
							)
						{
							//keep track of note parent
							(TrackNotes[i].begin() + j)->eventHasParent = true;
							(TrackNotes[i].begin() + j)->eventParent = (TrackNotes[i].begin() + backJ)->x;
						}


					}

				}
			}



		}



		//Merge vectors

		//if note x + length is greater than the next note's x value, 
		//if next note's length is greater than note's x + length, 

		//trackNotes0 will have priority

		//push everything into vector 3
		for (int p = 0; p < 2; ++p)
		{
			for (int i = 0; i < TrackNotes[p].size(); ++i)
			{
				TrackNotes[2].push_back(*(TrackNotes[p].begin() + i));
			}
		}

		std::sort(TrackNotes[2].begin(), TrackNotes[2].end(), SortFunction);//arranges them in order of X value

		//deletes entries based on priority and conflicting X and length values
		for (int i = 1; i < TrackNotes[2].size(); ++i)
		{

			//if i is out of range for either of our vectors, we simply add the rest of the other one. If both are out of range, we stop the copy (because it is finished)
			/*
			if (i >= TrackNotes[0].size())
			{
				if (i >= TrackNotes[1].size())
				{
					break;
				}
				TrackNotes[2].push_back(*(TrackNotes[1].begin() + i));
				continue;

			}
			else if (i >= TrackNotes[1].size())
			{
				TrackNotes[2].push_back(*(TrackNotes[0].begin() + i));
				continue;
			}
			*/

			//if current note start value is before the last note has ended
			if ((TrackNotes[2].begin() + i)->x < ((TrackNotes[2].begin() + i - 1)->x + (TrackNotes[2].begin() + i - 1)->length))
			{

				//any event notes need to be dealt with.
				//they need to be linked with their parent note so the program can tell which event notes need to be retained
				//otherwise we will get dynamics that don't match the original track (this is done in the logic above)
				if ((TrackNotes[2].begin() + i)->y == 0xFF &&
					(TrackNotes[2].begin() + i)->eventHasParent == true //event notes without parents will be maintained and copied per normal (should I automatically make them lowest-priority, though?)
					)
				{

					bool foundParentNote = false;
					//run back through notes until we find one that matches the parent 
					for (int reChecki = i; reChecki >= 0; --reChecki)
					{

						//if note is NOT an event AND it's start value is the same as our event's parent AND it came from the same original track
						//since this note has already been processed, all of its events are guaranteed safety
						//even if the priority track has events in the way of non-prio notes, those will be deleted in favor of actual notes
						if ((TrackNotes[2].begin() + reChecki)->y != 0xFF &&
							(TrackNotes[2].begin() + reChecki)->x == (TrackNotes[2].begin() + i)->eventParent &&
							(TrackNotes[2].begin() + reChecki)->trackPrio == (TrackNotes[2].begin() + i)->trackPrio

							)
						{
							//event will not be deleted
							foundParentNote = true;
							break;

						}


					}

					//delete event if no parent found
					if (foundParentNote == false)
					{
						TrackNotes[2].erase((TrackNotes[2].begin() + i));//erase the current note
					}




				}


				//if the priority number of the current note is less than that of the previous note
				//note came from track 0 and other came from track 1
				else if ((TrackNotes[2].begin() + i)->trackPrio < (TrackNotes[2].begin() + i - 1)->trackPrio)
				{
					//changes what note is erased based on what the user entered (1 gives top prio to the smaller, 2 gives top prio to the bigger)
					//this could probably be optimized. oh, well.
					if (PrioFile == 2 ||
						((TrackNotes[2].begin() + i)->y == 0xFF)//always puts non-note orphan events in the non-priority state
						)
					{
						TrackNotes[2].erase((TrackNotes[2].begin() + i));//erase the current note
					}
					else
					{
						TrackNotes[2].erase((TrackNotes[2].begin() + i - 1));//erase the other note
					}

					//start-overs will only happen in the case of normal notes
					i = 1;//start over (not starting over may give us skip-over errors)

				}
				else//the current priority number is bigger (opposite the logic above)
				{
					if (PrioFile == 2 ||
						!((TrackNotes[2].begin() + i)->y == 0xFF)//always puts non-note orphan events in the non-priority state
						)
					{
						TrackNotes[2].erase((TrackNotes[2].begin() + i - 1));//erase the other note
					}
					else
					{
						TrackNotes[2].erase((TrackNotes[2].begin() + i));//erase the current note
					}

					i = 1;//start over (not starting over may give us skip-over errors)

				}

			}


			//(TrackNotes[2].begin() + i)

			//TrackNotes[2].push_back(*(TrackNotes[0].begin() + i));


		}





		//finish writing out

		//finish the header
		for (unsigned int i = TrackPaste; i < MAXTRACK; ++i)
		{
			if (i > TrackPaste)
			{
				fwrite(memfile[3], 6, 1, file);//write the headers of everything past the target track
			}
			else
			{
				fwrite(memfile[2], 4, 1, file);//writes all except note count
				File_WriteLE16(TrackNotes[2].size(), file);//writes the new note count
			}

			memfile[3] += 6;//advance to next track

		}


		//write all the notes up to the special track
		for (unsigned int i = 0; i < TrackPaste; ++i)
		{
			fwrite(memfile[3], (8 * orgs[1].tracks[i].note_num), 1, file);//write the notes for this track

			memfile[3] += (8 * orgs[1].tracks[i].note_num);//move pointer forward

		}



		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE32((TrackNotes[2].begin() + i)->x, file);
		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE8((TrackNotes[2].begin() + i)->y, file);
		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE8((TrackNotes[2].begin() + i)->length, file);
		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE8((TrackNotes[2].begin() + i)->volume, file);
		for (int i = 0; i < TrackNotes[2].size(); ++i)
			File_WriteLE8((TrackNotes[2].begin() + i)->pan, file);



		//memfile[1] += (orgs[1].tracks[TrackPaste].note_num * 8);//move to the end of the data chunk (recall that the memory file's data didn't change with the operations above) (also recall that in reading from this pointer, we already moved it forward)

		for (int i = TrackPaste + 1; i < MAXTRACK; ++i)//iterate through the rest of the unwritten notes
		{

			fwrite(memfile[1], (8 * orgs[1].tracks[i].note_num), 1, file);//write the notes for this track			
			memfile[1] += (8 * orgs[1].tracks[i].note_num);//move pointer forward
		}




	}
	else//traditional copy method
	{

		//copy header data for the second memfile only (because the operation also writes out, and we need to already know the trackdata for the copied file)
		for (int i = 0; i < MAXTRACK; ++i)
		{
			if (i == TrackPaste)
			{
				fwrite(memfile[0], 6, 1, file);//write the contents of the first file instead
			}
			else
			{
				fwrite(memfile[1], 6, 1, file);//write the track info from the 2nd file
			}


			orgs[1].tracks[i].note_num = (memfile[1][5] << 8) | memfile[1][4];//read the note ammount of this track
			memfile[1] += 6;//advance to next track

		}
		memfile[0] += ((size_t)(MAXTRACK - TrackCopy) * 6);//iterate to the end of the header


		//send the memfile to the start of the track to be copied (recall that memfile0 is the one we read ONLY)
		for (unsigned int i = 0; i < TrackCopy; ++i)
		{
			memfile[0] += (orgs[0].tracks[i].note_num * 8);
		}



		//copy note data
		for (int i = 0; i < MAXTRACK; ++i)
		{
			if (i == TrackPaste)
			{

				fwrite(memfile[0], (8 * orgs[0].tracks[TrackCopy].note_num), 1, file);//write the notes for this track
			}
			else
			{
				fwrite(memfile[1], (8 * orgs[1].tracks[i].note_num), 1, file);//write the notes for this track
			}

			memfile[1] += (8 * orgs[1].tracks[i].note_num);//move pointer forward

		}

		//load file 2 to memory,
		//stream file 1, find track data
		//rebuild file 2 from memory
		//unsigned char* cutmemfile = DeleteByteSection(memfile, file_size_2, 0, 0);



		//WriteFileFromMemory(Path2.c_str(), memfile[1], file_size_2, "wb");
	}

	fclose(file);
	

	return true;

}



//exposes the copy engine to the window enviroment
void HandleOrgCopyBackend(void)
{

	//collect the most up-to-date org info
	if (strcmp(OrgCopyParams.track1Path, OrgCopyParams.track1PathOLD))
	{
		strcpy(OrgCopyParams.track1PathOLD, OrgCopyParams.track1Path);//make the old path congruent
		orgs[0] = GetOrgInfo(OrgCopyParams.track1Path);//update the info about the ORG
	}

	if (strcmp(OrgCopyParams.track2Path, OrgCopyParams.track2PathOLD))
	{
		strcpy(OrgCopyParams.track2PathOLD, OrgCopyParams.track2Path);//make the old path congruent
		orgs[1] = GetOrgInfo(OrgCopyParams.track2Path);//update the info about the ORG
	}


	if (OrgCopyParams.BeginCopy)
	{
		OrgCopyParams.BeginCopy = false;

		//an array of bools to detect if any of the copy operations failed
		bool JobSuccess[MAX_OPERATIONS]{};


		for (int i = 0; i < MAX_OPERATIONS; ++i)
		{
			//only copy from one to the other if the job says "yes"
			if (OrgCopyParams.OperationList[i].PerformOperation)
			{
				//CopyOrgData takes std-type strings, so we need to convert these
				std::string Path1 = OrgCopyParams.track1Path;
				std::string Path2 = OrgCopyParams.track2Path;

				JobSuccess[i] = CopyOrgData(Path1, Path2,
					OrgCopyParams.OperationList[i].CopyFrom,
					OrgCopyParams.OperationList[i].CopyTo,
					OrgCopyParams.OperationList[i].UseTrackMash,
					OrgCopyParams.OperationList[i].MashPrioFile);

			}
			else
			{
				JobSuccess[i] = true;
			}
			

		}

		for (int i = 0; i < MAX_OPERATIONS; ++i)
		{
			if (JobSuccess[i] == false)
			{
				OrgCopyParams.Success = false;//something went wrong
				break;//no need to check anymore, we found an issue
			}
			else
			{
				OrgCopyParams.Success = true;
			}
		}


		//update org info
		orgs[0] = GetOrgInfo(OrgCopyParams.track1Path);
		orgs[1] = GetOrgInfo(OrgCopyParams.track2Path);

		OrgCopyParams.FinishedCopy = true;//done

	}



}





