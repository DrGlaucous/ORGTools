#pragma once
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "../libs/imgui/imgui.h"
#include "../libs/imgui/imgui_impl_glfw.h"
#ifdef LEGACY_OPENGL
#include "../libs/imgui/imgui_impl_opengl2.h"
#else
#include "../libs/imgui/imgui_impl_opengl3.h"
#endif


#include "Main.h"
#include "ORGCopy.h"

#define TEXT_RED ImVec4(0.7f, 0, 0, 1)
#define TEXT_GREEN ImVec4(0, 0.7f, 0, 1)
#define TEXT_LIGHT_GREEN ImVec4(0, 1, 0, 1)
#define TEXT_YELLOW ImVec4(0.7f, 0.7f, 0, 1)

//save options that are configured by the top bar (such as what menu to show)
typedef struct TopBarOPTIONS
{
	//bool ShowOrgCopy{};
	//bool ShowMIDI2ORG{};
	bool ShowStyleEditor{};

	#ifdef DEBUG_MODE
	bool ShowDemoWindow{};
	#endif

} TopBarOPTIONS;


extern TopBarOPTIONS TabOptions;


GLFWwindow* InitializeGLFW(const char* name);
int DestroyGLFW(GLFWwindow* Lwindow);
int InitializeIMGUI(GLFWwindow* Lwindow);
int DestroyIMGUI(void);
void WriteToTerminal(char* NewText);
bool UpdateWindow(void);
bool TopFunction(void);
void RunTopSparingly(int WaitMilliseconds);

