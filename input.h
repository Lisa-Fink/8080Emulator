#ifndef INC_8080EMULATOR_INPUT_H
#define INC_8080EMULATOR_INPUT_H

#include <SDL2/SDL.h>
#include "ports.h"

void KeyDown(SDL_KeyCode key, Ports* ports);
void KeyUp(SDL_KeyCode key, Ports* ports);

#endif //INC_8080EMULATOR_INPUT_H
