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
#include "Main.h"
#include "SharedUtil.h"
//I ripped most of this from DoConfig for CSE2 (cry about it)

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720


GLFWwindow* window = NULL;
GLFWwindow* AboutWindow = NULL;
const char* glsl_version = NULL;

int MainWindowWidth{};
int MainWindowHeight{};

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


//everything that comes out of the top menu bar (so basically, everything)
void ShowTopBar(void)
{
	//puts the back window in one spot
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(MainWindowWidth, MainWindowHeight));


	ImGui::Begin("Main window", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

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
			ImGui::SetNextWindowPos(center);
			if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("ORGTools\nby Dr_Glaucous");
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

			ImGui::MenuItem("ImGUI Demo", "", &TabOptions.ShowDemoWindow);

			if (ImGui::MenuItem("Exit", "Ctrl+W")) { glfwSetWindowShouldClose(window, 1); }

			ImGui::EndMenu();

		}

		if (ImGui::BeginMenu("Tools"))//select what tool to use here
		{
			ImGui::MenuItem("OrgCopy", "", &TabOptions.ShowOrgCopy);
			ImGui::MenuItem("MIDI2ORG", "", &TabOptions.ShowMIDI2ORG);
			ImGui::EndMenu();
		}


		ImGui::EndMenuBar();
	}


	ImGui::End();

};


void ShowOrgCopy(bool* p_open)
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





	//TODO: revise where the menu starts when you click on it
	ImVec2 menuSize(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	ImVec2 center((ImGui::GetIO().DisplaySize.x * 0.5f) - (menuSize.x * 0.5f),
		(ImGui::GetIO().DisplaySize.y * 0.5f) - (menuSize.y * 0.5f)
	);
	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

	//give it an X button
	if (!ImGui::Begin("ORGCopy", p_open))
	{
		ImGui::End();
		return;
	}

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
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "ORG is GOOD");
		ImGui::SameLine();
		HelpMarker("The file header matches that of an ORG file.\nTotal notes: %d\nWait time: %d\nBeats Per Measure: %d\nNotes Per Beat: %d", 
			orgs[0].totalNotes, orgs[0].wait, orgs[0].bar, orgs[0].dot);
	}
	else
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "ORG Not GOOD");//will turn colors based on if the file is detected as an org or not
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
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Same Directory");
		ImGui::SameLine();
		HelpMarker("The destination ORG is the same as the copy-from org!\nYou can still complete the copy operation if this is what you want.");
	}
	else if (orgs[1].IsOrg)
	{
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "ORG is GOOD");
		ImGui::SameLine();
		HelpMarker("The file header matches that of an ORG file.\nTotal notes: %d\nWait time: %d\nBeats Per Measure: %d\nNotes Per Beat: %d",
			orgs[1].totalNotes, orgs[1].wait, orgs[1].bar, orgs[1].dot);
	}
	else
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "ORG Not GOOD");//will turn colors based on if the file is detected as an org or not
		ImGui::SameLine();
		HelpMarker("ORGCopy did not recognize this file's header as ORG.\nAttempting operations on this file may corrupt it or crash the program!");
	}

	ImGui::Separator();
	}


	//table selection buttons, tell orgcopy how many operations you would like done
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

	ImGui::SameLine();
	HelpMarker("Press the \"+\" button to add copy jobs to the cue,\n press the \"-\" button to take them away");

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
	ImGui::BeginChild("ChartSplit", ImVec2(ImGui::GetWindowContentRegionWidth(), getWindowHeight - 210) );//adds a scroll region to the chart

	const int COLUMNS_COUNT = 3;
	if (ImGui::BeginTable("##table1", COLUMNS_COUNT, ImGuiTableFlags_Borders | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
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

		//setup enums for drop-down track selection
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
						ImGui::PushItemWidth(80);
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
							ImVec4 TrackColor = ImVec4(0, 1, 0, 1);
							ImGui::TextColored(TrackColor, "Note Count: %d", 
								orgs[0].tracks[OrgCopyParams.OperationList[j].CopyFrom].note_num
							);
						}
						else
						{
							ImVec4 TrackColor = ImVec4(1, 0, 0, 1);
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
					ImGui::PushItemWidth(80);
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
						ImVec4 TrackColor = ImVec4(1, 0, 0, 1);
						ImGui::TextColored(TrackColor, "Note Count: %d",
							orgs[1].tracks[OrgCopyParams.OperationList[j].CopyTo].note_num
						);
					}
					else
					{
						ImVec4 TrackColor = ImVec4(0, 1, 0, 1);
						ImGui::TextColored(TrackColor, "Note Count: 0");
					}




				}
					break;
				case 2://copy settings
					{
						ImGui::TableSetColumnIndex(i);
						ImGui::PushID(j);
					
						ImGui::Checkbox("Use TrackMASH", &OrgCopyParams.OperationList[j].UseTrackMash);

						//only show these if we are using trackMASH
						if (OrgCopyParams.OperationList[j].UseTrackMash)
						{
							
							//technically, it works even if we don't do this ckeck, because orgCopy uses if(2)else, so anything other than 2 would count as 1
							//if (OrgCopyParams.OperationList[j].MashPrioFile == 0)
							//	OrgCopyParams.OperationList[j].MashPrioFile = 1;
						
							ImGui::Text("Priority: "); ImGui::SameLine();
							//use radioButtons to select trackMASH priority
							ImGui::RadioButton("1", &OrgCopyParams.OperationList[j].MashPrioFile, 0); ImGui::SameLine();
							ImGui::RadioButton("2", &OrgCopyParams.OperationList[j].MashPrioFile, 2);
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
	center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	ImGui::SetNextWindowPos(center);// , ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Result", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{

		if (OrgCopyParams.Success)
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Copy completed successfully!");
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Copy Failed!\nAre your input files valid ORGs?");
		}
		ImGui::Separator();
		if (ImGui::Button("Dismiss", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}






	//backend processes

	HandleOrgCopyBackend();


	ImGui::End();

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

//GUI is drawn here:
bool UpdateWindow(void)
{
	glfwPollEvents();//GUI engine

	glfwGetWindowSize(window, &MainWindowWidth, &MainWindowHeight);


#ifdef LEGACY_OPENGL
	ImGui_ImplOpenGL2_NewFrame();
#else
	ImGui_ImplOpenGL3_NewFrame();
#endif
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();


	//start of GUI layout


	ShowTopBar();

	if(TabOptions.ShowDemoWindow)
		ImGui::ShowDemoWindow();

	if (TabOptions.ShowOrgCopy)
		ShowOrgCopy(&TabOptions.ShowOrgCopy);

	if (TabOptions.ShowStyleEditor)
		ShowAppearanceWindow(&TabOptions.ShowStyleEditor);

	//end of GUI layout



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
		currentState = 1;
		break;
	case 1://run
		if (UpdateWindow())//returns a 1 when finished
			currentState = 2;
		break;
	case 2://stop
		DestroyGLFW(window);
		DestroyIMGUI();
		return 1;
		break;
	}
	return 0;
}