#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

int main (int argc, char**argv)
{
    // Initialize states
    State8080* state = calloc(1, sizeof(State8080));
    state->memory = malloc(0x10000);  // 16K

    // Read files into state[memory]
    ReadFileMem(state, "../Rom/invaders.h", 0);
    ReadFileMem(state, "../Rom/invaders.g", 0x800);
    ReadFileMem(state, "../Rom/invaders.f", 0x1000);
    ReadFileMem(state, "../Rom/invaders.e", 0x1800);

    int line = 0;
//    while (state->pc < 0x2000)
    while (line < 50000)
    {
        Emulate8080Op(state);
        // Print for debugging
        printf("\t");
        printf("%c", state->cc.z ? 'z' : '.');
        printf("%c", state->cc.s ? 's' : '.');
        printf("%c", state->cc.p ? 'p' : '.');
        printf("%c", state->cc.cy ? 'c' : '.');
        printf("%c  ", state->cc.ac ? 'a' : '.');
        printf("PC $%02x ", state->pc);
        printf("A $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x\n", state->a, state->b, state->c,
               state->d, state->e, state->h, state->l, state->sp);
        fflush(stdout);
        line++;
    }
    return 0;
}