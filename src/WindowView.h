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

//save options that are configured by the top bar (such as what menu to show)
typedef struct TopBarOPTIONS
{
	bool ShowOrgCopy{};
	bool ShowMIDI2ORG{};



	bool ShowDemoWindow{};

} TopBarOPTIONS;


typedef struct ORGCopyOPTIONS {
	bool track1Populated;
	char track1Path[PATH_LENGTH]{};//should be long enough for most paths
	char track1PathOLD[PATH_LENGTH]{};//store old path in buffer so we can run some functions only if the path changes

	char track2Path[PATH_LENGTH]{};
	char track2PathOLD[PATH_LENGTH]{};

} ORGCopyOPTIONS;





GLFWwindow* InitializeGLFW(const char* name);
int DestroyGLFW(GLFWwindow* Lwindow);
int InitializeIMGUI(GLFWwindow* Lwindow);
int DestroyIMGUI(void);
bool UpdateWindow(void);
bool TopFunction(void);


