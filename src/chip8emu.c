#include <stdio.h>
#include "chip8emu.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


int locked_key = -1;

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
    loop_program(memory);
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

void loop_program(struct chip8_mem *memory) {
    int64_t start = now_ns();
    int64_t frame = 0;

    for (;;) {
        int64_t due = (now_ns() - start) * FRAME_HZ / NS_PER_SEC;
        if (due - frame > MAX_CATCHUP_FRAMES) frame = due - MAX_CATCHUP_FRAMES;

        if (frame < due) {
            poll_inputs(memory);
            while (frame < due) {
                printf("Frame %lld started\n", (long long) frame);
                run_frame(memory);
                printf("Frame %lld finished\n", (long long) frame);
                frame++;
            }
            //Draw frame with SDL
        }

        sleep_until(start + (frame + 1) * NS_PER_SEC / FRAME_HZ);
    }
}

void run_frame(struct chip8_mem *memory) {

    for (int i = 0;i < CYCLES_PER_FRAME;i++) {
        struct chip8_instruction inst = decode_inst(fetch_inst(memory));
        
        printf("Executing instruction 0x%04X\n", inst.full_inst);
        memory->program_counter += 2;

        exec_inst(memory, inst);

        if (inst.opcode == 0xD) break;   // VIP: a sprite draw waits for vblank
    }


    if (memory->sound_timer != 0) memory->sound_timer--;
    if (memory->delay_timer != 0) memory->delay_timer--;
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
            memory->program_counter = inst.address;
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
            //Cxkk	set Vx to a random byte masked (bitwise AND) with kk

            write_register(memory,inst.register_x, vip_next_random_byte(memory) & inst.kk_value);
            break;
        case 0xD:
            //Dxyn	draw 8×n pixel graphics at position Vx & 63, Vy & 31 with data from memory, starting at the address in I, I is not changed
            reg_value = read_register(memory,inst.register_x) & 63;
            reg_value_y = read_register(memory,inst.register_x) & 31;
            uint8_t collision = 0;
            for (uint8_t row = 0; row < inst.n_value; row++) {
                uint8_t py = reg_value_y + row;
                if (py >= 32) break;

                uint8_t bits = mem_read(memory, memory->address_register + row & 0xFFF);
                for (uint8_t col = 0; col < 8; col++) {
                    uint8_t px = reg_value + col;
                    if (px >= 64) break;
                    uint8_t b = bits & (128 >> col);
                    if (b) {
                        if (memory->display[64 * py + px]) collision = 1;

                        memory->display[64 * py + px] ^= 1;
                    }
                }
            }

            write_register(memory,0xF, collision);
            break;
        case 0xE:
            reg_value = read_register(memory, inst.register_x);
            uint16_t pressed_value = (memory->keyboard >> (reg_value & 0xF)) & 1;
            uint16_t reg_expect = inst.kk_value == 0x9E ? 1 : 0;
            if (reg_expect == pressed_value) {
                memory->program_counter += 2;
            }
            break;
        case 0xF:

            switch (inst.kk_value) {
                case 0x0A:
                    if (locked_key == -1) {
                        for (int i = 0;i < 0xF;i++) {
                            uint16_t value = (memory->keyboard >> i) & 1;
                            if (value) locked_key = value; break;
                        }
                    }

                    if (locked_key != -1) {
                        if (!poll_key_state(memory, locked_key)) {
                            memory->program_counter -= 2;

                        } else {
                            write_register(memory, inst.register_x, locked_key);
                            locked_key = -1;
                        }
                    }
                    break;
                case 0x15:
                    reg_value = read_register(memory,inst.register_x);
                    memory->delay_timer = reg_value;

                    break;
                case 0x18:
                    reg_value = read_register(memory,inst.register_x);
                    memory->sound_timer = reg_value;

                    break;
                case 0x1E:
                    reg_value = read_register(memory,inst.register_x);
                    memory->address_register += reg_value;

                    break;
                case 0x29:
                    reg_value = read_register(memory,inst.register_x);
                    uint8_t offset = mem_read(memory, VIP_FONT_ADDR + (reg_value & 0xF));
                    memory->address_register = VIP_FONT_ADDR + offset;
                    
                    break;
                case 0x33:
                    //Fx33	write the value of Vx as BCD value to memory at the addresses I (hundreds), I+1 (tens) and I+2 (ones)
                    reg_value = read_register(memory,inst.register_x);
                    uint8_t hundreds = reg_value / 100;
                    uint8_t tens = (reg_value / 10) % 10;
                    uint8_t ones = reg_value % 10;

                    mem_write(memory,memory->address_register,hundreds);
                    mem_write(memory,memory->address_register + 1,tens);
                    mem_write(memory,memory->address_register + 2,ones);


                    break;
                case 0x55:

                    //Fx55	write the content of V0 to Vx at the memory pointed to by I, I is incremented by x+1
                    for (int i = 0;i <= inst.register_x;i++) {
                        reg_value = read_register(memory,i);
                        mem_write(memory,memory->address_register++, reg_value);
                    }

                    break;
                case 0x65:
                    //Fx65	read the bytes from memory pointed to by I into the registers V0 to Vx, I is incremented by x+1

                    for (int i = 0;i <= inst.register_x;i++) {
                        uint8_t read_value = mem_read(memory, memory->address_register++);
                        write_register(memory,i,read_value);
                    }
                    break;
            }

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

void poll_inputs(struct chip8_mem *memory) {

    //TODO: Aggiungere Poll degli input con SDL
    uint16_t keyvals = 0;
    for (int x = 0;x < 0xF;x++) {
        bool pressed = false;
        if (pressed) {
            keyvals |= 1 << x;
        }
    }

    memory->keyboard = keyvals;
}

uint16_t poll_key_state(struct chip8_mem *memory, uint8_t key_index) {
    return (memory->keyboard >> key_index) & 1;
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
