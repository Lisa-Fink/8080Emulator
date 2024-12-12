#include <stdio.h>
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
void SetFlagsNoCarry(ConditionCodes* cc, uint16_t answer);

void UnimplementedInstruction(State8080* state)
{
    //pc will have advanced one, so undo that
    printf ("Error: Unimplemented instruction\n");
    exit(1);
}

int Emulate8080Op(State8080* state)
{
    unsigned char *opcode = &state->memory[state->pc];

    switch(*opcode) {
        case 0x00:  // NOP
            break;
        case 0x01:  // LXI B, D16
            state->c = opcode[1];
            state->b = opcode[2];
            state->pc += 2;
            break;
        case 0x02:
            UnimplementedInstruction(state);
            break;
        case 0x03:  // INX  	BC <- BC+1
            state->c++;
            if (state->c == 0) state->b++;
            break;
        case 0x04:  // INR B	B <- B+1
            state->b++;
            SetFlagsNoCarry(&state->cc, state->b);
            break;
        case 0x05:  // DCR B    B <- B-1
            state->b--;
            SetFlagsNoCarry(&state->cc, state->b);
            break;
            /*....*/
        case 0x09:  // DAD B    HL = HL + BC
        {
            uint32_t hl = (state->h<<8) | (state->l);
            uint16_t bc = (state->b<<8) | (state->c);
            hl += bc;
            state->h = (hl>>8) & 0xff;
            state->l = hl & 0xff;
            state->cc.cy = (hl > 0xffff);   // only set carry
        }
            break;
            /*....*/
        case 0x0b:  // DCX  	BC <- BC-1
            if (state->c == 0) state->b--;
            state->c--;
            break;
        case 0x0c:  // INR C	C <- C+1
            state->c++;
            SetFlagsNoCarry(&state->cc, state->c);
            break;
        case 0x0d:  // DCR C    C <- C-1
            state->c--;
            SetFlagsNoCarry(&state->cc, state->c);
            break;
            /*....*/
        case 0x13:  // INX D 	DE <- DE+1
            state->e++;
            if (state->e == 0) state->d++;
            break;
        case 0x14:  // INR D	D <- D+1
            state->d++;
            SetFlagsNoCarry(&state->cc, state->d);
            break;
        case 0x15:  // DCR D    D <- D-1
            state->d--;
            SetFlagsNoCarry(&state->cc, state->d);
            break;
            /*....*/
        case 0x19:  // DAD B    HL = HL + DE
        {
            uint32_t hl = (state->h<<8) | (state->l);
            uint16_t de = (state->d<<8) | (state->e);
            hl += de;
            state->h = (hl>>8) & 0xff;
            state->l = hl & 0xff;
            state->cc.cy = (hl > 0xffff);   // only set carry
        }
            break;
            /*....*/
        case 0x1b:  // DCX D 	DE <- DE-1
            if (state->e == 0) state->d--;
            state->e--;
            break;
        case 0x1c:  // INR E	E <- E+1
            state->e++;
            SetFlagsNoCarry(&state->cc, state->e);
            break;
        case 0x1d:  // DCR E    E <- E-1
            state->e--;
            SetFlagsNoCarry(&state->cc, state->e);
            break;
            /*....*/ 
        case 0x23:  // INX H 	HL <- HL+1
            state->l++;
            if (state->l == 0) state->h++;
            break;
        case 0x24:  // INR H	H <- H+1
            state->h++;
            SetFlagsNoCarry(&state->cc, state->h);
            break;
        case 0x25:  // DCR H    H <- H-1
            state->h--;
            SetFlagsNoCarry(&state->cc, state->h);
            break;
            /*....*/
        case 0x29:  // DAD H    HL = HL + HL?
        {
            uint32_t hl = (state->h<<8) | (state->l);
            hl += hl;
            state->h = (hl>>8) & 0xff;
            state->l = hl & 0xff;
            state->cc.cy = (hl > 0xffff);   // only set carry
        }
            break;
            /*....*/
        case 0x2b:  // DCX H    HL = HL - 1
        {
            if (state->l == 0) state->h--;
            state->l--;
            break;
        }
        case 0x2c:  // INR L	L <- L+1
            state->l++;
            SetFlagsNoCarry(&state->cc, state->l);
            break;
        case 0x2d:  // DCR L    L <- L-1
            state->l--;
            SetFlagsNoCarry(&state->cc, state->l);
            break;
            /*....*/
        case 0x33:  // INX SP   SP = SP + 1
            state->sp++;
            break;
            /*....*/
        case 0x39:  // DAD SP   HL = HL + SP
        {
            uint32_t hl = (state->h << 8) | (state->l);
            hl += state->sp;
            state->h = (hl >> 8) & 0xff;
            state->l = hl & 0xff;
            state->cc.cy = (hl > 0xffff);   // only set carry
        }
            break;
            /*....*/
        case 0x3b:  // DCX SP   SP = SP - 1
            state->sp--;
        case 0x3c:  // INR A	A <- A+1
            state->a++;
            SetFlagsNoCarry(&state->cc, state->a);
            break;
        case 0x3d:  // DCR A    A <- A-1
            state->a--;
            SetFlagsNoCarry(&state->cc, state->a);
            break;
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
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->b + state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x89:   // ADC C
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->c + state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x8a:   // ADC D
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->d + state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x8b:  // ADC E
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->e + state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x8c:  // ADC H
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->h + state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x8d:  // ADC L
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->l + state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x8e:  // ADC M
        {
            uint16_t address = (state->h<<8) | (state->l);  // concat h and l
            uint16_t answer = (uint16_t) state->a + state->memory[address] + state->cc.cy;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x8f:  // ADC A
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->a + state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x90:  // SUB B
        {
            // store result in 16 bit answer to check for carry
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->b;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;   // remove extra 8 bits
        }
            break;
        case 0x91:  // SUB C
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->c;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x92:  // SUB D
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->d;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x93:  // SUB E
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->e;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x94:  // SUB H
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->h;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x95:  // SUB L
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->l;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x96:  // SUB M
        {
            // M is the byte pointed to by SUBress stored in HL
            uint16_t SUBress = (state->h<<8) | (state->l);  // concat h and l
            uint16_t answer = (uint16_t) state->a - state->memory[SUBress];
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x97:  // SUB A
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->a;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x98:  // 	SBB B
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->b - state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x99:   // SBB C
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->c - state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9a:   // SBB D
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->d - state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9b:  // SBB E
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->e - state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9c:  // SBB H
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->h - state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9d:  // SBB L
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->l - state->cc.cy;;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9e:  // SBB M
        {
            uint16_t address = (state->h<<8) | (state->l);  // concat h and l
            uint16_t answer = (uint16_t) state->a - state->memory[address] - state->cc.cy;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9f:  // SBB A
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->a - state->cc.cy;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
            /*...*/
        case 0xc3:  // ACI D8 (A <- A + data + CY)
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1] + state->cc.cy;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
            /*...*/
        case 0xc6:  // ADI D8 (A <- A + byte)
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1];
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
            /*...*/
            /*...*/
        case 0xd6:  // SUI D8 (A <- A - data)
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) opcode[1];
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
            /*...*/
        case 0xfe: UnimplementedInstruction(state); break;
        case 0xff: UnimplementedInstruction(state); break;
    }
    state->pc+=1;
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

void SetFlagsNoCarry(ConditionCodes* cc, uint16_t answer)
{
    // update flags Z, S, P, CY, AC (skip AC because Space Invaders doesn't use it)
    cc->z = ((answer & 0xff) == 0); // zero: check lowest 8 bits equals 0
    cc->s = ((answer & 0x80) != 0); // sign: check bit 7 0x80 = 0b1000 0000
    cc->p = Parity(answer&0xff);
}



