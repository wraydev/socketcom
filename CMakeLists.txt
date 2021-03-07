#########################################
#			CMAKE CONFIG				#
#########################################
cmake_minimum_required(VERSION 3.19)	# CMake version check
project(socketcom)						# Create project "socketcom"
set(CMAKE_CXX_STANDARD 17)				# C++17 standard

#########################################
#				BOOST					#
#########################################
set(Boost_USE_STATIC_LIBS        ON)	# Force use of static libraries
set(Boost_USE_MULTITHREADED      ON)	# Force multithread libraries
set(Boost_USE_STATIC_RUNTIME 	OFF)	# Force disable runtime linked libraries
find_package(Boost 1.75.0 REQUIRED date_time json)		# Find Boost 1.75 minimum

#########################################
#			CONSTRUCT SOURCE			#
#########################################
set(SOURCE_FILES
	src/main.cpp)

#########################################
#			INCLUDES/LIBS				#
#########################################
include_directories(${Boost_INCLUDE_DIRS})

#########################################
#			COMPILER FLAGS				#
#########################################
add_definitions(-D_WIN32_WINNT=0x0601)	# Windows 7 minimum: https://docs.microsoft.com/en-us/windows/win32/winprog/using-the-windows-headers?redirectedfrom=MSDN

#########################################
#			GENERATE EXECUTABLE			#
#########################################
add_executable(socketcom ${SOURCE_FILES})
target_link_libraries(socketcom ${Boost_LIBRARIES})
