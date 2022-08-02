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
//save options that are configured by the top bar (such as what menu to show)
typedef struct TopBarOPTIONS
{
	bool ShowOrgCopy{};
	bool ShowMIDI2ORG{};
	bool ShowStyleEditor{};


	bool ShowDemoWindow{};

} TopBarOPTIONS;


extern TopBarOPTIONS TabOptions;


GLFWwindow* InitializeGLFW(const char* name);
int DestroyGLFW(GLFWwindow* Lwindow);
int InitializeIMGUI(GLFWwindow* Lwindow);
int DestroyIMGUI(void);
bool UpdateWindow(void);
bool TopFunction(void);


