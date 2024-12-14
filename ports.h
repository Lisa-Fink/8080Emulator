#ifndef INC_8080EMULATOR_PORTS_H
#define INC_8080EMULATOR_PORTS_H

#include "ports.h"


typedef struct Ports {
    // input-read, output-write
    uint8_t input0, input1, input2, output2, output3, output5, output6;
    uint16_t shift_register;
    uint8_t shift_amount;
} Ports;

#endif //INC_8080EMULATOR_PORTS_H
