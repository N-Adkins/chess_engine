cmake_minimum_required(VERSION 3.20)

project(chess_engine VERSION 0.1.0 LANGUAGES CXX)

# Export compile commands for tooling support.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
	if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
	endif()
endif()

add_subdirectory(src)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

target_link_libraries(chess_engine PRIVATE 
	SDL3::SDL3
	SDL3_image::SDL3_image
)

target_include_directories(chess_engine PRIVATE 
	vendored/SDL/include
	vendored/SDL_image/include
)