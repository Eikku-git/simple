cmake_minimum_required(VERSION "3.19.2")

project(test VERSION 1.0.0 DESCRIPTION "Test")

set(CMAKE_CXX_STANDARD 20)

add_executable(test
	"src/main.cpp"
)

add_subdirectory(../simple [EXCLUDE_FROM_ALL])

target_include_directories(test
	PRIVATE ../engine/headers
)

target_link_libraries(test simple)
