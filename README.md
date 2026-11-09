## CHIP-8 Emulator
This is my implementation of the COSMAC VIP CHIP-8 interpreter.
I did not emulate the RCA CDP1802 (this means that i did not emulate the 0mmm instructions too) , this project emulates only the CHIP-8 interpreter and the memory layout
(Aside from the random emulation that takes a page of the original interpreter and emulates the R9 register behaviour)

### Requirements

- GCC 11 or newer
- CMake 4.3 or newer
- SDL3

Currently Linux only

### Building
Use these commands for building the project executable:
```
cmake -S . -B build #Creates the build folder
cmake --build build #Runs the build task
```

### Executing

```
./build/bin/chip8emu --rom <your-desired-rom.ch8> #if the --rom option is not defined it searches for the logo debug rom 'chip8-logo.ch8' i've used for testing 
```

### Controls

The CHIP-8 hex keypad is mapped to the left side of the keyboard:

| CHIP-8 keypad | Keyboard |
|:-------------:|:--------:|
| `1 2 3 C`     | `1 2 3 4` |
| `4 5 6 D`     | `Q W E R` |
| `7 8 9 E`     | `A S D F` |
| `A 0 B F`     | `Z X C V` |


### Roadmap
- [x] Full VIP CHIP-8 emulation
- [ ] Compatibility options for SCHIP

### Documentation
The documentation i've used for this project is the following: 
- [Gulrak's CHIP-8 knowledge base](https://chip8kb.gulrak.net/reference/variants/classic-chip-8/)
