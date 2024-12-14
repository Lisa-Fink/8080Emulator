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



int main (int argc, char**argv)
{
    // Initialize states
    State8080* state = calloc(1, sizeof(State8080));
    state->memory = malloc(0x10000);  // 16K

    Ports* ports = calloc(1, sizeof(Ports));
    InitPorts(ports);

    // Read files into state[memory]
    ReadFileMem(state, "../Rom/invaders.h", 0);
    ReadFileMem(state, "../Rom/invaders.g", 0x800);
    ReadFileMem(state, "../Rom/invaders.f", 0x1000);
    ReadFileMem(state, "../Rom/invaders.e", 0x1800);

    int line = 0;
//    while (state->pc < 0x2000)
    while (line < 50000)
    {
        unsigned char *opcode = &state->memory[state->pc];

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