#include "input.h"

void KeyDown(SDL_KeyCode key, Ports* ports)
{
    switch (key) {
        case SDLK_c:        // COIN
            ports->input1 |= 0x1;
            break;
        case SDLK_RETURN:   // 2P START
            ports->input1 |= 0x02;
            break;
        case SDLK_1:        // 1P START
            ports->input1 |= 0x04;
            break;
        case SDLK_SPACE:    // 1P SHOOT
            ports->input1 |= 0x10;
            break;
        case SDLK_a:        // 1P LEFT
            ports->input1 |= 0x20;
            break;
        case SDLK_d:        // 1P RIGHT
            ports->input1 |= 0x40;
            break;

        case SDLK_UP:       // 2P SHOOT
            ports->input2 |= 0x10;
            break;
        case SDLK_LEFT:     // 2P LEFT
            ports->input2 |= 0x20;
            break;
        case SDLK_RIGHT:     // 2P RIGHT
            ports->input2 |= 0x40;
            break;
        default:
            break;
    }
}

void KeyUp(SDL_KeyCode key, Ports* ports)
{
    switch (key) {
        case SDLK_c:
            ports->input1 &= ~0x1;
            break;
        case SDLK_RETURN:
            ports->input1 &= ~0x2;
            break;
        case SDLK_1:
            ports->input1 &= ~0x4;
            break;
        case SDLK_SPACE:
            ports->input1 &= ~0x10;
            break;
        case SDLK_a:
            ports->input1 &= ~0x20;
            break;
        case SDLK_d:
            ports->input1 &= ~0x40;
            break;

        case SDLK_UP:       // 2P SHOOT
            ports->input2 &= ~0x10;
            break;
        case SDLK_LEFT:     // 2P LEFT
            ports->input2 &= ~0x20;
            break;
        case SDLK_RIGHT:     // 2P RIGHT
            ports->input2 &= ~0x40;
            break;
        default:
            break;
    }
}
