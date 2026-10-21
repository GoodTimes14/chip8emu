cmake_minimum_required(VERSION 4.3)
project(chip8emu C)

set(CMAKE_C_STANDARD 99)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_subdirectory(src)
