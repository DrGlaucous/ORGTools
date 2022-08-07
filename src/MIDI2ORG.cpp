// To convert binary MIDIs to Organya files
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>
#if __cplusplus < 201703L // If the version of C++ is less than 17
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING//silence. 
#include <experimental/filesystem>
	// It was still in the experimental:: namespace
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "Main.h"
#include "WindowView.h"//needed to update the window while MIDI2ORG is running
#include "SharedUtil.h"
#include "MIDI2ORG.h"
#include "File.h"
#include "Midi.h"


//already defined elsewhere
TRACKINFO Mtracks[MAXTRACK];
//ORGFILES orgs[2]; (not actually needed)

MIDI2ORGOPTIONS MidiConvertParams{};



static unsigned short gResolution{};

//sorts by ascending note X value (TS is TimeStart)
bool SortFunctionTS(ORGNOTEDATA one, ORGNOTEDATA two)
{
	return (one.TimeStart < two.TimeStart);//in correct position if the earlier positioned one is less than the later positioned one

}
//sorts by start/stop values
bool SortFunctionEvT(MIDI_NOTEDATA one, MIDI_NOTEDATA two)
{

	if (one.TimeStart < two.TimeStart)
	{
		return true;
	}
	else if (one.TimeStart == two.TimeStart &&//puts all stop commands before the start commands
		one.Status < two.Status
		)
	{
		return true;
	}
	else
	{
		return false;
	}

}


//keep updating the menu while processing
void ExportStatus(const char* desc, ...)
{

	//extra arguments in the function
	va_list args;
	va_start(args, desc);//pair arguments with this string

	//we are using this huge constant size buffer to see how big to make our terminal text string
	//since we can't just compute the size of va_list without poping its values
	char FormatBuffer[PATH_LENGTH]{};

	int w = vsnprintf(FormatBuffer, PATH_LENGTH, desc, args);

	va_end(args);

	//we have nothing for if the condition is NULL...

	//figure out where to put the null end terminator
	if (w == -1 || w >= PATH_LENGTH)
		w = PATH_LENGTH - 1;
	FormatBuffer[w] = 0;




	//we need to deterine how big this string is going to be with the va_list args
	int BufferSize = (strlen(FormatBuffer) + 1);

	//no need for this anymore. we do the value baking in this function now
	//MidiConvertParams.TerminalArgs = args;

	//allocate a char array the size of the text we got and send the terminaltext pointer there.
	char* SecondFormatBuffer = (char*)malloc(BufferSize);


	if (SecondFormatBuffer != NULL)
	{
		//make allocated memory chunk all null terminator
		memset(SecondFormatBuffer, 0, strlen(desc) + 1);

		//copy our text to the buffer
		strcpy(SecondFormatBuffer, FormatBuffer);
	}

	//move data we gleaned into the terminal's memory
	//(this function only updates the memory, it does not actually display the terminal)
	WriteToTerminal(SecondFormatBuffer);

	//run top function 1x per millisecond value specified here.
	//larger values mean faster processing times but less feedback
	RunTopSparingly(20);


}


bool ConvertMidi(MIDICONV inOptions)
{

	MIDI_NOTEDATA PrepNote;
	std::string DirectoryPath = inOptions.Path + std::string(".fold");

	if (!(fs::is_directory(DirectoryPath.c_str())))//checks to see if the directory exists, skips making it if this is true
	{
		if (!(fs::create_directories(DirectoryPath.c_str())))
		{

			ExportStatus("Error! Could not create directory:\n%d", DirectoryPath.c_str());

			//std::cout << "Error! Could not create directory:\n"
			//	<< DirectoryPath.c_str()
			//	<< std::endl;


			return false;
		}
	}

	//smf::MidiFile MIDIFILE;

	//MIDIFILE.read(inOptions.Path);

	Midi MIDIFile{ inOptions.Path };

	auto& header = MIDIFile.getHeader();
	auto& Mtracks = MIDIFile.getTracks();
	//channel

	ExportStatus("Track Count: %d", header.getNTracks());
	//std::cout << "Track Count: " << header.getNTracks() << std::endl;



	//Tempo conversion formulae used by ORGmaker2:
	//ltempo = 60 * 1000 / (info.wait * info.dot * iDeltaTime);
	//ORGTempo = 60 * 1000 / "Wait" perameter * notes in each beat * ???)
	// 
	// 
	//invtempo = 60 * 1000000 / (60 * 1000 / (info.wait * info.dot ));
	//MIDI tempo = 60 * microseconds / ORGTempo
	//ORGtempo == just normal tempo (BPM)
	//BPM = 60000 / "X" * Notes in each beat
	//


	unsigned char BeatsPM = 4;//Beats per measure (4 by default if no MIDI time signaute events are encountered), will be shared between tracks
	unsigned int Tempo = 120;//default MIDI tempo is 120 BMP, or 500000 microseconds per 1/4 note, will be shared between tracks
	bool TempoSet = false;//if we encounter multiple tempo metadata in a piece of music, this will make sure only the first tempo event sets the tempo for the whole conversion

	gResolution = header.getDivision();//this may have 1 of 2 alternate resolution formats: time in 1/4 note, or something based on framerate (I have not accounted for the second format, and it may produce adverse results)

	ExportStatus("Resolution:  %d", gResolution);
	//std::cout << "Resolution: " << gResolution << std::endl;

	int currTrack = 0;//use this to know what track we are looking at
	for (const auto& track : Mtracks)//iterates through all tracks
	{

		unsigned int AbsTime = 0;//absolute time since the start of the track
		std::vector<MIDI_NOTEDATA> TrackData[MAXCHANNEL];//one vector for each channel (will be reused per each track)

		auto& events = track.getEvents();

		for (const auto& trackEvent : events)//iterates through all track events
		{
			auto* event = trackEvent.getEvent();
			//uint8_t* data;

			if (event->getType() == MidiType::EventType::MidiEvent)//I believe metaEvents contain the tempo and key signature info
			{
				auto* midiEvent = (MidiEvent*)event;
				auto status = midiEvent->getStatus();

				/*
				switch (status)
				{
				case MidiType::MidiMessageStatus::NoteOn:
				case MidiType::MidiMessageStatus::NoteOff:
				}
				*/

				if (status == MidiType::MidiMessageStatus::NoteOn)
				{

					//velocity == loudness (0 is note Off)
					//channel == location (we will have 1 folder for each track, and 1 org for each used channel)
					//frequency == note pitch
					PrepNote.Volume = (char)(midiEvent->getVelocity());
					PrepNote.Pitch = midiEvent->getNote();
					PrepNote.Channel = midiEvent->getChannel();
					PrepNote.Status = (midiEvent->getVelocity() ? true : false);
					PrepNote.TimeRelative = trackEvent.getDeltaTime().getData();
					AbsTime += PrepNote.TimeRelative;
					PrepNote.TimeStart = AbsTime;
					TrackData[PrepNote.Channel].push_back(PrepNote);//record the data to our vector


					ExportStatus("Loudness: %d\nFrequency: %d\nChannel: %d\n\t\t\tLength: %d\n\t\tABSLength: %d",
						(int)PrepNote.Volume, (int)PrepNote.Pitch, (int)PrepNote.Channel, PrepNote.TimeRelative, AbsTime);

					//std::cout << "Loudness " << (int)PrepNote.Volume << std::endl;
					//std::cout << "Frequency " << (int)PrepNote.Pitch << std::endl;
					//std::cout << "Channel " << (int)PrepNote.Channel;

					//std::cout << "\t\t\tLength: " << PrepNote.TimeRelative
					//	<< "\t\tABSLength: " << AbsTime << std::endl;


				}
				else if (status == MidiType::MidiMessageStatus::NoteOff)
				{
					PrepNote.Volume = (char)(midiEvent->getVelocity());
					PrepNote.Pitch = midiEvent->getNote();
					PrepNote.Channel = midiEvent->getChannel();
					PrepNote.Status = false;
					PrepNote.TimeRelative = trackEvent.getDeltaTime().getData();
					AbsTime += PrepNote.TimeRelative;
					PrepNote.TimeStart = AbsTime;
					TrackData[PrepNote.Channel].push_back(PrepNote);//record the data to our vector

					ExportStatus("\t\t\tLength: % d\n\t\tABSLength: % d",
						PrepNote.TimeRelative, AbsTime
						);
					//std::cout << "\t\t\tLength: " << PrepNote.TimeRelative << std::endl;
					//std::cout << "\t\t\tABSLength: " << AbsTime << std::endl;


				}
				else
				{
					int TimeRelative = trackEvent.getDeltaTime().getData();
					AbsTime += TimeRelative;

					ExportStatus("\t\t\t(MIDI) Length: % d\n\t\tABSLength: % d",
						TimeRelative, AbsTime
						);

					//std::cout << "\t\t\t(MIDI) Length: " << TimeRelative
					//	<< "\n\t\t\tABSLength: " << AbsTime
					//	<< std::endl;
				}


			}
			else if (event->getType() == MidiType::EventType::MetaEvent)
			{

				auto* metaEvent = (MetaEvent*)event;
				auto status = metaEvent->getStatus();

				uint8_t* data = ((MetaEvent*)event)->getData();

				if (status == MidiType::MetaMessageStatus::TimeSignature)//cache the BPM (in a later revision, we may need to see WHEN this happens, so we can process midis with multiple time signatures)
				{
					//I may also want to put a boolean check in here so that this is not reset more than 1x, so the first time signature is the one that is always kept
					BeatsPM = data[0];//if it is a time signature event, we already know that the first chunk of data will be the numerator, or BPM
					
					ExportStatus("\t\tTime signature event: Numerator = %d", (int)BeatsPM);
					//std::cout << "\t\tTime signature event: Numerator = " << (int)BeatsPM << std::endl;
				}
				else if (status == MidiType::MetaMessageStatus::SetTempo)
				{
					if (!TempoSet)
					{
						Tempo = 60000000 / ((0 << 24 | data[0] << 16) | (data[1] << 8) | data[2]);//tempo events only have 3 bits of data (in BIG_ENDIAN format), so the first bit needs to be filled with 0
						TempoSet = true;
						
						ExportStatus("\t\tTempo event: New Tempo = %d", Tempo);
						//std::cout << "\t\tTempo event: New Tempo = " << Tempo << std::endl;
						
					}
					else
					{
						ExportStatus("\t\tTempo event: Cannot change the tempo. Already set to: %d", Tempo);
						//std::cout << "\t\tTempo event: Cannot change the tempo. Already set to " << Tempo << std::endl;
					}

				}



				//write out the event to the terminal


				ExportStatus("\t\tMeta event:\n\t\t\tStatus: 0x%x\n\t\t\tData: 0x", ((MetaEvent*)event)->getStatus());


				//std::cout << "\t\tMeta event:" << std::endl
				//	<< "\t\t\tStatus: 0x" << std::hex
				//	<< ((MetaEvent*)event)->getStatus() << std::endl
				//	<< "\t\t\tData: 0x";
				for (int i{ 0 }; i < ((MetaEvent*)event)->getLength(); ++i) {

					ExportStatus("%x -", (int)data[i]);

					//std::cout << (int)data[i] << '-';
				}
				if (!((MetaEvent*)event)->getLength()) {

					ExportStatus("0");

					//std::cout << "0";
				}
				//std::cout << std::dec << std::endl;




			}
			else
			{
				int TimeRelative = trackEvent.getDeltaTime().getData();
				AbsTime += TimeRelative;
				
				ExportStatus("\t\t\t(OTHER) Length: %d\n\t\t\tABSLength: %d", TimeRelative, AbsTime);
				
				//std::cout << "\t\t\t(OTHER) Length: " << TimeRelative
				//	<< "\n\t\t\tABSLength: " << AbsTime
				//	<< std::endl;
			}


		}


		int reductionRate = 1;
		//user inputs a note to make worth 1 tick
		if (inOptions.ForceSimplify)
		{

			if (inOptions.SimplestNote > gResolution)
			{

				ExportStatus("WARNING: Most Prescise Note is less than entered value: No reduction will happen.");

				//std::cout << "WARNING: Most Prescise Note is less than entered value: No reduction will happen." << std::endl;
			}
			else
			{
				//int reducFactor  = inOptions.SimplestNote / 4;
				reductionRate = gResolution / (inOptions.SimplestNote / 4);//if we still did this operation, a large enough number would yield a 0, which is *very* bad: we devide by it multiple times
			}




			//gResolution;//length of 1/4 note
			//gResolution/2 == length of 1/8 note

		}
		else
		{
			//Auto-shrink
			//
			std::vector<int> ArrayOfLength;//will be used to find the gcd of all the MIDI events (so we can hopefully shrink those values some)
			//see if the file can be reduced
			for (int i = 0; i < MAXCHANNEL; ++i)//for each vector in the array
			{

				//catch any 0 length arrays sent our way
				if (TrackData[i].size() == 0)
				{
					continue;
				}

				//int ZeroValue = (TrackData[i].begin()->TimeStart) - (TrackData[i].begin()->TimeRelative);//in case we had to use the absolute time, this allows the start value to be anything (but we can just use relative time)
				for (int j = 0; j < TrackData[i].size(); ++j)//for each event recorded in the track
				{

					if (((TrackData[i].begin() + j)->TimeRelative) == 0)
					{
						continue;//disregard any 0 addresses
					}

					ArrayOfLength.push_back(((TrackData[i].begin() + j)->TimeRelative));//add the event times to the vector
				}



			}

			reductionRate = gcdArray(ArrayOfLength);
			if (reductionRate > gResolution)//since our resolution is based on 1/4 notes, any shrinkage past this (like whole notes) will be stopped (it causes a devide by 0 error further down the code)
				reductionRate = gResolution;


			ExportStatus("Auto-Shrinkable by: %d", reductionRate);
			//std::cout << "Auto-Shrinkable by: " << reductionRate << std::endl;//what space can we save?

		}




		//force compression: get an input as to how much we want the song crunched down
		//This is good for cases where a few little notes will balloon up the entire track, or if the midi was created
		//with imperfect locations. 
		//deviding an integer will produce an integer + remainder,
		//we will round the notes forward or backward depending on how much is left in said remainder (big values will round up, small ones down)

		//compression here
		for (int i = 0; i < MAXCHANNEL; ++i)//for each vector in the array
		{

			if (TrackData[i].size() == 0)//skips the track if there are no note events in there
			{
				continue;
			}
			else if (TrackData[i].size() > 1)
			{
				//i moved this here becasue the sorting was originally applied after the notes had been compressed (creating many more 0-length start-stop pairs than there should be).
				//this resulted in some incorrect start-stop re-ordering and inverted note playing
				std::sort(TrackData[i].begin(), TrackData[i].end(), SortFunctionEvT);//organize the out-of-order 0 length events (puts all OFF events before ON events)
			}


			for (int j = 0; j < TrackData[i].size(); ++j)//for each event recorded in the track
			{

				//round up logic
				int remainderBoost{};
				if (((TrackData[i].begin() + j)->TimeRelative) % reductionRate > (reductionRate / 2))
				{
					remainderBoost = 1;
				}
				(TrackData[i].begin() + j)->TimeRelative = (((TrackData[i].begin() + j)->TimeRelative) / reductionRate) + remainderBoost;//compress addresses


				//re-applying this logic here may be redundant
				remainderBoost = 0;
				if (((TrackData[i].begin() + j)->TimeStart) % reductionRate > (reductionRate / 2))
				{
					remainderBoost = 1;
				}
				(TrackData[i].begin() + j)->TimeStart = ((TrackData[i].begin() + j)->TimeStart) / reductionRate + remainderBoost;



			}
		}




		//this is where we need to put the data from our vector into a file
		std::string TrackSubfolder = DirectoryPath + std::string("/Track") + std::to_string(currTrack) + ".fold";


		if (!(fs::is_directory(TrackSubfolder.c_str())))//checks to see if the directory exists, skips making it if this is true
		{
			//creates the sub-folder directory inside the parent
			if (!(fs::create_directories(TrackSubfolder.c_str())))
			{
				ExportStatus("Error! Could not create directory:\n %s", TrackSubfolder.c_str());

				//std::cout << "Error! Could not create directory:\n"
				//	<< TrackSubfolder.c_str()
				//	<< std::endl;
				return false;
			}
		}


		for (int i = 0; i < MAXCHANNEL; ++i)//go through all channels of notes (one ORG file will be made per each channel)
		{
			if (TrackData[i].size() == 0)//skips the track if there are no note events in there
			{
				continue;
			}



			std::vector<ORGNOTEDATA> TrackDataOrg[MAXTRACK];//One vector per each ORG track (used to bump notes to the next track if the current note is still active)
			ORGNOTEDATA BufferNote;


			//std::sort by relative time and note status (for the entire TrackData[i])
			//with midi vehicle.mid, track 1, channel 0, note 45 and note 46 are flopped (use this for reference)

			//converts start/stop values into note X + length (and sorts multiple simultaneous notes into different tracks)
			for (int z = 0; z < TrackData[i].size(); ++z)//for every MIDI note event
			{

				//middle C in MIDI is 60, in ORG it is 48, a difference of 12
				//ORG range goes from 0 to 95, while the MIDI range is up to 127
				if ((TrackData[i].begin() + z)->Pitch < 12)//bump deep notes up by 1 octave
				{
					BufferNote.Pitch = (TrackData[i].begin() + z)->Pitch;
				}
				else if ((TrackData[i].begin() + z)->Pitch > 107)//bring high notes down by however much needed: 95 is the upper ORG limit, plus the 12 we knock it down by = 107
				{
					//knock it down by 12 + 12*the number of higher octaves it is 
					BufferNote.Pitch = (TrackData[i].begin() + z)->Pitch -
						((((TrackData[i].begin() + z)->Pitch - 96) / 12) * 12 + 12);//the /12 * 12 seems redundant, but it groups the number by 12s (because ints can't be decimals, so they only take the value of the 1st digit, no rounding)



				}
				else
				{
					BufferNote.Pitch = (TrackData[i].begin() + z)->Pitch - 12;//middle C in MIDI is 60, in ORG it is 48, a difference of 12 (one octave down)
				}
				BufferNote.TimeStart = (TrackData[i].begin() + z)->TimeStart;//no conversions needed here
				BufferNote.Volume = (char)ValueMap(0, 127, 4, 252, (TrackData[i].begin() + z)->Volume);//map volume from MIDI format to ORG format
				BufferNote.Length = 1;//in case the MIDI never tells this note to stop, it will still have a termination point
				BufferNote.LengthSet = false;//tells us if we need to move on to the next track or not (true == good to add another note behind it)
				BufferNote.EventType = (TrackData[i].begin() + z)->Status;//start/stop

				//iterate through the ORG tracks to ask the questions below
				for (int j = 0; j < MAXTRACK; ++j)
				{

					//see the MIDI event's status: if it is an OFF note event, check and see what track has that note as the last one played
					//we may be pushing back multiple OFF events

					if (TrackDataOrg[j].size() == 0)//automatically insert the buffer since there is nothing before it
					{
						if (BufferNote.EventType == true)//I've run across some MIDIs that have NOTE OFF events that are NOT tied to any NOTE ON events. That was very poor of the MIDI, but we still need to be able to handle it.
						{
							TrackDataOrg[j].push_back(BufferNote);//only append it if it is a note.start command
							break;//get out of this for loop: we gave the data a home.
						}
						break;//get out of this for loop: we gave the data a home.
						//do we need continue; if the note is a note.stop command? if the conditions here are satisfied, I don't think any of the higher tracks can be populated

					}



					//we need to put this one above the condition seen above (because if we are playing 2 of the same notes, we want them to be separate)
					//we also need to check if the input is a start function before doing this \/
					if ((TrackDataOrg[j].end() - 1)->LengthSet == true &&//other note is done
						BufferNote.EventType == true &&//this one is a note.start command
						((TrackDataOrg[j].end() - 1)->Length + (TrackDataOrg[j].end() - 1)->TimeStart) <= BufferNote.TimeStart//a condition that can occur when notes are snapped from length 0 to length 1 (see the function below)
						)//check to see if the previous note has stopped playing
					{
						TrackDataOrg[j].push_back(BufferNote);//we are allowed to push the next note back
						break;//data applied successfully

					}


					//the only issue I see here is if we get two note.start commands for the same note without any note.stops in between (this is not likely to happen, but I think this assumption may come back to bite me)
					//it DID come back to bite me: I found several midis that fed the program duplicate note.start events and royally screwed it over
					//no need to check if the type is a startNote because all conditions where that would be true are caught in the statement above
					if ((TrackDataOrg[j].end() - 1)->Pitch == BufferNote.Pitch)//are this and last note the same
					{

						//check for duplicate note.on events (same pitch and Time.Start notes will be discarded) (NON-duplicate Time.Start notes will be handled further down below.)						
						if ((TrackDataOrg[j].end() - 1)->TimeStart == BufferNote.TimeStart &&
							BufferNote.EventType == true//also make sure that the note is indeed note.ON in type
							)
							break;

						(TrackDataOrg[j].end() - 1)->Length = (BufferNote.TimeStart) - ((TrackDataOrg[j].end() - 1)->TimeStart);//delta time
						(TrackDataOrg[j].end() - 1)->LengthSet = true;//note is complete

						//break-up function
						if ((TrackDataOrg[j].end() - 1)->Length > 255)//ORG notes can only be 1 char long, so any longer than that will need to be cut up into multiple notes
						{

							BufferNote = *(TrackDataOrg[j].end() - 1);//copy the last note to the buffer
							BufferNote.Length = 255;

							int OfTotalLength = (TrackDataOrg[j].end() - 1)->Length;//used to see how much of the total note length we have left

							TrackDataOrg[j].pop_back();//remove the note whose data is too long

							while (OfTotalLength > 255)//keep pushing back clones of the note (but only 255 in length) until our total length is less than 1 char
							{

								TrackDataOrg[j].push_back(BufferNote);

								BufferNote.TimeStart += 255;
								OfTotalLength -= 255;
							}
							BufferNote.Length = OfTotalLength;
							TrackDataOrg[j].push_back(BufferNote);

						}


						//this can happen if the force-simplify function rounds both the start and stop markers of a note to the same address
						if ((TrackDataOrg[j].end() - 1)->Length == 0)
						{
							(TrackDataOrg[j].end() - 1)->Length = 1;//mimimum value
						}

						//that off-chance the MIDI is being bad and sends 2 note.start commands our way, we cap the last note and begin the new one
						if (BufferNote.EventType == true)
						{
							TrackDataOrg[j].push_back(BufferNote);
						}

						break;//data applied successfully

					}

				}

				//if we get here and were not able to find a space, the note is (unfortunately) discarded

			//I have a habit of taking notes right inside the code. Here is a list of questions asked by the logic above:
			// check: is previous note the same?
			// if no, check if the previous note's deltaTime is set
			// if no, bump to the next track and repeat
			// 
			// if the note IS the same, regardless of status, we can slap it on (it will simply change the note already on the board)
			// to slap it on, take deltaTime between the notes and set the previous note's deltaTime variable
			// 
			//process: send note data

			}





			//handle drum track parsing
			if (inOptions.HasDrumChannel == true &&
				i == inOptions.DrumChannel)
			{
				//the goal of this function is to separate all notes by pitch

				std::vector<ORGNOTEDATA> TrackDrumOrg[MAXTRACK];//we will be copying the processed data 

				bool PushedBack = false;//use this to ratchet DrumTrackNumber x1 if things were pushed back on that note[u]
				int DrumTrackNumber = 0;//increment this each time we find a populated note (currently can handle 16 different precussion insturments, but this may need changed in order to handle more)
				//iterate through all values in a char (max possible values in an ORG's pitch scale)
				for (int u = 0; u < 256; ++u)
				{
					//iterate thorugh all tracks
					for (int z = 0; z < MAXTRACK; ++z)
					{
						//skip channels with 0 notes
						if (TrackDataOrg[z].size() == 0)
							continue;

						//go through all the notes and push back any that match the pitch
						for (int w = 0; w < TrackDataOrg[z].size(); ++w)
						{
							if ((TrackDataOrg[z].begin() + w)->Pitch == u)
							{
								PushedBack = true;
								TrackDrumOrg[DrumTrackNumber].push_back(*(TrackDataOrg[z].begin() + w));
							}
						}


					}

					if (PushedBack)
					{
						PushedBack = false;
						if (++DrumTrackNumber >= MAXTRACK)
						{
							break;//end process if we've run out of avalible slots
						}
					}

				}

				//At this point, TrackDrumOrg is organized by pitch: all notes with the same pitch are in the same slot
				//we need to sort them by start time (we don't need to worry about total length because they are all the same note)
				for (int u = 0; u < MAXTRACK; ++u)
				{
					if (TrackDrumOrg[u].size() == 0)
						continue;

					std::sort(TrackDrumOrg[u].begin(), TrackDrumOrg[u].end(), SortFunctionTS);//sort by X value

				}


				//copy our new drum track to the Org vector to be written out
				for (int u = 0; u < MAXTRACK; ++u)
				{
					TrackDataOrg[u].clear();
					TrackDataOrg[u] = TrackDrumOrg[u];

				}




			}




			//time to make the org file

			std::string ORGPath = TrackSubfolder + std::string("/Channel") + std::to_string(i) + ".org";

			FILE* outFile = fopen(ORGPath.c_str(), "wb");

			if (outFile == NULL)
			{

				ExportStatus("Error! Could not create file:\n %s", ORGPath.c_str());

				//std::cout << "Error! Could not create file:\n"
				//	<< ORGPath.c_str()
				//	<< std::endl;
				fclose(outFile);
				return false;
			}

			//setup header data

			//BeatsPM can be found at the top of the file (it is set by a MIDI time signature event)

			//gResolution is the length of a 1/4 note
			//NotesPB used to ba a char, but MIDI resolution can be much larger than this. this causes some undefined behavior if we cast it down to a char
			int NotesPB = (gResolution / reductionRate);//this will never become 0 (the reduction rate is never larger than the resolution)
			unsigned char ReducedNotesPB{};//we will devide the value above until it is less than 1 char, so we can append it to the ORG (and maybe have a multiple of the original notes per beat?)

			//there is no need to correctly adjust the tempo to match the new ReducedNotesPB because tempo is based on per-note, not per-measure, so any measure size will work in it
			//ORGCopy will have trouble with combining tracks with such large notes per beat values, but these can be reduced in the ORG settings with OrgMaker2 (or manually with a HEX editor)
			//I feel like I am beating a dead horse though. If your values are big enough to require this logic, you should probably simplify the song
			//

			//deal with a NotesPB that could be larger than 255
			if (NotesPB > 255)
				ReducedNotesPB = 255;//0xFF is our limit
			else
				ReducedNotesPB = NotesPB;//we can just use it as-is


			fwrite("Org-02", 6, 1, outFile);
			//fwrite(&gResolution, 2, 1, outFile);
			File_WriteLE16((60000 / (Tempo * NotesPB)) ? (60000 / (Tempo * NotesPB)) : 1, outFile);//tempo: if the tempo is 0, we just use 1 (because that is as fast as the ORG player can go)
			File_WriteLE8(BeatsPM, outFile);//beats per measure
			File_WriteLE8(ReducedNotesPB, outFile);//notes per beat, max of 255		
			File_WriteLE32(0, outFile);//start of repeat loop
			File_WriteLE32((TrackData[i].end() - 1)->TimeStart, outFile);//end of repeat loop uses the last MIDI event to tell the ORG file where the repeat sign is
			for (int j = 0; j < MAXTRACK; ++j)//write note info
			{

				File_WriteLE16(1000, outFile);//insturment frequency, default is 1000
				File_WriteLE8(0, outFile);//insturment, will make them all 0
				File_WriteLE8(0, outFile);//Pipi, 0 by default
				File_WriteLE16((short)TrackDataOrg[j].size(), outFile);//number of notes in this track

			}

			for (int j = 0; j < MAXTRACK; ++j)//write the notes themselves (do it for each track)
			{

				//iterate through each note
				for (int z = 0; z < TrackDataOrg[j].size(); ++z)
					File_WriteLE32((TrackDataOrg[j].begin() + z)->TimeStart, outFile);//X value
				for (int z = 0; z < TrackDataOrg[j].size(); ++z)
					File_WriteLE8((TrackDataOrg[j].begin() + z)->Pitch, outFile);//Y location
				for (int z = 0; z < TrackDataOrg[j].size(); ++z)
					File_WriteLE8((unsigned char)(TrackDataOrg[j].begin() + z)->Length, outFile);//length (we need to greatly reduce the size of the org tempo for the actual length to fit here (I.E run the values through the gcm function to get the devisor))
				for (int z = 0; z < TrackDataOrg[j].size(); ++z)
					File_WriteLE8((TrackDataOrg[j].begin() + z)->Volume, outFile);//volume
				for (int z = 0; z < TrackDataOrg[j].size(); ++z)
					File_WriteLE8(6, outFile);//PAN (I need to grab this from the MIDI, too, kind of like the key signature)

			}


			fclose(outFile);

		}



		currTrack += 1;

	}





	return false;

}


void HandleMIDI2ORGBackend(void)
{
	//verify the file is a MIDI
	if (strcmp(MidiConvertParams.PathOLD, MidiConvertParams.BackendOptions.Path))
	{
		//make paths the same
		strcpy(MidiConvertParams.PathOLD, MidiConvertParams.BackendOptions.Path);

		//re-verify file
		MidiConvertParams.IsMIDI = VerifyFile_MIDI(MidiConvertParams.BackendOptions.Path);

	}


	if (MidiConvertParams.BeginCopy)
	{
		MidiConvertParams.BeginCopy = false;

		MidiConvertParams.RunningCopy = true;

		//actual conversion happens here (this function is boolean, but it doesn't actually return anything other than 0, so this reading isn't very helpful)
		MidiConvertParams.Success = ConvertMidi(MidiConvertParams.BackendOptions);

		MidiConvertParams.RunningCopy = false;
		MidiConvertParams.FinishedCopy = true;


	}






}





















