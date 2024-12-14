# include <stdio.h>
#include "graphics.h"

void draw_screen(State8080* state, SDL_Renderer* renderer, int interrupt_num, SDL_Surface* surface) {
    uint8_t *video_memory = &state->memory[0x2400];  // Starting address for video memory
    int start_y = (interrupt_num == 0) ? 0 : 112;   // Top half or bottom half
    int end_y = (interrupt_num == 0) ? 112 : 224;




    uint32_t color;
    for (int y = start_y; y < end_y; y++) {
        for (int x = 0; x < 256; x++) {
            int byte_offset = y * 32 + x / 8;
            int bit_offset = x % 8;

            color = (video_memory[byte_offset] & (1 << bit_offset)) ? 0xFFFFFFFF : 0x00000000;
            ((uint32_t*)surface->pixels)[(255 - x) * 224 + y] = color;
        }
    }


    if (interrupt_num == 1) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        if (!texture) {
            printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
            return;
        }

        SDL_Rect destRect = { 0, 0, 224 * 3, 256 * 3};
        SDL_RenderCopy(renderer, texture, NULL, &destRect);

        // Clean up the texture
        SDL_DestroyTexture(texture);


        SDL_RenderPresent(renderer);
    }
}