#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include "8080emulator.h"
#include "ports.h"
#include "sound.h"
#include "input.h"
#include "graphics.h"

void ReadFileMem(State8080* state, char* filename, uint32_t mem_address)
{
    FILE *f= fopen(filename, "rb");
    if (f==NULL)
    {
        printf("error: Couldn't open %s\n", filename);
        exit(1);
    }
    fseek(f, 0L, SEEK_END);
    int file_size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    uint8_t *buffer = &state->memory[mem_address];
    fread(buffer, file_size, 1, f);
    fclose(f);
}


void InitPorts(Ports* ports)
{
    ports->input0 = 0b00001110; // Bits 1, 2, 3 are always 1; other inputs are default 0.
    ports->input1 = 0b00001000; // Bit 3 is always 1; all others default to 0.
    ports->input2 = 0b00000000;

    ports->shift_register = 0x0000;
    ports->shift_amount = 0;
    ports->output2 = 0;
    ports->output3 = 0;
    ports->output5 = 0;
    ports->output6 = 0;
}

void MachineIN(uint8_t port, Ports* ports, State8080* state)
{
    switch (port) {
        case 0:
            state->a = ports->input0;
            break;
        case 1:
            state->a = ports->input1;
            break;
        case 2:
            state->a = ports->input2;
            break;
        case 3:
            // read shift register with shifted amount
            state->a = ((ports->shift_register >> (8 - ports->shift_amount)) & 0xff);
            break;
        default:
            break;
    }
}
void MachineOUT(uint8_t port, Ports* ports, State8080* state)
{
    switch (port) {
        case 2:
            ports->shift_amount = state->a & 0x7;
            break;
        case 3:
            ports->output3 = state->a;
            break;
        case 4:
            // shift register moves left half to right side, and place new value on left side
            ports->shift_register = (state->a << 8) | (ports->shift_register >> 8);
            break;
        case 5:
            ports->output5 = state->a;
            break;
        case 6:
            ports->output6 = state->a;
            break;
        default:
            break;
    }
}

void GenerateInterrupt(State8080* state, int interrupt_num)
{
    // PUSH PC
    state->memory[state->sp-1] = (state->pc & 0xFF00) >> 8;
    state->memory[state->sp-2] = state->pc & 0xff;
    state->sp = state->sp - 2;

    // Set the PC to the low memory vector
    state->pc = 8 * interrupt_num;

    state->int_enable = 0;  // DI
}

double HandleInterrupt(State8080* state, Uint32 lastInterrupt, uint8_t* interrupt_num, SDL_Renderer* renderer, SDL_Surface* surface)
{
    if (state->int_enable == 0) return lastInterrupt;
    Uint32 currentTime = SDL_GetTicks64();
    if ((currentTime - lastInterrupt) > (8))
    {
        GenerateInterrupt(state, *interrupt_num + 1);
        draw_screen(state, renderer, *interrupt_num, surface);

        *interrupt_num ^= 1;    // toggles between 0-1
        return currentTime;
    }
    return lastInterrupt;
}

int RunCPUCycles(State8080* state, int last_processing, Ports* ports)
{
    // Stay here for a few cycles to keep up
    int cycles_per_frame = 3000;
    int elapsed_time = (int) SDL_GetTicks64() - last_processing;
    int cycles = (int) (elapsed_time * cycles_per_frame);
    unsigned char *opcode;
    if (cycles == 0) return last_processing;
    while (cycles > 0)
    {
        opcode = &state->memory[state->pc];
//        Disassemble8080Op(state->memory, state->pc);

        // Game has specific function for IN/OUT, which isn't in the general emulator function
        // IN
        if (*opcode == 0xdb) {
            uint8_t port = opcode[1];
            MachineIN(port, ports, state);
            state->pc += 2;
        }
        // OUT
        else if (*opcode == 0xd3) {
            uint8_t port = opcode[1];
            uint8_t old_bits;
            if (port == 3) old_bits = ports->output3;
            else if (port == 5) old_bits = ports->output5;
            MachineOUT(port, ports, state);
            state->pc += 2;
            if (port == 3 || port == 5) PlaySounds(ports, port, old_bits);
        } else
            Emulate8080Op(state);
        cycles -= cycles8080[*opcode];
    }

    return (int) SDL_GetTicks64();
}


int main(int argc, char**argv)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("8080 Emulator",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          224 * 3, 256 * 3, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 224, 256, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        printf("SDL_CreateSurface Error: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize states
    State8080* state = calloc(1, sizeof(State8080));
    state->memory = malloc(0x10000);  // 16K

    Ports* ports = calloc(1, sizeof(Ports));
    InitPorts(ports);

    Uint32 last_interrupt = SDL_GetTicks64();
    Uint32 last_processing = last_interrupt;
    uint8_t interrupt_num = 0;

    // Read files into state[memory]
    ReadFileMem(state, "../Rom/invaders", 0);
//    ReadFileMem(state, "../Rom/invaders.h", 0);
//    ReadFileMem(state, "../Rom/invaders.g", 0x800);
//    ReadFileMem(state, "../Rom/invaders.f", 0x1000);
//    ReadFileMem(state, "../Rom/invaders.e", 0x1800);

    SDL_Event event;
    int running = 1;
    while (running == 1)
    {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_KEYDOWN)
            {
                KeyDown(event.key.keysym.sym, ports);
            }
            if (event.type == SDL_KEYUP)
            {
                KeyUp(event.key.keysym.sym, ports);
            }
        }

        last_interrupt = (int) HandleInterrupt(state, last_interrupt, &interrupt_num, renderer, surface);
        last_processing = RunCPUCycles(state, (int) last_processing, ports);

//        // Print for debugging
//        printf("\t");
//        printf("%c", state->cc.z ? 'z' : '.');
//        printf("%c", state->cc.s ? 's' : '.');
//        printf("%c", state->cc.p ? 'p' : '.');
//        printf("%c", state->cc.cy ? 'c' : '.');
//        printf("%c  ", state->cc.ac ? 'a' : '.');
//        printf("PC $%02x ", state->pc);
//        printf("A $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x\n", state->a, state->b, state->c,
//               state->d, state->e, state->h, state->l, state->sp);
//        fflush(stdout);
    }
    return 0;
}
