#include <stdio.h>
#include "chip8emu.h"

#include <stdlib.h>
#include <string.h>


int main(void) {
    init_chip8emu();


    return 0;
}

void init_chip8emu() {
    struct chip8_mem* memory = calloc(1, sizeof(struct chip8_mem));
    add_font_memory(memory);
    memory->program_counter = 0x200;
    memory->stack_pointer = 0xECF;
    load_program(memory, "chip8-logo.ch8");
}


void add_font_memory(struct chip8_mem* memory) {
    memcpy(&memory->rom[0x100], font_bits, sizeof(font_bits));
}

uint16_t fetch_inst(struct chip8_mem *memory) {
    //Fetch dell'istruzione: mem_read di PC è l'high byte mentre PC + 1 è low byte
    uint16_t instruction = ((uint16_t) mem_read(memory, memory->program_counter) << 8) |mem_read(memory, memory->program_counter + 1);

    return instruction;
}



int load_program(struct chip8_mem *memory, const char* file_name) {
    FILE *fileptr = fopen(file_name, "rb");
    if (fileptr == NULL) {
        fprintf(stderr,"Can't load file: %s \n", file_name);
        perror(file_name);
        return -1;
    }
    uint8_t buffer[PROGRAM_SIZE_LIMIT + 1] = {0};
    size_t x = fread(buffer, 1 , (PROGRAM_SIZE_LIMIT + 1), fileptr);
    if (x == 0) {
        fprintf(stderr,"File is empty: %s \n", file_name);
        return -2;
    }
    if (x > PROGRAM_SIZE_LIMIT) {
        fprintf(stderr,"File is too large: %s \n", file_name);
        return -3;
    }

    fclose(fileptr);
    memcpy(&memory->memory[0x200], buffer,PROGRAM_SIZE_LIMIT);
    printf("Program loaded into memory! %s", file_name);

    return 0;
}

struct chip8_instruction decode_inst(uint16_t instruction) {

    struct chip8_instruction decoded_instruction;

    decoded_instruction.opcode = instruction >> 12;
    decoded_instruction.register_x = (instruction >> 8) & 0xF;
    decoded_instruction.register_y = (instruction >> 4) & 0xF;
    decoded_instruction.n_value = instruction & 0xF;
    decoded_instruction.kk_value = instruction & 0xFF;
    decoded_instruction.address = instruction & 0xFFF;

    return decoded_instruction;
}


void exec_inst(struct chip8_mem *memory, struct chip8_instruction inst) {


    switch (inst.opcode) {

        case 0x00E0:
            clear_display(memory);
            break;

        default: printf("Instruction not found or implemented: %d", inst.opcode);
    }

}

void clear_display(struct chip8_mem *memory) {
}

void mem_write(struct chip8_mem* memory, uint16_t address, uint8_t val) {
    if (address >= ROM_REGION_START_ADDR) { //ROM memory
        printf("Can't write on ROM memory");

    } else {
        const uint16_t effective_address = address & (RAM_SIZE - 1);
        memory->memory[effective_address] = val;
    }
}

uint8_t mem_read(struct chip8_mem *memory, uint16_t address) {
    uint16_t effective_address;
    if (address >= ROM_REGION_START_ADDR) { //ROM memory
        effective_address = address & (ROM_SIZE - 1);
        return memory->rom[effective_address];
    }

    effective_address = address & (RAM_SIZE - 1);
    return memory->memory[effective_address];
}
