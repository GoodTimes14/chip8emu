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
    uint16_t instruction = ((uint16_t) mem_read(memory, memory->program_counter) << 8) | mem_read(memory, memory->program_counter + 1);

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

    decoded_instruction.full_inst = instruction;
    decoded_instruction.opcode = instruction >> 12;
    decoded_instruction.register_x = (instruction >> 8) & 0xF;
    decoded_instruction.register_y = (instruction >> 4) & 0xF;
    decoded_instruction.n_value = instruction & 0xF;
    decoded_instruction.kk_value = instruction & 0xFF;
    decoded_instruction.address = instruction & 0xFFF;

    return decoded_instruction;
}


void exec_inst(struct chip8_mem *memory, struct chip8_instruction inst) {
    if (inst.full_inst == 0x00E0) {
        clear_display(memory);

    } else if (inst.full_inst == 0x00EE) {
        //Return from subroutine to address pull from stack
        if (memory->stack_pointer == STACK_TOP) {
            fprintf(stderr, "Tried to return from subroutine with an empty stack, aborting...");
            exit(-1);

            return;
        }
        uint16_t jump = pop_stack(memory) << 8;
        jump |= pop_stack(memory);

        memory->program_counter = jump;
    }

    switch (inst.opcode) {
        case 0:
            printf("0mmm instruction is ignored");
            break;

        case 1:
            //1mmm: Jump to address at mmm
            if (mem_check_bounds(inst.address) == 0) {
                fprintf(stderr, "Jumping out of program memory bounds, aborting...");
                exit(-1);
            }

            break;
        case 2:
            //2mmm: push return address onto stack and call subroutine at address mmm
            if (mem_check_bounds(inst.address) == 0) {
                fprintf(stderr, "Jumping out of program memory bounds, aborting...");
                exit(-1);
            }

            uint16_t pc_stack = memory->program_counter + 2;
            push_stack(memory,pc_stack & 0xFF);
            push_stack(memory,pc_stack >> 8);

            memory->program_counter = inst.address;
            break;
        case 3:
            uint8_t reg_value = read_register(memory, inst.register_x);
            if (reg_value == inst.kk_value) {
                memory->program_counter += 2;
            }

            break;
        case 4:
            reg_value = read_register(memory, inst.register_x);
            if (reg_value != inst.kk_value) {
                memory->program_counter += 2;
            }

            break;
        case 5:
            reg_value = read_register(memory, inst.register_x);
            uint8_t reg_value_y = read_register(memory, inst.register_y);
            if (reg_value == reg_value_y) {
                memory->program_counter += 2;
            }

            break;
        case 6:
            write_register(memory,inst.register_x, inst.kk_value);
            break;
        case 7:
            reg_value = read_register(memory, inst.register_x);
            write_register(memory,inst.register_x, reg_value + inst.kk_value);
        case 8:
            /* 8xy0	set Vx to the value of Vy
             * 8xy1	set Vx to the result of bitwise Vx OR Vy, set VF to 0, even if x is F! (VF is written last)
             * 8xy2	set Vx to the result of bitwise Vx AND Vy, set VF to 0, even if x is F! (VF is written last)
             * 8xy3	set Vx to the result of bitwise Vx XOR Vy, set VF to 0, even if x is F! (VF is written last)
             * 8xy4	add Vy to Vx, VF is set to 1 if an overflow happened, to 0 if not, even if x=F! (VF is written last)
             * 8xy5	subtract Vy from Vx, VF is set to 0 if an underflow happened, to 1 if not, even if x=F! (VF is written last)
             * 8xy6	set Vx to Vy and shift Vx one bit to the right, set VF to the bit shifted out, even if x=F! (VF is written last)
             * 8xy7	set Vx to the result of subtracting Vx from Vy, VF is set to 0 if an underflow happened, to 1 if not, even if x=F! (VF is written last)
             * 8xyE	set Vx to Vy and shift Vx one bit to the left, set VF to the bit shifted out, even if x=F! (VF is written last)
             */
            switch (inst.n_value) {
                case 0:
                    reg_value = read_register(memory, inst.register_x);
                    write_register(memory,inst.register_y, reg_value);
                    break;
                case 1:
                    reg_value = read_register(memory, inst.register_x);
                    reg_value_y = read_register(memory, inst.register_y);
                    write_register(memory,inst.register_y, reg_value | reg_value_y);
                    write_register(memory,0xF, 0);
                    break;
                case 2:
                    reg_value = read_register(memory, inst.register_x);
                    reg_value_y = read_register(memory, inst.register_y);
                    write_register(memory,inst.register_y, reg_value & reg_value_y);
                    write_register(memory,0xF, 0);
                    break;
                case 3:
                    reg_value = read_register(memory, inst.register_x);
                    reg_value_y = read_register(memory, inst.register_y);
                    write_register(memory,inst.register_y, reg_value ^ reg_value_y);
                    write_register(memory,0xF, 0);
                    break;
                case 4:
                    reg_value = read_register(memory, inst.register_x);
                    reg_value_y = read_register(memory, inst.register_y);
                    uint8_t sum = reg_value + reg_value_y;
                    write_register(memory,inst.register_x, sum);
                    write_register(memory,0xF, sum < reg_value ? 1 : 0);
                    break;
                case 5:
                    reg_value = read_register(memory, inst.register_x);
                    reg_value_y = read_register(memory, inst.register_y);
                    uint8_t sub = reg_value - reg_value_y;
                    write_register(memory,inst.register_x, sub);
                    write_register(memory,0xF, sub > reg_value ? 0 : 1);
                    break;
                case 6:
                    reg_value_y = read_register(memory, inst.register_y);
                    write_register(memory,inst.register_x, reg_value_y >> 1);
                    write_register(memory,0xF, reg_value_y & 0x1);
                    break;
                case 7:
                    reg_value = read_register(memory, inst.register_x);
                    reg_value_y = read_register(memory, inst.register_y);
                    sub = reg_value_y - reg_value;
                    write_register(memory,inst.register_x, sub);
                    write_register(memory,0xF, sub > reg_value_y ? 0 : 1);
                    break;
                case 0xE:
                    reg_value_y = read_register(memory, inst.register_y);
                    write_register(memory,inst.register_x, reg_value_y << 1);
                    write_register(memory,0xF, reg_value_y >> 7);
                    break;
                default: printf("Unknown instruction");
            }
        case 9:
            //9xy0	skip next opcode if Vx != Vy
            reg_value = read_register(memory, inst.register_x);
            reg_value_y = read_register(memory, inst.register_y);
            if (reg_value != reg_value_y) {
                memory->program_counter += 2;
            }
            break;
        case 0xA:
            memory->address_register = inst.address;
            break;
        case 0xB:
            uint16_t jump_addr = inst.address + (uint16_t) read_register(memory,0);
            jump_addr &= PROGRAM_END;
            memory->program_counter = jump_addr;

            break;
        case 0xC:

            write_register(memory,inst.register_x, vip_next_random_byte(memory) & inst.kk_value);
            break;
        case 0xD:

            break;
        


        default: printf("Instruction not found or implemented: %d", inst.opcode);
    }

}

void push_stack(struct chip8_mem *memory, uint8_t value) {
    if (memory->stack_pointer == STACK_LOW) {
        fprintf(stderr,"Stack overflow, aborting...");
        exit(-1);
        return;
    }

    mem_write(memory, --memory->stack_pointer, value);
}

uint8_t pop_stack(struct chip8_mem *memory) {
    if (memory->stack_pointer == STACK_TOP) {
        fprintf(stderr,"Stack underflow, aborting...");
        exit(-1);
        return 0;
    }

    return mem_read(memory, ++memory->stack_pointer);
}

uint8_t read_register(struct chip8_mem* memory, uint8_t reg_num) {
    if (reg_num > 0xF) {
        fprintf(stderr,"Invalid register index");
        exit(-1);
        return 0;
    }

    return mem_read(memory, 0xEF0 + (uint16_t) reg_num);
}

uint8_t vip_next_random_byte(struct chip8_mem *memory) {

    memory->seed = (uint16_t)(memory->seed + 1u);

    uint8_t lo = (uint8_t)(memory->seed & 0xFFu);
    uint8_t hi = (uint8_t)(memory->seed >> 8);

    uint8_t b = vip_random_table[lo];

    /* 01DE–01E0  D = M(RE) + R9.1 ; bit 8 of sum is DF */
    uint16_t sum = (uint16_t)(b + hi);

    /* 01E1  STR 6 — low byte of sum goes to VX (temporary) */
    uint8_t tmp = (uint8_t)(sum & 0xFFu);

    /* 01E2  SHRC — 9-bit shift right: DF enters bit 7 */
    uint8_t half = (uint8_t)(sum >> 1);

    /* 01E3–01E4  D = M(R6) + D ; carry discarded */
    uint8_t r = (uint8_t)(tmp + half);

    /* 01E5  PHI 9 — result becomes the new seed high byte; low byte untouched */
    memory->seed = (uint16_t)(((uint16_t)r << 8) | lo);

    /* 01E6–01E9  VX = r & kk is done by the caller */
    return r;

}

void write_register(struct chip8_mem* memory, uint8_t reg_num, uint8_t value) {
    if (reg_num > 0xF) {
        fprintf(stderr,"Invalid register index");
        exit(-1);
        return;
    }

    mem_write(memory, 0xEF0 + (uint16_t) reg_num, value);
}

int mem_check_bounds(const uint16_t address) {
    return address >= PROGRAM_START && address <= PROGRAM_END ? 1 : 0;
}


void clear_display(struct chip8_mem *memory) {
    memset(memory->display,0, sizeof(memory->display));
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
