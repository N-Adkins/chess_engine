cmake_minimum_required(VERSION 3.20)

project(chess_engine VERSION 0.1.0 LANGUAGES CXX)

# Export compile commands for tooling support.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ensure MSVC generates Program Database (PDB) debug info in Debug/RelWithDebInfo.
if(MSVC)
	# /Zi equivalent via CMake property; applies to C and C++.
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
	# Use the dynamic runtime by default: /MD in Release and /MDd in Debug.
	if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
	endif()
endif()

add_subdirectory(src)
