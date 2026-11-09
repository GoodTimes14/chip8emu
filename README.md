## CHIP-8 Emulator
This is my implementation of the COSMAC VIP CHIP-8 interpreter.
I did not emulate the RCA CDP1802, this project emulates only the CHIP-8 interpreter and the memory layout
(Aside from the random emulation that takes a page of the original interpreter and emulates the R9 register behaviour)

### Building
Use these commands for building the project executable:
```
cmake -S . -B build #Creates the build folder
cmake --build build #Runs the build task
```

### Executing

```
./chip8emu --rom <your-desired-rom.ch8> #if the --rom option is not defined it searches for the logo debug rom 'chip8-logo.ch8' i've used for testing 
```

### Roadmap
[x] Full VIP CHIP-8 emulation
[ ] Compatibility options for SCHIP

### Documentation
The documentation i've used for this project is the following: 
- [https://chip8kb.gulrak.net/reference/variants/classic-chip-8/]
