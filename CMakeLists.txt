cmake_minimum_required(VERSION 3.20)

project(chess_engine VERSION 0.1.0 LANGUAGES CXX)

# Export compile commands for tooling support.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
