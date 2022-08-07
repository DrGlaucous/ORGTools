#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>//old string stuff
#include <nfd.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>//new string stuff

#include "../libs/imgui/imgui.h"
#include "../libs/imgui/imgui_impl_glfw.h"
#ifdef LEGACY_OPENGL
#include "../libs/imgui/imgui_impl_opengl2.h"
#else
#include "../libs/imgui/imgui_impl_opengl3.h"
#endif


#include "WindowView.h"
#include "ORGCopy.h"
#include "MIDI2ORG.h"
#include "Main.h"
#include "SharedUtil.h"
//I ripped most of this from DoConfig for CSE2 (cry about it)

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720


GLFWwindow* window = NULL;
GLFWmonitor* monitor = NULL;


const char* glsl_version = NULL;

int MainWindowWidth{};
int MainWindowHeight{};

float MonitorScaleWidth{};
float MonitorScaleHeight{};
float MonitorScaleAverage{};

char* Directory1{};
char* Directory2{};

TopBarOPTIONS TabOptions{};
//ORGCopyOPTIONS OrgCopySettings{}; //variable exists in OrgCopy.cpp

//boilerplate start and stop scripts
GLFWwindow* InitializeGLFW(const char* name)
{
	GLFWwindow* Lwindow;

	//botched solution
	if (!glfwInit())
		//return -1;
	{
	}

#ifdef LEGACY_OPENGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
	glsl_version = "#version 150 core";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	Lwindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, name, NULL, NULL);

	if (Lwindow != NULL)
	{
		glfwMakeContextCurrent(Lwindow);
		glfwSwapInterval(2);
		if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			return Lwindow;
		}



		return Lwindow;//oops: a potential instance where the function may not return anything
	}
	else
	{
		return Lwindow;
	}
}

int DestroyGLFW(GLFWwindow* Lwindow)
{
	glfwDestroyWindow(Lwindow);
	glfwTerminate();
	return 0;
}

int InitializeIMGUI(GLFWwindow* Lwindow)
{
	// Check if the platform supports the version of OpenGL we want
	#ifdef LEGACY_OPENGL
	if (GLAD_GL_VERSION_2_1)
	#else
	if (GLAD_GL_VERSION_3_2)
	#endif
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = NULL;	// Disable `imgui.ini`

		ImGui_ImplGlfw_InitForOpenGL(Lwindow, true);
	#ifdef LEGACY_OPENGL
		ImGui_ImplOpenGL2_Init();
	#else
		ImGui_ImplOpenGL3_Init(glsl_version);
	#endif

		return 0;
	}

	return 1;
}

int DestroyIMGUI(void)
{
#ifdef LEGACY_OPENGL
	ImGui_ImplOpenGL2_Shutdown();
#else
	ImGui_ImplOpenGL3_Shutdown();
#endif
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	return 0;
}

//set imgui's scale to that of the main system
void SetUpUI(void)
{
	//get user's UI scale
	monitor = glfwGetPrimaryMonitor();
	glfwGetMonitorContentScale(monitor, &MonitorScaleWidth, &MonitorScaleHeight);
	ImGuiIO& io = ImGui::GetIO();

	//average the values (they should be the same unless something funky is happenening)
	MonitorScaleAverage = (MonitorScaleWidth + MonitorScaleHeight / 2.0f);


	//I need to rebuild the font atlas so that it looks OK when upscaled.
	io.Fonts->AddFontDefault();
	io.FontGlobalScale = MonitorScaleAverage;
	//This seems to do nothing...
	io.Fonts->Build();

	//set up the default window style
	ImGuiStyle& style = ImGui::GetStyle();

	//start with the light color preset
	ImGui::StyleColorsLight(&style);

	//square frames
	style.WindowRounding = 0;





}


//ripped from imgui_demo.cpp
static void HelpMarker(const char* desc, ...)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		va_list args;
		va_start(args, desc);

		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		//ImGui::TextUnformatted(desc);
		ImGui::TextV(desc, args);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();


		va_end(args);
	}
}

//uses NativeFileDialog to choose files
char* OpenFile(void)
{
	char* outPath = NULL;
	nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

	int n = 1;
	return outPath;
}



//run the tool backends regardless of if its window is in focus or not
void RunBackends(void)
{
	//backend processes
	HandleOrgCopyBackend();
	HandleMIDI2ORGBackend();

}


//windows
void ShowTerminal(void)
{
	//keep a pointer array for showing terminal text
	static char* TerminalLines[TERMINAL_MAX_BUFFER]{};


	//static va_list TerminalLinesArguments[TERMINAL_MAX_BUFFER]{};
	
	bool SnapToBottom{};//set so we can lock and unlock scroll snapping to the bottom


	//if our buffer contents have changed
	if (TerminalLines[TERMINAL_MAX_BUFFER - 1] != MidiConvertParams.TerminalText
		)
	{
		//shifter function (move everything up in the array)
		for (int i = 1; i < TERMINAL_MAX_BUFFER; ++i)
		{
			//move the pointer into the next array position
			TerminalLines[i - 1] = TerminalLines[i];
			

		}


		//put new values in the top of the array
		TerminalLines[TERMINAL_MAX_BUFFER - 1] = MidiConvertParams.TerminalText;

		//tell the window to snap to the bottom again
		SnapToBottom = true;

	}

	//this should be active for only 1 frame before being set to false again by another function
	if (MidiConvertParams.FinishedCopy)
	{
		SnapToBottom = true;
	}



	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(100, 100, 100, 255));


	//oddly enough, imgui has a function to get the region width, but not the height, so we have to do that ourselves (I don't want to modify the imgui libraries for compatibility sake)
	int getWindowHeight = (ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);
	ImGui::BeginChild("TextWindow", ImVec2(ImGui::GetWindowContentRegionWidth() , getWindowHeight), true, ImGuiWindowFlags_None);


	for (int i = 0; i < TERMINAL_MAX_BUFFER; ++i)
	{
		//do not print any empty buffer lines
		if (TerminalLines[i] == NULL)
		{
			continue;
		}

		//print full ones
		ImGui::TextColored(TEXT_LIGHT_GREEN, TerminalLines[i]);



	}

	if (SnapToBottom)
	{
		SnapToBottom = false;
		//put the window focus on the newest text (drawn at the bottom)
		ImGui::SetScrollHereY(1);
	}


	ImGui::EndChild();


	ImGui::PopStyleColor();



}

void ShowOrgCopy(void)
{

	//layout:
	//	we need 2 file pickers (for each org) with status color that indicates if the file typed is an org
	//	status color button that opens an info menu for the ORG
	//		a chart that looks like this:
	//		copy from	|	copy to
	// 	   --------------------------------
	//		track 1		|	track 2		| TrackMASH [Y]
	//		track 2		|	track 4		| TrackMASH [N]
	//		[GO]
	//		copy to track colors will be red if the track is populated (a '?' symbol tells the note number currently in the track)




	//we are now running this in tabbed mode.
	//because of this, we no nonger need start() and end() functions
	 
	
	//TODO: revise where the menu starts when you click on it
	//ImVec2 menuSize(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	//ImVec2 center((ImGui::GetIO().DisplaySize.x * 0.5f) - (menuSize.x * 0.5f),
	//	(ImGui::GetIO().DisplaySize.y * 0.5f) - (menuSize.y * 0.5f)
	//);
	//ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver);
	//ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

	//give it an X button
	//if (!ImGui::Begin("ORGCopy", p_open))
	//{
	//	ImGui::End();
	//	return;
	//}

	//draw file pickers
	{
	ImGui::Text("File to be copied from:");
	ImGui::PushID(1);
	ImGui::InputTextWithHint("", "enter file path", OrgCopyParams.track1Path, IM_ARRAYSIZE(OrgCopyParams.track1Path));
	ImGui::PopID();


	ImGui::PushID(1);//unique ID (buttons with the same name need this)
	if (ImGui::Button("Browse"))
	{
		char* destination = OpenFile();
		if (destination != NULL)
			strcpy(OrgCopyParams.track1Path, destination);
	}
	ImGui::PopID();
	ImGui::SameLine();
	//0-1 Red, Green, Blue, Alpha
	if (orgs[0].IsOrg)
	{
		ImGui::TextColored(TEXT_GREEN, "ORG is GOOD");
		ImGui::SameLine();
		HelpMarker("The file header matches that of an ORG file.\nTotal notes: %d\nWait time: %d\nBeats Per Measure: %d\nNotes Per Beat: %d", 
			orgs[0].totalNotes, orgs[0].wait, orgs[0].bar, orgs[0].dot);
	}
	else
	{
		ImGui::TextColored(TEXT_RED, "ORG Not GOOD");//will turn colors based on if the file is detected as an org or not
		ImGui::SameLine();
		HelpMarker("ORGCopy did not recognize this file's header as ORG.\nAttempting operations on this file may corrupt it or crash the program!");
	}
	ImGui::Separator();


	ImGui::Text("File to be copied to:");
	ImGui::PushID(2);
	ImGui::InputTextWithHint("", "enter file path", OrgCopyParams.track2Path, IM_ARRAYSIZE(OrgCopyParams.track2Path));
	ImGui::PopID();

	ImGui::PushID(2);//unique ID
	if (ImGui::Button("Browse"))
	{
		char* destination = OpenFile();
		if (destination != NULL)
			strcpy(OrgCopyParams.track2Path, destination);
	}
	ImGui::PopID();

	ImGui::SameLine();
	//same exact logic as seen above
	if (strcmp(OrgCopyParams.track1Path, OrgCopyParams.track2Path) == 0 &&
		strlen(OrgCopyParams.track2Path) > 0
		)//strings match and aren't 0 length
	{
		ImGui::TextColored(TEXT_YELLOW, "Warning: Same Directory");
		ImGui::SameLine();
		HelpMarker("The destination ORG is the same as the copy-from org!\nYou can still complete the copy operation if this is what you want.");
	}
	else if (orgs[1].IsOrg)
	{
		ImGui::TextColored(TEXT_GREEN, "ORG is GOOD");
		ImGui::SameLine();
		HelpMarker("The file header matches that of an ORG file.\nTotal notes: %d\nWait time: %d\nBeats Per Measure: %d\nNotes Per Beat: %d",
			orgs[1].totalNotes, orgs[1].wait, orgs[1].bar, orgs[1].dot);
	}
	else
	{
		ImGui::TextColored(TEXT_RED, "ORG Not GOOD");//will turn colors based on if the file is detected as an org or not
		ImGui::SameLine();
		HelpMarker("ORGCopy did not recognize this file's header as ORG.\nAttempting operations on this file may corrupt it or crash the program!");
	}

	ImGui::Separator();
	}


	//table selection buttons, tell orgcopy how many operations you would like done
	ImGui::PushButtonRepeat(true);
	float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
	if (ImGui::Button("+"))
	{
		//scan through all operation slots until an empty one is found
		for (int i = 0; i < IM_ARRAYSIZE(OrgCopyParams.OperationList); ++i)
		{
			if (OrgCopyParams.OperationList[i].PerformOperation == false)
			{
				OrgCopyParams.OperationList[i].PerformOperation = true;
				break;
			}
		}


	}
	ImGui::SameLine();
	if (ImGui::Button("-"))
	{
		for (int i = IM_ARRAYSIZE(OrgCopyParams.OperationList) - 1; i >= 0; --i)
		{
			if (OrgCopyParams.OperationList[i].PerformOperation == true)
			{
				OrgCopyParams.OperationList[i].PerformOperation = false;
				break;
			}
		}
	}
	ImGui::PopButtonRepeat();
	ImGui::SameLine(0.0f, spacing);
	HelpMarker("Press the \"+\" button to add copy jobs to the queue,\n press the \"-\" button to take them away\nJobs will be performed in order from top to bottom");

	//tally total operation number
	int operationCnt{};
	for (int i = 0; i < IM_ARRAYSIZE(OrgCopyParams.OperationList); ++i)
	{
		if (OrgCopyParams.OperationList[i].PerformOperation == true)
		{
			++operationCnt;
		}
	}
	ImGui::Text("Current job count: %d", operationCnt);



	//draw track selection table

	ImGuiWindowFlags clipRegion_flags = ImGuiWindowFlags_HorizontalScrollbar;
	//oddly enough, imgui has a function to get the region width, but not the height, so we have to do that ourselves (I don't want to modify the imgui libraries for compatibility sake)
	int getWindowHeight = (ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);
	ImGui::BeginChild("ChartSplit", ImVec2(ImGui::GetWindowContentRegionWidth(), getWindowHeight - (120 + (140 * ImGui::GetIO().FontGlobalScale)) )  );//adds a scroll region to the chart, this second function is so that the UI still sort-of works at multiple scales. I don't know the correct formula for the bottom

	const int COLUMNS_COUNT = 3;
	if (ImGui::BeginTable("##table1", COLUMNS_COUNT, ImGuiTableFlags_Borders | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingPolicyFixedX))
	{
		//title for each ORG perameter
		ImGui::TableSetupColumn("Copy From:");
		ImGui::TableSetupColumn("Copy To:");
		ImGui::TableSetupColumn("Copy Type:");
		//ImGui::TableAutoHeaders();

		ImGui::TableNextRow();

		//each row must be done at the same time


		//column titles
		for (int i = 0; i < COLUMNS_COUNT; i++)
		{
			ImGui::TableSetColumnIndex(i);
			ImGui::Text(ImGui::TableGetColumnName(i));
		}

		//setup names for drop-down track selection
		const char* track_names [TrackCount] =
		{
			"1","2","3","4","5","6","7","8","Q","W","E","R","T","Y","U", "I",
		};

		//each track (have the track names change colors depending on the presence of notes and tell how many there are in each track)
		for (int j = 0; j < operationCnt; ++j)
		{
			ImGui::TableNextRow();
			for (int i = 0; i < COLUMNS_COUNT; i++)
			{
				//different parameters for each column

				switch (i)
				{

				default:
				case 0://copy from track
					{

						ImGui::TableSetColumnIndex(i);//put in correct column

						//the one that is currently selected
						const char* combo_label = track_names[OrgCopyParams.OperationList[j].CopyFrom];
						ImGui::PushID(j);//very important: makes each tab selection different
						ImGui::PushItemWidth(80 * ImGui::GetIO().FontGlobalScale);
						if (ImGui::BeginCombo("", combo_label))
						{
							//list all avalible tracks
							for (int n = 0; n < IM_ARRAYSIZE(track_names); n++)
							{
								//looks at the "copy from" org to see if the track we are copying from has notes in it
								//int thisss = orgs[0].tracks[OrgCopyParams.OperationList[n].CopyFrom].note_num;

								const bool is_selected = (OrgCopyParams.OperationList[j].CopyFrom == n);

								//if the entry has been clicked on
								if (ImGui::Selectable(track_names[n], is_selected))
									OrgCopyParams.OperationList[j].CopyFrom = n;//pass the corresponding number to our perameters


								//put the menu on the selected item to start with
								if (is_selected)
									ImGui::SetItemDefaultFocus();

							}
							ImGui::EndCombo();
						}
						ImGui::PopItemWidth();
						ImGui::PopID();



						//tell us if the track is populated
						if (orgs[0].tracks[OrgCopyParams.OperationList[j].CopyFrom].note_num)
						{
							ImVec4 TrackColor = TEXT_GREEN;
							ImGui::TextColored(TrackColor, "Note Count: %d", 
								orgs[0].tracks[OrgCopyParams.OperationList[j].CopyFrom].note_num
							);
						}
						else
						{
							ImVec4 TrackColor = TEXT_RED;
							ImGui::TextColored(TrackColor, "Note Count: 0");
						}




					}
					break;
				case 1://copy to track
					{

						ImGui::TableSetColumnIndex(i);//put in correct column

						//the one that is currently selected
						const char* combo_label = track_names[OrgCopyParams.OperationList[j].CopyTo];
						ImGui::PushID(j + IM_ARRAYSIZE(OrgCopyParams.OperationList));//adding the entire array from our job list so that these don't interfere with the tabs in case 0
						ImGui::PushItemWidth(80 * ImGui::GetIO().FontGlobalScale);
						if (ImGui::BeginCombo("", combo_label))
						{
							//list all avalible tracks
							for (int n = 0; n < IM_ARRAYSIZE(track_names); n++)
							{
								//looks at the "copy from" org to see if the track we are copying from has notes in it
								//int thisss = orgs[0].tracks[OrgCopyParams.OperationList[n].CopyFrom].note_num;

								const bool is_selected = (OrgCopyParams.OperationList[j].CopyTo == n);

								//if the entry has been clicked on
								if (ImGui::Selectable(track_names[n], is_selected))
									OrgCopyParams.OperationList[j].CopyTo = n;//pass the corresponding number to our perameters


								//put the menu on the selected item to start with
								if (is_selected)
									ImGui::SetItemDefaultFocus();

							}
							ImGui::EndCombo();
						}
						ImGui::PopItemWidth();
						ImGui::PopID();



						//tell us if the track is populated
						if (orgs[1].tracks[OrgCopyParams.OperationList[j].CopyTo].note_num)
						{
							ImVec4 TrackColor = TEXT_RED;
							ImGui::TextColored(TrackColor, "Note Count: %d",
								orgs[1].tracks[OrgCopyParams.OperationList[j].CopyTo].note_num
							);
						}
						else
						{
							ImVec4 TrackColor = TEXT_GREEN;
							ImGui::TextColored(TrackColor, "Note Count: 0");
						}




					}
					break;
				case 2://copy settings
					{
						ImGui::TableSetColumnIndex(i);
						ImGui::PushID(j);
					
						ImGui::Checkbox("Use TrackMASH", &OrgCopyParams.OperationList[j].UseTrackMash);

						ImGui::SameLine();
						HelpMarker("TrackMASH will combine the notes from 2 tracks into one.\nUse the buttons to select which notes are kept when 2 come into conflict.");

						//only show these if we are using trackMASH
						if (OrgCopyParams.OperationList[j].UseTrackMash)
						{
							
							//technically, it works even if we don't do this ckeck, because orgCopy uses if(2)else, so anything other than 2 would count as 1
							//if (OrgCopyParams.OperationList[j].MashPrioFile == 0)
							//	OrgCopyParams.OperationList[j].MashPrioFile = 1;
						
							ImGui::Text("Priority: "); ImGui::SameLine();
							//use radioButtons to select trackMASH priority
							ImGui::RadioButton("From", &OrgCopyParams.OperationList[j].MashPrioFile, 0); ImGui::SameLine();
							ImGui::RadioButton("To", &OrgCopyParams.OperationList[j].MashPrioFile, 2);
						}
						ImGui::PopID();
					}

					break;
					
				}


			}
		}
		
	


		ImGui::EndTable();
	}
	ImGui::EndChild();

	ImGui::Separator();
	
	if (ImGui::Button("GO"))
	{
		OrgCopyParams.BeginCopy = true;
	}
	

	if (OrgCopyParams.FinishedCopy == true)
	{
		OrgCopyParams.FinishedCopy = false;
		ImGui::OpenPopup("Result");
	}
	//make about popup menu
	ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));// , ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Result", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{

		if (OrgCopyParams.Success)
		{
			ImGui::TextColored(TEXT_GREEN, "Copy completed successfully!");
		}
		else
		{
			ImGui::TextColored(TEXT_RED, "Copy Failed!\nAre your input files valid ORGs?");
		}
		ImGui::Separator();
		if (ImGui::Button("Dismiss", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}




	//ImGui::End();

}

void ShowMIDI2ORG(void)
{

	//window layout
	//FileSelect
	//(is valid midi?)
	//options:



	//we are now running this in tabbed mode.
	//because of this, we no nonger need start() and end() functions

	//TODO: revise where the menu starts when you click on it
	//ImVec2 menuSize(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	//ImVec2 center((ImGui::GetIO().DisplaySize.x * 0.5f) - (menuSize.x * 0.5f),
	//	(ImGui::GetIO().DisplaySize.y * 0.5f) - (menuSize.y * 0.5f)
	//);
	//ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver);
	//ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

	//give it an X button
	//if (!ImGui::Begin("MIDI2ORG", p_open))
	//{
	//	ImGui::End();
	//	return;
	//}


	ImGui::Text("MIDI to convert:");
	ImGui::PushID(1);
	ImGui::InputTextWithHint("", "enter file path", MidiConvertParams.BackendOptions.Path, IM_ARRAYSIZE(MidiConvertParams.BackendOptions.Path));
	ImGui::PopID();


	ImGui::PushID(1);//unique ID (buttons with the same name need this)
	if (ImGui::Button("Browse"))
	{
		char* destination = OpenFile();
		if (destination != NULL)
			strcpy(MidiConvertParams.BackendOptions.Path, destination);
	}
	ImGui::PopID();
	ImGui::SameLine();
	//0-1 Red, Green, Blue, Alpha
	if(strlen(MidiConvertParams.BackendOptions.Path) == 0)
	{
		ImGui::TextColored(TEXT_YELLOW, "No Path Entered");
		ImGui::SameLine();
		HelpMarker("This operation cannot run without an input file.");
	}
	else if (MidiConvertParams.IsMIDI == 0)
	{
		ImGui::TextColored(TEXT_GREEN, "MIDI is GOOD");
		ImGui::SameLine();
		HelpMarker("The file header matches that of a MIDI file");
	}
	else if (MidiConvertParams.IsMIDI == -1)
	{
		ImGui::TextColored(TEXT_RED, "Invalid Path");//will turn colors based on if the file is detected as an org or not
		ImGui::SameLine();
		HelpMarker("The path you entered does not point to a file.");
	}
	else
	{
		ImGui::TextColored(TEXT_RED, "MIDI Not GOOD");//will turn colors based on if the file is detected as an org or not
		ImGui::SameLine();
		HelpMarker("MIDI2ORG did not recognize this file as a MIDI");
	}
	ImGui::Separator();



	ImGui::Checkbox("MIDI has Drum Channel", &MidiConvertParams.BackendOptions.HasDrumChannel);

	if (MidiConvertParams.BackendOptions.HasDrumChannel)
	{

		//setup names for drop-down track selection
		const char* channel_names[TrackCount] =
		{
			 "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15",
		};


		//the one that is currently selected
		const char* combo_label = channel_names[MidiConvertParams.BackendOptions.DrumChannel];
		ImGui::PushItemWidth(80);
		if (ImGui::BeginCombo("Select Drum Channel", combo_label))
		{
			//list all avalible tracks
			for (int n = 0; n < IM_ARRAYSIZE(channel_names); n++)
			{

				const bool is_selected = (MidiConvertParams.BackendOptions.DrumChannel == n);

				//if the entry has been clicked on
				if (ImGui::Selectable(channel_names[n], is_selected))
					MidiConvertParams.BackendOptions.DrumChannel = n;//pass the corresponding number to our perameters


				//put the menu on the selected item to start with
				if (is_selected)
					ImGui::SetItemDefaultFocus();

			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		HelpMarker("default drum channel is #9");


	}



	ImGui::Separator();

	ImGui::Checkbox("Force Simplify", &MidiConvertParams.BackendOptions.ForceSimplify);
	

	if (MidiConvertParams.BackendOptions.ForceSimplify)
	{





		float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		ImGui::PushButtonRepeat(true);
		if (ImGui::ArrowButton("##left", ImGuiDir_Down))
		{

			//no negatives allowed
			if (MidiConvertParams.ForceSimplifyExponet > 0)
				--MidiConvertParams.ForceSimplifyExponet;

			MidiConvertParams.BackendOptions.SimplestNote = pow(2, MidiConvertParams.ForceSimplifyExponet);

		}
		ImGui::SameLine(0.0f, spacing);
		if (ImGui::ArrowButton("##right", ImGuiDir_Up))
		{
			if (MidiConvertParams.ForceSimplifyExponet < 30)//2^31 is the max value of a signed int (needlessly huge numbers will be disregarded by the copy engine anyway)
				++MidiConvertParams.ForceSimplifyExponet;

			MidiConvertParams.BackendOptions.SimplestNote = pow(2, MidiConvertParams.ForceSimplifyExponet);

		}
		ImGui::PopButtonRepeat();


		ImGui::Text("Most precise note: 1/%d", MidiConvertParams.BackendOptions.SimplestNote);
		ImGui::SameLine();

		HelpMarker("Note must be a power of 2");


	}





	ImGui::Separator();

	if (ImGui::Button("GO"))
	{
		//clicking GO in a freshly opened window will result in IsMidi being 0.
		//becasue of this, we must also check the length of the path that's been entered
		if (MidiConvertParams.IsMIDI == 0 &&
			strlen(MidiConvertParams.BackendOptions.Path) != 0
			
			)
		{
			MidiConvertParams.BeginCopy = true;
		}
		else
		{
			ImGui::OpenPopup("Error");
		}



	}


	ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));// , ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{


		if (strlen(MidiConvertParams.BackendOptions.Path) == 0)
		{
			ImGui::TextColored(TEXT_RED, "No path has been selected.\nPlease enter a valid path.");
		}
		else if (MidiConvertParams.IsMIDI == -1)
		{
			ImGui::TextColored(TEXT_RED, "The path you entered is invalid.\nPlease enter a valid path.");
		}
		else
		{
			ImGui::TextColored(TEXT_RED, "The file you selected does not appear to be a MIDI.");
		}


		ImGui::Separator();
		if (ImGui::Button("Dismiss", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}





		ImGui::EndPopup();
	}




	ImGui::Separator();


	ImGuiWindowFlags clipRegion_flags = ImGuiWindowFlags_NoDecoration;
	//oddly enough, imgui has a function to get the region width, but not the height, so we have to do that ourselves (I don't want to modify the imgui libraries for compatibility sake)
	int getWindowHeight = (ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);

	ImGui::BeginChild("Terminal Limit", ImVec2(ImGui::GetWindowContentRegionWidth(), getWindowHeight - ( 120  + (MidiConvertParams.BackendOptions.ForceSimplify ? 35 : 0) + (MidiConvertParams.BackendOptions.HasDrumChannel ? 20 : 0)    + (110 * ImGui::GetIO().FontGlobalScale   ))), clipRegion_flags);//adds a scroll region to the chart
	ShowTerminal();
	ImGui::EndChild();



	//moved below ShowTerminal() so that we can use FinishedCopy to snap the terminal text to the bottom again
	if (MidiConvertParams.FinishedCopy)
	{
		MidiConvertParams.FinishedCopy = false;
		ImGui::OpenPopup("Result");
	}

	//center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));// , ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Result", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{

		ImGui::TextColored(TEXT_GREEN, "Finished");
		ImGui::Separator();
		if (ImGui::Button("Dismiss", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}







	//ImGui::End();

}

void ShowAppearanceWindow(bool* p_open)
{
	if (!ImGui::Begin("Appearance", p_open))
	{
		ImGui::End();
		return;
	}

	ImGui::ShowStyleEditor();

	ImGui::End();
}

//everything that comes out of the top menu bar (so basically, everything)
void ShowTopBar(void)
{
	//puts the back window in one spot
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(MainWindowWidth, MainWindowHeight));


	ImGui::Begin("Main window", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			//click button to get about info
			if (ImGui::Button("About.."))
			{

				ImGui::OpenPopup("About");

			}
			//make about popup menu
			ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("ORGTools\nby Dr_Glaucous (2022)");
				ImGui::Separator();
				ImGui::Text("Version: ");
				ImGui::Text(VERSION_NO);
				ImGui::Separator();
				if (ImGui::Button("Done", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}



			ImGui::MenuItem("Appearance", "", &TabOptions.ShowStyleEditor);

			#ifdef DEBUG_MODE
			ImGui::MenuItem("ImGUI Demo", "", &TabOptions.ShowDemoWindow);
			#endif

			if (ImGui::MenuItem("Exit", "Ctrl+W")) { glfwSetWindowShouldClose(window, 1); }

			ImGui::EndMenu();

		}


		//we moved all this into tabs across the top of the screen, so we don't need the menu anymore
		//if (ImGui::BeginMenu("Tools"))//select what tool to use here
		//{
		//	ImGui::MenuItem("OrgCopy", "", &TabOptions.ShowOrgCopy);
		//	ImGui::MenuItem("MIDI2ORG", "", &TabOptions.ShowMIDI2ORG);
		//	ImGui::EndMenu();
		//}


		ImGui::EndMenuBar();
	}

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;


	if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("MIDI2ORG"))
		{

			ShowMIDI2ORG();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("ORGCopy"))
		{

			ShowOrgCopy();


			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}










	ImGui::End();

};



//GUI is drawn here:
bool UpdateWindow(void)
{
	glfwPollEvents();//GUI engine

	//get size of the GLFW window
	glfwGetWindowSize(window, &MainWindowWidth, &MainWindowHeight);






#ifdef LEGACY_OPENGL
	ImGui_ImplOpenGL2_NewFrame();
#else
	ImGui_ImplOpenGL3_NewFrame();
#endif
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();


	//start of GUI layout

	if (!MidiConvertParams.RunningCopy)
	{
	
	ShowTopBar();

#ifdef DEBUG_MODE
	if (TabOptions.ShowDemoWindow)
		ImGui::ShowDemoWindow();
#endif


	if (TabOptions.ShowStyleEditor)
		ShowAppearanceWindow(&TabOptions.ShowStyleEditor);

	//end of GUI layout

	}
	else
	{
		//during processing, the only thing you can see is MIDI2ORG output terminal
		 		
		//puts the back window in one spot
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(MainWindowWidth, MainWindowHeight));


		ImGui::Begin("Main window", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);


		ShowTerminal();


		ImGui::End();
	}



	ImGui::Render();//put the GUI on the screen

	glClear(GL_COLOR_BUFFER_BIT);
#ifdef LEGACY_OPENGL
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#else
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
	glfwSwapBuffers(window);


	return glfwWindowShouldClose(window);

}

//manages all window events
bool TopFunction(void)
{
	static int currentState{};

	switch (currentState)
	{
	case 0://start
		window = InitializeGLFW("ORGTools");
		InitializeIMGUI(window);

		//makes the scale equivalent to that of the user's main preference
		//this also rebuilds the font atlas for crisp text. we only need to call this one time
		SetUpUI();

		currentState = 1;
		break;
	case 1://run
		if (UpdateWindow())//returns a 1 when finished
			currentState = 2;
		RunBackends();
		break;
	case 2://stop
		DestroyGLFW(window);
		DestroyIMGUI();
		return 1;
		break;
	}
	return 0;
}