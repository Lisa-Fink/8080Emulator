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
        case 0xc0:  // RNZ adr  (if NZ, RET)
            if (state->cc.z == 0)
                Return(state);
            else
                state->pc += 1;
            break;
            /*...*/
        case 0xc2:  // JNZ adr  (if NZ, PC <- adr)
        // TODO: check how pc gets updated in disassembler for jumps/call/return (should only be handled here)
            if (state->cc.z == 0)
                    state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xc3:  // JMP adr  (PC <- adr)
            state->pc = (opcode[2] << 8) | opcode[1];
        case 0xc4:  // CNZ adr  (if NZ, CALL adr)
            if (state->cc.z == 0)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            /*...*/
        case 0xc6:  // ADI D8 (A <- A + byte)
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1];
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0xc7:  // RST 0    (CALL $0)
            CallConstantAdr(state, 0);
            break;
        case 0xc8:  // RZ adr  (if Z, RET)
            if (state->cc.z)
                Return(state);
            else
                state->pc += 1;
            break;
        case 0xc9:  // RET      (PC.lo <- (sp); PC.hi<-(sp+1); SP <- SP+2)
            Return(state);
            break;
            /*...*/
        case 0xca:  // JZ adr   (if Z, PC <- adr)
            if (state->cc.z)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xcc:  // CZ adr  (if Z, CALL adr)
            if (state->cc.z)
                CallAdr(state, opcode);
            else
                state->pc += 2;
        case 0xcd:  // CALL adr ((SP-1)<-PC.hi;(SP-2)<-PC.lo;SP<-SP-2;PC=adr)
            CallAdr(state, opcode);
            break;
        case 0xce:  // ACI D8 (A <- A + data + CY)
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1] + state->cc.cy;
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0xcf:  // RST 1    (CALL $8)
            CallConstantAdr(state, 8);
            break;
        case 0xd0:  // RNC adr  (if NCY, RET)
            if (state->cc.cy == 0)
                Return(state);
            else
                state->pc += 1;
            break;
            /*...*/
        case 0xd2:  // JNC adr  (if NCY, PC <- adr)
            if (state->cc.cy == 0)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xd4:  // CNC adr  (if NCY, CALL adr)
            if (state->cc.cy == 0)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xd6:  // SUI D8 (A <- A - data)
        {
            uint16_t answer = (uint16_t) state->a - (uint16_t) opcode[1];
            SetFlags(&state->cc, answer);
            state->a = answer & 0xff;
        }
            break;
        case 0xd7:  // RST 2    (CALL $10)
            CallConstantAdr(state, 10);
            break;
        case 0xd8:  // RC adr  (if CY, RET)
            if (state->cc.cy)
                Return(state);
            else
                state->pc += 1;
            break;
            /*...*/
        case 0xda:  // JC adr  (if CY, PC <- adr)
            if (state->cc.cy)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xdc:  // CC adr  (if CY, CALL adr)
            if (state->cc.cy)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xdf:  // RST 3    (CALL $18)
            CallConstantAdr(state, 18);
            break;
        case 0xe0:  // RPO  (if PO, RET)
            if (state->cc.p == 0)
                Return(state);
            else
                state->pc += 1;
            break;
            /*...*/
        case 0xe2:  // JPO  (if PO, PC <- adr)
        // TODO: check if this and JPE aligns with parity correctly
            if (state->cc.p == 0)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xe4:  // CPO adr  (if PO, CALL adr)
            if (state->cc.p == 0)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xe7:  // RST 4    (CALL $20)
            CallConstantAdr(state, 20);
            break;
        case 0xe8:  // RPE  (if PE, RET)
            if (state->cc.p)
                Return(state);
            else
                state->pc += 1;
            break;
        case 0xe9:  // PCHL (PC.hi <- H; PC.lo <- L)
            state->pc = (state->h << 8) | state->l;
            /*...*/
        case 0xea:  // JPE  (if PE, PC <- adr)
            // TODO: check if this and JPO aligns with parity correctly
            if (state->cc.p)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xec:  // CPE adr  (if PE, CALL adr)
            if (state->cc.p)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xef:  // RST 5    (CALL $28)
            CallConstantAdr(state, 28);
            break;
        case 0xf0:  // RP plus sign  (if P, RET)
            if (state->cc.s == 0)
                Return(state);
            else
                state->pc += 1;
            break;
            /*...*/
        case 0xf2:  // JP plus for sign (if P, PC <- adr)
            if (state->cc.s == 0)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xf4:  // CP adr plus sign  (if PO, CALL adr)
            if (state->cc.s == 0)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xf7:  // RST 6    (CALL $30)
            CallConstantAdr(state, 30);
            break;
        case 0xf8:  // RM minus sign  (if M, RET)
            if (state->cc.s)
                Return(state);
            else
                state->pc += 1;
            break;
            /*...*/
        case 0xfa:  // JM minus for sign (if M, PC <- adr)
            if (state->cc.s)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xfc:  // CM adr minus sign  (if M, CALL adr)
            if (state->cc.s)
                CallAdr(state, opcode);
            else
                state->pc += 2;
            break;
            /*...*/
        case 0xfe: UnimplementedInstruction(state); break;
        case 0xff:  // RST 7    (CALL $38)
            CallConstantAdr(state, 38);
            break;
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
