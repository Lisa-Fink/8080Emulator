#ifndef INC_8080EMULATOR_GRAPHICS_H
#define INC_8080EMULATOR_GRAPHICS_H

#include "8080emulator.h"
#include <SDL2/SDL.h>

void draw_screen(State8080* state, SDL_Renderer* renderer, int interrupt_num, SDL_Surface* surface);

#endif //INC_8080EMULATOR_GRAPHICS_H
