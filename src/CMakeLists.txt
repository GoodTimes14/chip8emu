


add_executable(chip8emu chip8emu.c chip8emu.h)

# paths relative to src/
target_include_directories(chip8emu PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(chip8emu PRIVATE SDL3::SDL3)