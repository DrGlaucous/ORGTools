#pragma once

#include "SharedUtil.h"

#define MAX_OPERATIONS 16//max number of copy operations we can do at one time

//TODO: remove and account for all instances of '{}', so the program will comply with an older C standard


//i might not actually need this enum...
enum TrackSelect
{
	Track1 = 0,
	Track2 = 1,
	Track3 = 2,
	Track4 = 3,
	Track5 = 4,
	Track6 = 5,
	Track7 = 6,
	Track8 = 7,
	TrackQ = 8,
	TrackW = 9,
	TrackE = 10,
	TrackR = 11,
	TrackT = 12,
	TrackY = 13,
	TrackU = 14,
	TrackI = 15,
	TrackCount = 16,


};


//for each copy operation to be done, this is set
typedef struct JobQuery
{
	bool PerformOperation{};//if true, then the program will perform one copy operation
	unsigned int CopyFrom{};//track to copy from
	unsigned int CopyTo{};
	bool UseTrackMash{};
	int MashPrioFile{};

}JobQuery;

//options passed from the GUI to the copy engine
typedef struct ORGCopyOPTIONS {
	bool track1Populated{};
	char track1Path[PATH_LENGTH]{};//should be long enough for most paths
	char track1PathOLD[PATH_LENGTH]{};//store old path in buffer so we can run some functions only if the path changes

	bool track2Populated{};
	char track2Path[PATH_LENGTH]{};
	char track2PathOLD[PATH_LENGTH]{};

	//list of copy jobs needed to be performed by ORGCopy
	JobQuery OperationList[MAX_OPERATIONS];

	//used to tell the backend to actually start
	bool BeginCopy{};
	//tells the window that the process finished
	bool FinishedCopy{};

	bool Success{};//did the copy engine perform the operation correctly?


} ORGCopyOPTIONS;


extern ORGCopyOPTIONS OrgCopyParams;
extern ORGFILES orgs[2];


void HandleOrgCopyBackend(void);