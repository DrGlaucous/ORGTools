cmake_minimum_required(VERSION 3.8)

option(ORGTools_LEGACY_OPENGL "Use OpenGL 2.1 instead of 3.2" OFF)

option(PKG_CONFIG_STATIC_LIBS "On platforms with pkg-config, static-link the dependencies (good for Windows builds, so you don't need to bundle DLL files)" OFF)
option(MSVC_LINK_STATIC_RUNTIME "Link the static MSVC runtime library (Visual Studio only)" OFF)

option(FORCE_LOCAL_LIBS "Compile the built-in version of GLFW3 instead of using the system-provided one" OFF)

project(ORGTools LANGUAGES C CXX)

add_subdirectory(libs/nativefiledialog)

add_executable(ORGTools WIN32


	"src/Main.cpp"
	"src/Main.h"
	"src/WindowView.cpp"
	"src/WindowView.h"
	"src/ORGCopy.cpp"
	"src/ORGCopy.h"
	"src/MIDI2ORG.cpp"
	"src/MIDI2ORG.h"
	"src/SharedUtil.cpp"
	"src/SharedUtil.h"
	"src/File.cpp"
	"src/File.h"

	"libs/imgui/imconfig.h"
	"libs/imgui/imgui.cpp"
	"libs/imgui/imgui.h"
	"libs/imgui/imgui_demo.cpp"
	"libs/imgui/imgui_draw.cpp"
	"libs/imgui/imgui_impl_glfw.cpp"
	"libs/imgui/imgui_impl_glfw.h"
	"libs/imgui/imgui_internal.h"
	"libs/imgui/imgui_widgets.cpp"
	"libs/imgui/imstb_rectpack.h"
	"libs/imgui/imstb_textedit.h"
	"libs/imgui/imstb_truetype.h"
)

set_target_properties(ORGTools PROPERTIES
	C_STANDARD 11#90
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF
	CXX_STANDARD 11#98
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)


target_link_libraries(ORGTools PRIVATE ${CMAKE_DL_LIBS})


target_include_directories(ORGTools PRIVATE "libs/glad/include")
target_compile_definitions(ORGTools PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)

if(ORGTools_LEGACY_OPENGL)
	target_compile_definitions(ORGTools PRIVATE LEGACY_OPENGL)
	target_sources(ORGTools PRIVATE
		"libs/imgui/imgui_impl_opengl2.cpp"
		"libs/imgui/imgui_impl_opengl2.h"
	)
else()
	target_sources(ORGTools PRIVATE
		"libs/imgui/imgui_impl_opengl3.cpp"
		"libs/imgui/imgui_impl_opengl3.h"
	)
endif()

if(PKG_CONFIG_STATIC_LIBS)
	target_link_options(ORGTools PRIVATE "-static")
endif()

if(MSVC)
	# Disable warnings that normally fire up on MSVC when using "unsafe" functions instead of using MSVC's "safe" _s functions
	target_compile_definitions(ORGTools PRIVATE _CRT_SECURE_NO_WARNINGS)

	# Use `main` instead of `WinMain`
	set_target_properties(ORGTools PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")

	# statically-link the CRT (so MSVC dlls are not required to run the program)
	if(MSVC_LINK_STATIC_RUNTIME)
		foreach(flag_var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
			if(${flag_var} MATCHES "/MD")
				string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
			endif()
		endforeach()
	endif()
endif()


################
# Dependencies #
################

# glad

add_subdirectory("libs/glad" "glad" EXCLUDE_FROM_ALL)

target_link_libraries(ORGTools PRIVATE glad)
target_link_libraries(ORGTools PRIVATE nativefiledialog)


if (CMAKE_VERSION GREATER_EQUAL 3.11)
	cmake_policy(SET CMP0072 NEW)
endif()

find_package(OpenGL REQUIRED)
target_link_libraries(ORGTools PRIVATE OpenGL::GL)

# GLFW3

if(NOT FORCE_LOCAL_LIBS)
	find_package(PkgConfig QUIET)
endif()

if(NOT FORCE_LOCAL_LIBS)
	find_package(glfw3)

	if (PKG_CONFIG_FOUND)
		pkg_check_modules(glfw3 QUIET IMPORTED_TARGET glfw3)
	endif()
endif()

if(TARGET PkgConfig::glfw3)
	# pkg-config
	if (PKG_CONFIG_STATIC_LIBS)
		message(STATUS "Using system GLFW3 (pkg-config, static)")
		target_compile_options(ORGTools PRIVATE ${glfw3_STATIC_CFLAGS})
		target_link_libraries(ORGTools PRIVATE ${glfw3_STATIC_LDFLAGS})
	else()
		message(STATUS "Using system GLFW3 (pkg-config, dynamic)")
		target_compile_options(ORGTools PRIVATE ${glfw3_CFLAGS})
		target_link_libraries(ORGTools PRIVATE ${glfw3_LDFLAGS})
	endif()
elseif(TARGET glfw)
	# CMake
	message(STATUS "Using system GLFW3 (CMake)")
	target_link_libraries(ORGTools PRIVATE glfw)
else()
	# Compile it ourselves
	message(STATUS "Using local GLFW3")
	set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
	set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
	add_subdirectory("libs/glfw" "glfw" EXCLUDE_FROM_ALL)
	target_link_libraries(ORGTools PRIVATE glfw)
endif()



#add the midi parsing library
message(STATUS "adding midifile")
add_subdirectory("libs/MIDIParse")
target_link_libraries(ORGTools PRIVATE libParser)