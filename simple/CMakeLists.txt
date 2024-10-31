cmake_minimum_required(VERSION "3.19.2")

project(simple VERSION 0.5.0 DESCRIPTION "A game engine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_STATIC_VULKAN On CACHE BOOL "" FORCE)
set(ALLOW_EXTERNAL_SPIRV_TOOLS ON)

option(INCLUDE_IM_GUI "Include ImGui in rendering pipeline" ON)
option(EDITOR "Include editor functions and data types" ON)

if (INCLUDE_IM_GUI)
	add_compile_definitions(INCLUDE_IM_GUI)
endif()
if (EDITOR)
	add_compile_definitions(EDITOR)
endif()

add_subdirectory(libraries/vulkan/Vulkan-Headers-main)
add_subdirectory(libraries/vulkan/Vulkan-Loader-main)
add_subdirectory(libraries/glfw/glfw-3.3.9)
add_subdirectory(libraries/glslang)
add_subdirectory(libraries/tinyobjloader-release)

add_library(simple STATIC
	"src/simple.cpp"
)

target_link_libraries(simple vulkan glfw tinyobjloader glslang)

target_include_directories(simple
	PUBLIC headers
	PUBLIC imgui
	PUBLIC src/vulkan
	PUBLIC src
	PUBLIC libraries/vulkan/Vulkan-Headers-main/include
	PUBLIC libraries/glfw/glfw-3.3.9/include
	PUBLIC libraries/glslang/glslang/include
	PRIVATE libraries/imgui_backend
)
