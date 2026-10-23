//
// Created by raniero on 12/10/26.
//

#ifndef CHIP8EMU_CHIP8EMU_H
#define CHIP8EMU_CHIP8EMU_H
#include <stdint.h>


#define VIP_FONT_ADDR 0x8100
#define ROM_REGION_START_ADDR 0x8000
#define STACK_LOW 0x0EA0
#define STACK_TOP 0x0ECF
#define RAM_SIZE 4096
#define ROM_SIZE 512
#define PROGRAM_SIZE_LIMIT ((RAM_SIZE - 352) - 0x200)
#define PROGRAM_START 0x0200
#define PROGRAM_END 0x0E9F
#define DISPLAY_SIZE 2048

uint8_t font_bits[] = {
    // Page offsets for 0 - F, e.g. 0x20 means 0x8120
    0x30, 0x39, 0x22, 0x2A, 0x3E, 0x20, 0x24, 0x34,
    0x26, 0x28, 0x2E, 0x18, 0x14, 0x1C, 0x10, 0x12,
    // Actual font data
    0xF0, 0x80, 0xF0, 0x80, 0xF0, 0x80, 0x80, 0x80,
    0xF0, 0x50, 0x70, 0x50, 0xF0, 0x50, 0x50, 0x50,
    0xF0, 0x80, 0xF0, 0x10, 0xF0, 0x80, 0xF0, 0x90,
    0xF0, 0x90, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90,
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x10, 0x10, 0x10,
    0x10, 0x60, 0x20, 0x20, 0x20, 0x70, 0xA0, 0xA0,
    0xF0, 0x20, 0x20
};


struct chip8_mem {
    uint16_t address_register;
    uint16_t program_counter;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint16_t stack_pointer;
    uint8_t memory[RAM_SIZE];
    uint8_t rom[ROM_SIZE];
    uint8_t display[DISPLAY_SIZE];
};

struct chip8_instruction {
    uint16_t full_inst;
    uint8_t opcode;
    uint8_t n_value;
    uint8_t kk_value;
    uint8_t register_x;
    uint8_t register_y;
    uint16_t address;
};

extern void reset(struct chip8_mem* memory);

extern void add_font_memory(struct chip8_mem* memory);

extern uint16_t fetch_inst(struct chip8_mem* memory);

extern int load_program(struct chip8_mem* memory, const char* file_name);

extern struct chip8_instruction decode_inst(uint16_t instruction);

extern void exec_inst(struct chip8_mem* memory, struct chip8_instruction inst);

extern void push_stack(struct chip8_mem* memory, uint8_t value);

extern uint8_t pop_stack(struct chip8_mem* memory);

extern uint8_t read_register(struct chip8_mem* memory, uint8_t reg_num);

extern void write_register(struct chip8_mem* memory, uint8_t reg_num, uint8_t value);

extern int mem_check_bounds(uint16_t address);

extern void clear_display(struct chip8_mem* memory);

extern void read_register(struct chip8_mem* memory, uint8_t reg_num);

extern void write_register(struct chip8_mem* memory, uint8_t reg_num, uint8_t val);

extern void mem_write(struct chip8_mem* memory, uint16_t address, uint8_t val);

extern uint8_t mem_read(struct chip8_mem* memory, uint16_t address);

extern void init_chip8emu();

#endif //CHIP8EMU_CHIP8EMU_H
