cmake_minimum_required(VERSION 3.19)  # CMake version check
project(socketcom)               # Create project "socketcom"
set(CMAKE_CXX_STANDARD 17)       # C++17 standard

find_package(Boost 1.75.0)

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES
	main.cpp)

# Add executable target with SOURCE_FILES
add_executable(socketcom ${SOURCE_FILES})