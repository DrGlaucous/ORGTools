#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nfd.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "../libs/imgui/imgui.h"
#include "../libs/imgui/imgui_impl_glfw.h"
#ifdef LEGACY_OPENGL
#include "../libs/imgui/imgui_impl_opengl2.h"
#else
#include "../libs/imgui/imgui_impl_opengl3.h"
#endif


#include "WindowView.h"
#include "Main.h"

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
ORGCopyOPTIONS OrgCopySettings{};

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
				ImGui::Text("ORGTools");
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





			if (ImGui::MenuItem("Exit", "Ctrl+W")) { glfwSetWindowShouldClose(window, 1); }


			if (ImGui::MenuItem("Demo", "")) { TabOptions.ShowDemoWindow = !TabOptions.ShowDemoWindow; }
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
	//TODO: revise where the menu starts when you click on it
	ImVec2 menuSize(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	ImVec2 center((ImGui::GetIO().DisplaySize.x * 0.5f) - (menuSize.x * 0.5f),
		(ImGui::GetIO().DisplaySize.y * 0.5f) - (menuSize.y * 0.5f)
	);

	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("ORGCopy", p_open))
	{
		ImGui::End();
		return;
	}


	ImGui::InputTextWithHint("File to be copied from", "enter file path", OrgCopySettings.track1Path, IM_ARRAYSIZE(OrgCopySettings.track1Path));
	if (ImGui::Button("Browse for file 1"))
	{
		char* destination = OpenFile();
		if (destination != NULL)
			strcpy(OrgCopySettings.track1Path, destination);
	}
	ImGui::Separator();
	ImGui::InputTextWithHint("File to be copied to", "enter file path", OrgCopySettings.track2Path, IM_ARRAYSIZE(OrgCopySettings.track2Path));
	if (ImGui::Button("Browse for file 2"))
	{
		char* destination = OpenFile();
		if(destination != NULL)
			strcpy(OrgCopySettings.track2Path, destination);
	}
	ImGui::Separator();


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