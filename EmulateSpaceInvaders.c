#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include "8080emulator.h"


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

typedef struct Ports {
    // input-read, output-write
    uint8_t input0, input1, input2, output2, output3, output5, output6;
    uint16_t shift_register;
    uint8_t shift_amount;
} Ports;

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
            state->a = ports->shift_register >> (8 - ports->shift_amount);
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
    }
}

void KeyDown(SDL_KeyCode key, Ports* ports)
{
    switch (key) {
        case SDLK_c:        // COIN
            ports->input1 |= 0x1;
            break;
        case SDLK_RETURN:   // 2P START
            printf("return");
            ports->input1 |= 0x02;
            break;
        case SDLK_1:        // 1P START
            ports->input1 |= 0x04;
            break;
        case SDLK_SPACE:    // 1P SHOOT
            ports->input1 |= 0x10;
            break;
        case SDLK_a:        // 1P LEFT
            ports->input1 |= 0x20;
            break;
        case SDLK_d:        // 1P RIGHT
            ports->input1 |= 0x40;
            break;

        case SDLK_UP:       // 2P SHOOT
            ports->input2 |= 0x10;
            break;
        case SDLK_LEFT:     // 2P LEFT
            ports->input2 |= 0x20;
            break;
        case SDLK_RIGHT:     // 2P RIGHT
            ports->input2 |= 0x40;
            break;
        default:
            break;
    }
}

void KeyUp(SDL_KeyCode key, Ports* ports)
{
    switch (key) {
        case SDLK_c:
            ports->input1 &= ~0x1;
            break;
        case SDLK_RETURN:
            ports->input1 &= ~0x2;
            break;
        case SDLK_1:
            ports->input1 &= ~0x4;
            break;
        case SDLK_SPACE:
            ports->input1 &= ~0x10;
            break;
        case SDLK_a:
            ports->input1 &= ~0x20;
            break;
        case SDLK_d:
            ports->input1 &= ~0x40;
            break;

        case SDLK_UP:       // 2P SHOOT
            ports->input2 &= ~0x10;
            break;
        case SDLK_LEFT:     // 2P LEFT
            ports->input2 &= ~0x20;
            break;
        case SDLK_RIGHT:     // 2P RIGHT
            ports->input2 &= ~0x40;
            break;
        default:
            break;
    }
}

void draw_screen(State8080* state, SDL_Renderer* renderer, int interrupt_num, SDL_Surface* surface) {
    uint8_t *video_memory = &state->memory[0x2400];  // Starting address for video memory
    int start_y = (interrupt_num == 0) ? 0 : 112;   // Top half or bottom half
    int end_y = (interrupt_num == 0) ? 112 : 224;




    uint32_t color;
    for (int y = start_y; y < end_y; y++) {
        for (int x = 0; x < 256; x++) {
            int byte_offset = y * 32 + x / 8;
            int bit_offset = x % 8;

            color = (video_memory[byte_offset] & (1 << bit_offset)) ? 0xFFFFFFFF : 0x00000000;
            ((uint32_t*)surface->pixels)[(255 - x) * 224 + y] = color;
        }
    }


    if (interrupt_num == 1) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        if (!texture) {
            printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
            return;
        }

        SDL_Rect destRect = { 0, 0, 224 * 3, 256 * 3};
        SDL_RenderCopy(renderer, texture, NULL, &destRect);

        // Clean up the texture
        SDL_DestroyTexture(texture);


        SDL_RenderPresent(renderer);
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
    if ((currentTime - lastInterrupt) > (16))
    {
        GenerateInterrupt(state, *interrupt_num + 1);
        draw_screen(state, renderer, *interrupt_num, surface);

        *interrupt_num ^= 1;    // toggles between 0-1
        return currentTime;
    }
    return lastInterrupt;
}




int main(int argc, char**argv)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_TIMER) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("8080 Emulator",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          256 * 3, 224 * 3, SDL_WINDOW_SHOWN);
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
    uint8_t interrupt_num = 0;

    // Read files into state[memory]
    ReadFileMem(state, "../Rom/invaders", 0);
//    ReadFileMem(state, "../Rom/invaders.h", 0);
//    ReadFileMem(state, "../Rom/invaders.g", 0x800);
//    ReadFileMem(state, "../Rom/invaders.f", 0x1000);
//    ReadFileMem(state, "../Rom/invaders.e", 0x1800);

    int line = 0;
//    while (state->pc < 0x2000)
//    while (line < 2000000)
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

        last_interrupt = HandleInterrupt(state, last_interrupt, &interrupt_num, renderer, surface);


        unsigned char *opcode = &state->memory[state->pc];
//        Disassemble8080Op(state->memory, state->pc);

        // Game has specific function for IN/OUT, which isn't in the general emulator function
        // IN
        if (*opcode == 0xdb)
        {
            uint8_t port = opcode[1];
            MachineIN(port, ports, state);
            state->pc += 2;
        }
        // OUT
        else if (*opcode == 0xd3)
        {
            uint8_t port = opcode[1];
            MachineOUT(port, ports, state);
            state->pc += 2;
        }
        else
            Emulate8080Op(state);




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
        line++;
    }
    return 0;
}