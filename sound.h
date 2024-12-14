#ifndef INC_8080EMULATOR_SOUND_H
#define INC_8080EMULATOR_SOUND_H

#include "ports.h"

void PlaySounds(Ports* ports, int port, uint8_t old_bits);

#endif //INC_8080EMULATOR_SOUND_H
