#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "Disassembler/disassembler.h"


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
void SetFlagsSubtraction(ConditionCodes* cc, uint16_t answer);
void SetFlagsNoCarry(ConditionCodes* cc, uint16_t answer);
void CallAdr(State8080* state, const unsigned char *opcode);
void CallConstantAdr(State8080* state, uint8_t adr);
void Return(State8080* state);

void UnimplementedInstruction(State8080* state)
{
    //pc will have advanced one, so undo that
    printf ("Error: Unimplemented instruction\n");
    exit(1);
}

int Emulate8080Op(State8080* state)
{
    unsigned char *opcode = &state->memory[state->pc];

    Disassemble8080Op(state->memory, state->pc);

    // default inc pc
    // do this BEFORE processing
    // some operations will set pc, so pc shouldn't get changed after that
    state->pc+=1;

    switch(*opcode) {
        case 0x00:  // NOP
            break;
        case 0x01:  // LXI B, D16   B <- byte 3, C <- byte 2
            state->c = opcode[1];
            state->b = opcode[2];
            state->pc += 2;
            break;
        case 0x02:  // STAX B   (BC) <- A
        {
            uint16_t address = (state->b << 8) | state->c;
            state->memory[address] = state->a;
        }
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
        case 0x06:  // MVI B, D8    B <- byte2
            state->b = opcode[1];
            state->pc++;
            break;
        case 0x07:  // RLC  	A = A << 1; bit 0 = prev bit 7; CY = prev bit 7
        {
            uint8_t a = state->a;
            // bit 7 wraps around to bit 0, everything else goes left
            state->a = ((a & 0x80) >> 7) | (a << 1);
            state->cc.cy = state->a & 1;
        }
            break;
        case 0x08:  // NOP
            break;
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
        case 0x0a:  // LDAX B   A <- (BC)
        {
            uint16_t address = (state->b << 8) | state->c;
            state->a = state->memory[address];
        }
            break;
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
        case 0x0e:  // MVI C, D8    C <- byte 2
            state->c = opcode[1];
            state->pc++;
            break;
        case 0x0f:  // RRC      A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0
        {
            uint8_t a = state->a;
            // bit 0 wraps around to bit 7, everything else goes right
            state->a = ((a & 1) << 7) | (a >> 1);
            state->cc.cy = a & 1;
        }
            break;
        case 0x10:  // NOP
            break;
        case 0x11:  // LXI D, D16   D <- byte 3, E <- byte 2
            state->e = opcode[1];
            state->d = opcode[2];
            state->pc += 2;
            break;
        case 0x12:  // STAX D   (DE) <- A
        {
            uint16_t address = (state->d << 8) | state->e;
            state->memory[address] = state->a;
        }
            break;
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
        case 0x16:  // MVI D, D8    D <- byte2
            state->d = opcode[1];
            state->pc++;
            break;
        case 0x17:  // RAL  	A = A << 1; bit 0 = prev CY; CY = prev bit 7
        {
            uint8_t a = state->a;
            // bit 0 is prev carry, everything else goes left
            state->a = ((state->cc.cy) >> 7) | (a << 1);
            state->cc.cy = (a & 0x80) == 0x80;
        }
            break;
        case 0x18:  // NOP
            break;
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
        case 0x1a:  // LDAX D   A <- (DE)
        {
            uint16_t address = (state->d << 8) | state->e;
            state->a = state->memory[address];
        }
            break;
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
        case 0x1e:  // MVI E, D8    E <- byte2
            state->e = opcode[1];
            state->pc++;
            break;
        case 0x1f:  // RAR      A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0
        // TODO: double check this, example uses carry but instruction says prev bit 7
        {
            uint8_t a = state->a;
            state->a = (a & 0x80) | (a >> 1);
            state->cc.cy = a & 1;
        }
            break;
        case 0x20:  // NOP
            break;
        case 0x21:  // LXI H, D16   H <- byte 3, L <- byte 2
            state->l = opcode[1];
            state->h = opcode[2];
            state->pc += 2;
            break;
        case 0x22:  // 	SHLD adr    (adr) <-L; (adr+1)<-H
        {
            uint16_t address = (opcode[2] << 8) | opcode[1];
            state->memory[address] = state->l;
            state->memory[address + 1] = state->h;
            state->pc += 2;
        }
            break;
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
        case 0x26:  // MVI H, D8    H <- byte2
            state->h = opcode[1];
            state->pc++;
            break;
        case 0x27:  // DAA special
            // TODO: lookup?
            UnimplementedInstruction(state);
            break;
        case 0x28:  // NOP
            break;
        case 0x29:  // DAD H    HL = HL + HL?
        {
            uint32_t hl = (state->h<<8) | (state->l);
            hl += hl;
            state->h = (hl>>8) & 0xff;
            state->l = hl & 0xff;
            state->cc.cy = (hl > 0xffff);   // only set carry
        }
            break;
        case 0x2a:  // LHLD adr L <- (adr); H <- (adr + 1)
        {
            uint16_t address = (opcode[2] << 8) | opcode[1];
            state->l = state->memory[address];
            state->h = state->memory[address + 1];
            state->pc += 2;
        }
            break;
        case 0x2b:  // DCX H    HL = HL - 1
        {
            if (state->l == 0) state->h--;
            state->l--;
        }
            break;
        case 0x2c:  // INR L	L <- L+1
            state->l++;
            SetFlagsNoCarry(&state->cc, state->l);
            break;
        case 0x2d:  // DCR L    L <- L-1
            state->l--;
            SetFlagsNoCarry(&state->cc, state->l);
            break;
        case 0x2e:  // MVI L, D8    L <- byte2
            state->l = opcode[1];
            state->pc++;
            break;
        case 0x2f:  // CMA  not  (A<-!A)
            state->a = ~state->a;
            break;
        case 0x30:  // NOP
            break;
        case 0x31:  // LXI SP, D16  SP.hi <- byte 3, SP.lo <- byte 2
        {
            uint16_t address = (opcode[2] << 8) | opcode[1];
            state->sp = address;
            state->pc += 2;
        }
            break;
        case 0x32:  // STA adr  (adr) <- A
        {

            uint16_t address = (opcode[2] << 8) | opcode[1];
            state->memory[address] = state->a;
            state->pc += 2;
        }
            break;
        case 0x33:  // INX SP   SP = SP + 1
            state->sp++;
            break;
        case 0x34:  // INR M    (HL) <- (HL) + 1
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] += 1;
            SetFlagsNoCarry(&state->cc, state->memory[address]);
        }
            break;
        case 0x35:  // DCR M    (HL) <- (HL) - 1
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] -= 1;
            SetFlagsNoCarry(&state->cc, state->memory[address]);
        }
            break;
        case 0x36:  // MVI M, D8    (HL) <- byte2
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] = opcode[1];
            state->pc++;
        }
            break;
        case 0x37:  // STC      CY = 1
            state->cc.cy = 1;
            break;
        case 0x38:  // NOP
            break;
        case 0x39:  // DAD SP   HL = HL + SP
        {
            uint32_t hl = (state->h << 8) | (state->l);
            hl += state->sp;
            state->h = (hl >> 8) & 0xff;
            state->l = hl & 0xff;
            state->cc.cy = (hl > 0xffff);   // only set carry
        }
            break;
        case 0x3a:  // LDA adr  A <- (adr)
        {

            uint16_t address = (opcode[2] << 8) | opcode[1];
            state->a = state->memory[address];
            state->pc += 2;
        }
            break;
        case 0x3b:  // DCX SP   SP = SP - 1
            state->sp--;
            break;
        case 0x3c:  // INR A	A <- A+1
            state->a++;
            SetFlagsNoCarry(&state->cc, state->a);
            break;
        case 0x3d:  // DCR A    A <- A-1
            state->a--;
            SetFlagsNoCarry(&state->cc, state->a);
            break;
        case 0x3e:  // MVI A, D8    A <- byte2
            state->a = opcode[1];
            state->pc++;
            break;
        case 0x3f:  // CMC  not  (CY<-!CY)
            state->cc.cy = ~state->cc.cy;
            break;
        case 0x40:  // MOV B, B     B <- B
            state->b = state->b;
            break;
        case 0x41:  // MOV B, C
            state->b = state->c;
            break;
        case 0x42:  // MOV B, D
            state->b = state->d;
            break;
        case 0x43:  // MOV B, E
            state->b = state->e;
            break;
        case 0x44:  // MOV B, H
            state->b = state->h;
            break;
        case 0x45:  // MOV B, L
            state->b = state->l;
            break;
        case 0x46:  // MOV B, M     B <- (HL)
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->b = state->memory[address];
        }
            break;
        case 0x47:  // MOV B, A
            state->b = state->a;
            break;
        case 0x48:  // MOV C, B     C <- B
            state->c = state->b;
            break;
        case 0x49:  // MOV C, C
            state->c = state->c;
            break;
        case 0x4a:  // MOV C, D
            state->c = state->d;
            break;
        case 0x4b:  // MOV C, E
            state->c = state->e;
            break;
        case 0x4c:  // MOV C, H
            state->c = state->h;
            break;
        case 0x4d:  // MOV C, L
            state->c = state->l;
            break;
        case 0x4e:  // MOV C, M     C <- (HL)
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->c = state->memory[address];
        }
            break;
        case 0x4f:  // MOV C, A
            state->c = state->a;
            break;
        case 0x50:  // MOV D, B
            state->d = state->b;
            break;
        case 0x51:  // MOV D, C
            state->d = state->c;
            break;
        case 0x52:  // MOV D, D
            state->d = state->d;
            break;
        case 0x53:  // MOV D, E
            state->d = state->e;
            break;
        case 0x54:  // MOV D, H
            state->d = state->h;
            break;
        case 0x55:  // MOV D, L
            state->d = state->l;
            break;
        case 0x56:  // MOV D, M
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->d = state->memory[address];
        }
            break;
        case 0x57:  // MOV D, A
            state->d = state->a;
            break;
        case 0x58:  // MOV E, B
            state->e = state->b;
            break;
        case 0x59:  // MOV E, C
            state->e = state->c;
            break;
        case 0x5a:  // MOV E, D
            state->e = state->d;
            break;
        case 0x5b:  // MOV E, E
            state->e = state->e;
            break;
        case 0x5c:  // MOV E, H
            state->e = state->h;
            break;
        case 0x5d:  // MOV E, L
            state->e = state->l;
            break;
        case 0x5e:  // MOV E, M
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->e = state->memory[address];
        }
            break;
        case 0x5f:  // MOV E, A
            state->e = state->a;
            break;
        case 0x60:  // MOV H, B
            state->h = state->b;
            break;
        case 0x61:  // MOV H, C
            state->h = state->c;
            break;
        case 0x62:  // MOV H, D
            state->h = state->d;
            break;
        case 0x63:  // MOV H, E
            state->h = state->e;
            break;
        case 0x64:  // MOV H, H
            state->h = state->h;
            break;
        case 0x65:  // MOV H, L
            state->h = state->l;
            break;
        case 0x66:  // MOV H, M
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->h = state->memory[address];
        }
            break;
        case 0x67:  // MOV H, A
            state->h = state->a;
            break;
        case 0x68:  // MOV L, B
            state->l = state->b;
            break;
        case 0x69:  // MOV L, C
            state->l = state->c;
            break;
        case 0x6a:  // MOV L, D
            state->l = state->d;
            break;
        case 0x6b:  // MOV L, E
            state->l = state->e;
            break;
        case 0x6c:  // MOV L, H
            state->l = state->h;
            break;
        case 0x6d:  // MOV L, L
            state->l = state->l;
            break;
        case 0x6e:  // MOV L, M
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->l = state->memory[address];
        }
            break;
        case 0x6f:  // MOV L, A
            state->l = state->a;
            break;
        case 0x70:  // MOV M, B
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] = state->b;
            break;
        }
        case 0x71:  // MOV M, C
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] = state->c;
            break;
        }
        case 0x72:  // MOV M, D
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] = state->d;
            break;
        }
        case 0x73:  // MOV M, E
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] = state->e;
            break;
        }
        case 0x74:  // MOV M, H
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] = state->h;
            break;
        }
        case 0x75:  // MOV M, L
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] = state->l;
            break;
        }
        case 0x76:  // HLT special
            exit(0);
        case 0x77:  // MOV M, A
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->memory[address] = state->a;
        }
            break;
        case 0x78:  // MOV A, B     A <- B
            state->a = state->b;
            break;
        case 0x79:  // MOV A, C
            state->a = state->c;
            break;
        case 0x7a:  // MOV A, D
            state->a = state->d;
            break;
        case 0x7b:  // MOV A, E
            state->a = state->e;
            break;
        case 0x7c:  // MOV A, H
            state->a = state->h;
            break;
        case 0x7d:  // MOV A, L
            state->a = state->l;
            break;
        case 0x7e:  // MOV A, M     A <- (HL)
        {
            uint16_t address = (state->h << 8) | (state->l);
            state->a = state->memory[address];
        }
            break;
        case 0x7f:  // MOV A, A
            state->a = state->a;
            break;
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
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;   // remove extra 8 bits
        }
            break;
        case 0x91:  // SUB C
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->c;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x92:  // SUB D
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->d;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x93:  // SUB E
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->e;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x94:  // SUB H
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->h;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x95:  // SUB L
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->l;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x96:  // SUB M
        {
            // M is the byte pointed to by SUBress stored in HL
            uint16_t SUBress = (state->h<<8) | (state->l);  // concat h and l
            uint16_t answer = (uint16_t) state->a - state->memory[SUBress];
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x97:  // SUB A
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->a;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x98:  // 	SBB B
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->b - state->cc.cy;;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x99:   // SBB C
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->c - state->cc.cy;;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9a:   // SBB D
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->d - state->cc.cy;;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9b:  // SBB E
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->e - state->cc.cy;;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9c:  // SBB H
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->h - state->cc.cy;;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9d:  // SBB L
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->l - state->cc.cy;;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9e:  // SBB M
        {
            uint16_t address = (state->h<<8) | (state->l);  // concat h and l
            uint16_t answer = (uint16_t) state->a - state->memory[address] - state->cc.cy;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0x9f:  // SBB A
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->a - state->cc.cy;
            SetFlagsSubtraction(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0xa0:  // ANA B   (A <-A & B)
            state->a &= state->b;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;   // AND always clears CY
            break;
        case 0xa1:  // ANA C   (A <-A & C)
            state->a &= state->c;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xa2:  // ANA D   (A <-A & D)
            state->a &= state->d;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xa3:  // ANA E   (A <-A & E)
            state->a &= state->e;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xa4:  // ANA H   (A <-A & H)
            state->a &= state->h;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xa5:  // ANA L   (A <-A & L)
            state->a &= state->l;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xa6:  // ANA M   (A <-A & (HL))
        {
            uint16_t address = (state->h << 8) | (state->l);  // concat h and l
            state->a &= state->memory[address];
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
        }
            break;
        case 0xa7:  // ANA A   (A <-A & A)
            // Don't need to update A because A & A doesn't change A
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xa8:  // XRA B   (A <-A ^ B)
            state->a ^= state->b;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;   // AND always clears CY
            break;
        case 0xa9:  // XRA C   (A <-A ^ C)
            state->a ^= state->c;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xaa:  // XRA D   (A <-A ^ D)
            state->a ^= state->d;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xab:  // XRA E   (A <-A ^ E)
            state->a ^= state->e;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xac:  // XRA H   (A <-A ^ H)
            state->a ^= state->h;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xad:  // XRA L   (A <-A ^ L)
            state->a ^= state->l;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xae:  // XRA M   (A <-A ^ (HL))
        {
            uint16_t address = (state->h << 8) | (state->l);  // concat h and l
            state->a ^= state->memory[address];
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
        }
            break;
        case 0xaf:  // XRA A   (A <-A ^ A)
            state->a = 0;   // xor itself is always 0
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xb0:  // ORA B   (A <-A | B)
            state->a |= state->b;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;   // AND always clears CY
            break;
        case 0xb1:  // ORA C   (A <-A | C)
            state->a |= state->c;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xb2:  // ORA D   (A <-A | D)
            state->a |= state->d;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xb3:  // ORA E   (A <-A | E)
            state->a |= state->e;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xb4:  // ORA H   (A <-A | H)
            state->a |= state->h;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xb5:  // ORA L   (A <-A | L)
            state->a |= state->l;
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xb6:  // ORA M   (A <-A | (HL))
        {
            uint16_t address = (state->h << 8) | (state->l);  // concat h and l
            state->a |= state->memory[address];
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
        }
            break;
        case 0xb7:  // ORA A   (A <-A | A)
            // Don't need to update A because A | A doesn't change A
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            break;
        case 0xb8:  // CMP B    (A - B)
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->b;
            SetFlagsSubtraction(&state->cc, answer);
        }
            break;
        case 0xb9:  // CMP C    (A - C)
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->c;
            SetFlagsSubtraction(&state->cc, answer);
        }
            break;
        case 0xba:  // CMP D    (A - D)
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->d;
            SetFlagsSubtraction(&state->cc, answer);
        }
            break;
        case 0xbb:  // CMP E    (A - E)
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->e;
            SetFlagsSubtraction(&state->cc, answer);
        }
            break;
        case 0xbc:  // CMP H    (A - H)
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->h;
            SetFlagsSubtraction(&state->cc, answer);
        }
            break;
        case 0xbd:  // CMP L    (A - L)
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->l;
            SetFlagsSubtraction(&state->cc, answer);
        }
            break;
        case 0xbe:  // CMP M    (A - (HL))
        {
            uint16_t address = (state->h << 8) | (state->l);  // concat h and l
            uint16_t answer = (uint16_t) state->a - (uint16_t) state->memory[address];
            SetFlagsSubtraction(&state->cc, answer);
        }
            break;
        case 0xbf:  // CMP A    (A - A)
            SetFlagsSubtraction(&state->cc, 0);    // A - A is always 0
            break;
        case 0xc0:  // RNZ adr  (if NZ, RET)
            if (state->cc.z == 0)
                Return(state);
            break;
        case 0xc1:  // POP B    C <- (sp); B <- (sp+1); sp <- sp+2
        {
            state->c = state->memory[state->sp];
            state->b = state->memory[state->sp+1];
            state->sp += 2;
        }
            break;
        case 0xc2:  // JNZ adr  (if NZ, PC <- adr)
            if (state->cc.z == 0)
                    state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xc3:  // JMP adr  (PC <- adr)
            state->pc = (opcode[2] << 8) | opcode[1];
            break;
        case 0xc4:  // CNZ adr  (if NZ, CALL adr)
            if (state->cc.z == 0)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
        case 0xc5:  // PUSH B    (sp-2)<-C; (sp-1)<-B; sp <- sp - 2
        {
            state->memory[state->sp-1] = state->b;
            state->memory[state->sp-2] = state->c;
            state->sp = state->sp - 2;
        }
            break;
        case 0xc6:  // ADI D8 (A <- A + byte)
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1];
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
            state->pc++;
        }
            break;
        case 0xc7:  // RST 0    (CALL $0)
            CallConstantAdr(state, 0);
            break;
        case 0xc8:  // RZ adr  (if Z, RET)
            if (state->cc.z)
                Return(state);
            break;
        case 0xc9:  // RET      (PC.lo <- (sp); PC.hi<-(sp+1); SP <- SP+2)
            Return(state);
            break;
        case 0xca:  // JZ adr   (if Z, PC <- adr)
            if (state->cc.z)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xcb:  // NOP
            break;
        case 0xcc:  // CZ adr  (if Z, CALL adr)
            if (state->cc.z)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
//        case 0xcd:  // CALL adr ((SP-1)<-PC.hi;(SP-2)<-PC.lo;SP<-SP-2;PC=adr)
//            CallAdr(state, opcode);
//            break;
        case 0xcd:                      //CALL address

            if (5 ==  ((opcode[2] << 8) | opcode[1]))
            {
                if (state->c == 9)
                {
                    uint16_t offset = (state->d<<8) | (state->e);
                    char *str = &state->memory[offset+3];  //skip the prefix bytes
                    while (*str != '$')
                        printf("%c", *str++);
                    printf("\n");
                }
                else if (state->c == 2)
                {
                    //saw this in the inspected code, never saw it called
                    printf ("print char routine called\n");
                }
            }
            else if (0 ==  ((opcode[2] << 8) | opcode[1]))
            {
                exit(0);
            }
            else

        {
            uint16_t    ret = state->pc+2;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret & 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
        }
            break;
        case 0xce:  // ACI D8 (A <- A + data + CY)
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1] + state->cc.cy;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
            state->pc++;
        }
            break;
        case 0xcf:  // RST 1    (CALL $8)
            CallConstantAdr(state, 8);
            break;
        case 0xd0:  // RNC adr  (if NCY, RET)
            if (state->cc.cy == 0)
                Return(state);
            break;
        case 0xd1:  // POP D    E <- (sp); D <- (sp+1); sp <- sp+2
        {
            state->e = state->memory[state->sp];
            state->d = state->memory[state->sp+1];
            state->sp += 2;
        }
            break;
        case 0xd2:  // JNC adr  (if NCY, PC <- adr)
            if (state->cc.cy == 0)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xd3:  // OUT D8    special
            // TODO: implement this later
            state->pc++;
            break;
        case 0xd4:  // CNC adr  (if NCY, CALL adr)
            if (state->cc.cy == 0)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
        case 0xd5:  // PUSH D    (sp-2)<-E; (sp-1)<-D; sp <- sp - 2
        {
            state->memory[state->sp-1] = state->d;
            state->memory[state->sp-2] = state->e;
            state->sp = state->sp - 2;
        }
            break;
        case 0xd6:  // SUI D8 (A <- A - data)
        {
            state->cc.cy = (state->a < opcode[1]);
            state->a -= opcode[1];
            SetFlagsNoCarry(&state->cc, state->a);

            state->pc++;
        }
            break;
        case 0xd7:  // RST 2    (CALL $10)
            CallConstantAdr(state, 10);
            break;
        case 0xd8:  // RC adr  (if CY, RET)
            if (state->cc.cy)
                Return(state);
            break;
        case 0xd9:  // NOP
            break;
        case 0xda:  // JC adr  (if CY, PC <- adr)
            if (state->cc.cy)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xdb:  // IN D8    special
            // TODO: implement this later
            state->pc++;
            break;
        case 0xdc:  // CC adr  (if CY, CALL adr)
            if (state->cc.cy)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
        case 0xdd:  // NOP
            break;
        case 0xde:  // SBI D8   A <- A - data - CY
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) opcode[1] - state->cc.cy;
            SetFlagsNoCarry(&state->cc, answer);
            state->cc.cy = (state->a < (opcode[1] + state->cc.cy));
            state->a = answer & 0xff;
            state->pc++;
        }
            break;
        case 0xdf:  // RST 3    (CALL $18)
            CallConstantAdr(state, 18);
            break;
        case 0xe0:  // RPO  (if PO, RET)
            if (state->cc.p == 0)
                Return(state);
            break;
        case 0xe1:  // POP B    L <- (sp); H <- (sp+1); sp <- sp+2
        {
            state->l = state->memory[state->sp];
            state->h = state->memory[state->sp+1];
            state->sp += 2;
        }
            break;
        case 0xe2:  // JPO  (if PO, PC <- adr)
        // TODO: check if this and JPE aligns with parity correctly
            if (state->cc.p == 0)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xe3:  // XTHL 	L <-> (SP); H <-> (SP+1)
        {
            uint8_t temp = state->l;
            state->l = state->memory[state->sp];
            state->memory[state->sp] = temp;

            temp = state->h;
            state->h = state->memory[state->sp + 1];
            state->memory[state->sp + 1] = temp;
        }
            break;
        case 0xe4:  // CPO adr  (if PO, CALL adr)
            if (state->cc.p == 0)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
        case 0xe5:  // PUSH H    (sp-2)<-L; (sp-1)<-H; sp <- sp - 2
        {
            state->memory[state->sp-1] = state->h;
            state->memory[state->sp-2] = state->l;
            state->sp = state->sp - 2;
        }
            break;
        case 0xe6:  // ANI D8   (A <-A & data)
            state->a &= opcode[1];
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;   // ANI always clears CY
            state->pc++;
            break;
        case 0xe7:  // RST 4    (CALL $20)
            CallConstantAdr(state, 20);
            break;
        case 0xe8:  // RPE  (if PE, RET)
            if (state->cc.p)
                Return(state);
            break;
        case 0xe9:  // PCHL (PC.hi <- H; PC.lo <- L)
            state->pc = (state->h << 8) | state->l;
            break;
        case 0xea:  // JPE  (if PE, PC <- adr)
            // TODO: check if this and JPO aligns with parity correctly
            if (state->cc.p)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xeb:  // XCHG 	H <-> D; L <-> E
        {
            uint8_t temp = state->h;
            state->h = state->d;
            state->d = temp;

            temp = state->l;
            state->l = state->e;
            state->e = temp;
        }
            break;
        case 0xec:  // CPE adr  (if PE, CALL adr)
            if (state->cc.p)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
        case 0xed:  // NOP
            break;
        case 0xee:  // XRI D8   A <- A ^ data
            state->a ^= opcode[1];
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            state->pc++;
            break;
        case 0xef:  // RST 5    (CALL $28)
            CallConstantAdr(state, 28);
            break;
        case 0xf0:  // RP plus sign  (if P, RET)
            if (state->cc.s == 0)
                Return(state);
            break;
        case 0xf1:  // POP PSW   flags <- (sp); A <- (sp+1); sp <- sp+2
        {
            state->a = state->memory[state->sp+1];
            // Low 5 bits store each flag ac-cy-p-s-z
            uint8_t psw = state->memory[state->sp];
            state->cc.z  = (0x01 == (psw & 0x01));
            state->cc.s  = (0x02 == (psw & 0x02));
            state->cc.p  = (0x04 == (psw & 0x04));
            state->cc.cy = (0x08 == (psw & 0x08));
            state->cc.ac = (0x10 == (psw & 0x10));
            state->sp += 2;
        }
            break;
        case 0xf2:  // JP plus for sign (if P, PC <- adr)
            if (state->cc.s == 0)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xf3:  // DI   special disable interrupt
            state->int_enable = 0;
            break;
        case 0xf4:  // CP adr plus sign  (if PO, CALL adr)
            if (state->cc.s == 0)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
        case 0xf5:  // PUSH PSW  (sp-2)<-flags; (sp-1)<-A; sp <- sp - 2
        {
            state->memory[state->sp-1] = state->a;
            // Low 5 bits store each flag ac-cy-p-s-z
            uint8_t psw = (state->cc.z |
                           state->cc.s << 1 |
                           state->cc.p << 2 |
                           state->cc.cy << 3 |
                           state->cc.ac << 4 );
            state->sp -= 2;
            state->memory[state->sp] = psw;
        }
            break;
        case 0xf6:  // ORI D8   A <- A | data
            state->a |= opcode[1];
            SetFlagsNoCarry(&state->cc, state->a);
            state->cc.cy = 0;
            state->pc++;
            break;
        case 0xf7:  // RST 6    (CALL $30)
            CallConstantAdr(state, 30);
            break;
        case 0xf8:  // RM minus sign  (if M, RET)
            if (state->cc.s)
                Return(state);
            break;
        case 0xf9:  // SPHL SP=HL
            state->sp = (state->h << 8) | state->l;
            break;
        case 0xfa:  // JM minus for sign (if M, PC <- adr)
            if (state->cc.s)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xfb:  // EI   special enable interrupt
            state->int_enable = 1;
            break;
        case 0xfc:  // CM adr minus sign  (if M, CALL adr)
            if (state->cc.s)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
        case 0xfd:  // NOP
            break;
        case 0xfe:  // CPI D8   (A - data)
        {
            SetFlagsNoCarry(&state->cc, state->a - opcode[1]);
            state->cc.cy = (state->a < opcode[1]);
            state->pc++;
        }
            break;
        case 0xff:  // RST 7    (CALL $38)
            CallConstantAdr(state, 38);
            break;
    }
    return 0;
}

uint8_t Parity(uint8_t answer)
{
    int i;
    int count = 0;
    for (i = 0; i < 8; i++)
    {
        if (answer & 0x1) count++;
        answer >>= 1;
    }
    return (count & 0x1) == 0;
}

void SetFlags(ConditionCodes* cc, uint16_t answer)
{
    // update flags Z, S, P, CY, AC (skip AC because Space Invaders doesn't use it)
    cc->z = ((answer & 0xff) == 0); // zero: check lowest 8 bits equals 0
    cc->s = ((answer & 0x80) != 0); // sign: check bit 7 0x80 = 0b1000 0000
    cc->p = Parity(answer&0xff);
    cc->cy = (answer > 0xff);       // carry: check if it overflowed over 8 bits
}

void SetFlagsSubtraction(ConditionCodes* cc, uint16_t answer)
{
    // update flags Z, S, P, CY, AC (skip AC because Space Invaders doesn't use it)
    cc->z = ((answer & 0xff) == 0); // zero: check lowest 8 bits equals 0
    cc->s = ((answer & 0x80) != 0); // sign: check bit 7 0x80 = 0b1000 0000
    cc->p = Parity(answer&0xff);
    cc->cy = (answer <= 0xff);       // carry: check if it overflowed over 8 bits
}

void SetFlagsNoCarry(ConditionCodes* cc, uint16_t answer)
{
    // update flags Z, S, P, CY, AC (skip AC because Space Invaders doesn't use it)
    cc->z = ((answer & 0xff) == 0); // zero: check lowest 8 bits equals 0
    cc->s = ((answer & 0x80) != 0); // sign: check bit 7 0x80 = 0b1000 0000
    cc->p = Parity(answer&0xff);
}

void CallAdr(State8080* state, const unsigned char *opcode)
{
    // store return address on stack, then set pc to call address
    // pc is 16bits, memory is 8 bits, so use 2 slots in stack
    uint16_t ret = state->pc + 2;
    state->memory[state->sp - 1] = (ret >> 8) & 0xff; // bits 8-15 stored in sp-1
    state->memory[state->sp - 2] = (ret & 0xff);      // bits 0-7 stored in sp-2
    state->sp -= 2;
    state->pc = (opcode[2] << 8) | opcode[1];
}

void CallConstantAdr(State8080* state, uint8_t adr)
{
    // store return address on stack, then set pc to call address
    // pc is 16bits, memory is 8 bits, so use 2 slots in stack
    uint16_t ret = state->pc + 2;
    state->memory[state->sp - 1] = (ret >> 8) & 0xff; // bits 8-15 stored in sp-1
    state->memory[state->sp - 2] = (ret & 0xff);      // bits 0-7 stored in sp-2
    state->sp -= 2;
    state->pc = adr;
}

void Return(State8080* state)
{
    // Restore pc by popping return address off stack (16 bit adr gets stored in 2 slots)
    //              bits 8-15                           bits 0-7
    state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];
    state->sp += 2;
}

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
    state->memory = malloc(0x10000);  //16K

//    // Read files into state[memory]
//    ReadFileMem(state, "../Rom/invaders.h", 0);
//    ReadFileMem(state, "../Rom/invaders.g", 0x800);
//    ReadFileMem(state, "../Rom/invaders.f", 0x1000);
//    ReadFileMem(state, "../Rom/invaders.e", 0x1800);

    ReadFileMem(state, "../Rom/Test/cpudiag.bin", 0x100);

    //Fix the first instruction to be JMP 0x100
    state->memory[0]=0xc3;
    state->memory[1]=0;
    state->memory[2]=0x01;

    //Fix the stack pointer from 0x6ad to 0x7ad
    // this 0x06 byte 112 in the code, which is
    // byte 112 + 0x100 = 368 in memory
    state->memory[368] = 0x7;

    //Skip DAA test
    state->memory[0x59c] = 0xc3; //JMP
    state->memory[0x59d] = 0xc2;
    state->memory[0x59e] = 0x05;

    int line = 0;
//    while (state->pc < 0x2000)
//    while (line < 50000)
    int done = 0;
    while (done == 0)
    {
        done = Emulate8080Op(state);


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
//        line++;
    }
    return 0;
}