#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


typedef struct ConditionCodes {
    uint8_t    z:1;
    uint8_t    s:1;
    uint8_t    p:1;
    uint8_t    cy:1;
    uint8_t    ac:1;
    uint8_t    pad:3;
} ConditionCodes;

typedef struct State8080 {
    uint8_t    a;
    uint8_t    b;
    uint8_t    c;
    uint8_t    d;
    uint8_t    e;
    uint8_t    h;
    uint8_t    l;
    uint16_t    sp;
    uint16_t    pc;
    uint8_t     *memory;
    struct      ConditionCodes      cc;
    uint8_t     int_enable;
} State8080;

void SetFlags(ConditionCodes* cc, uint16_t answer);

void UnimplementedInstruction(State8080* state)
{
    //pc will have advanced one, so undo that
    printf ("Error: Unimplemented instruction\n");
    exit(1);
}

int Emulate8080Op(State8080* state)
{
    unsigned char *opcode = &state->memory[state->pc];

    switch(*opcode)
    {
        case 0x00:  // NOP
            break;
        case 0x01:  // LXI B, D16
            state->c = opcode[1];
            state->b = opcode[2];
            state->pc += 2;
            break;
        case 0x02: UnimplementedInstruction(state); break;
        case 0x03: UnimplementedInstruction(state); break;
        case 0x04: UnimplementedInstruction(state); break;
            /*....*/
        case 0x80:  // ADD B
        {
            // store result in 16 bit answer to check for carry
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;   // remove extra 8 bits
        }
            break;
        case 0x81:  // ADD C
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->c;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x82:  // ADD D
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->d;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x83:  // ADD E
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->e;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x84:  // ADD H
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->h;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x85:  // ADD L
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->l;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x86:  // ADD M
        {
            // M is the byte pointed to by address stored in HL
            uint16_t address = (state->h<<8) | (state->l);  // concat h and l
            uint16_t answer = (uint16_t) state->a + state->memory[address];
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x87:  // ADD A
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->a;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x88:  // 	ADC B
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;
            SetFlags(&state->cc, answer);
            state->a = (answer & 0xff) + state->cc.cy;
        }
            break;
        case 0x89:   // ADC C
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->c;
            SetFlags(&state->cc, answer);
            state->a = (answer & 0xff) + state->cc.cy;
        }
            break;
        case 0x8a:   // ADC D
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->d;
            SetFlags(&state->cc, answer);
            state->a = (answer & 0xff) + state->cc.cy;
        }
            break;
        case 0x8b:  // ADC E
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->e;
            SetFlags(&state->cc, answer);
            state->a = (answer & 0xff) + state->cc.cy;
        }
            break;
        case 0x8c:  // ADC H
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->h;
            SetFlags(&state->cc, answer);
            state->a = (answer & 0xff) + state->cc.cy;
        }
            break;
        case 0x8d:  // ADC L
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->l;
            SetFlags(&state->cc, answer);
            state->a = (answer & 0xff) + state->cc.cy;
        }
            break;
        case 0x8e:  // ADC M
        {
            uint16_t address = (state->h<<8) | (state->l);  // concat h and l
            uint16_t answer = (uint16_t) state->a + state->memory[address];
            SetFlags(&state->cc, answer);
            state->a = (answer & 0xff) + state->cc.cy;
        }
            break;
        case 0x8f:  // ADC A
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->a;
            SetFlags(&state->cc, answer);
            state->a = (answer & 0xff) + state->cc.cy;
        }
            /*...*/
        case 0xfe: UnimplementedInstruction(state); break;
        case 0xff: UnimplementedInstruction(state); break;
    }
    state->pc+=1;  //for the opcode
}

uint8_t Parity(uint8_t answer)
{
    return 0;
}

void SetFlags(ConditionCodes* cc, uint16_t answer)
{
    // update flags Z, S, P, CY, AC (skip AC because Space Invaders doesn't use it)
    cc->z = ((answer & 0xff) == 0); // zero: check lowest 8 bits equals 0
    cc->s = ((answer & 0x80) != 0); // sign: check bit 7 0x80 = 0b1000 0000
    cc->p = Parity(answer&0xff);
    cc->cy = (answer > 0xff);       // carry: check if it overflowed over 8 bits
}




