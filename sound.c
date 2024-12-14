#include <stdio.h>
#include <SDL2/SDL.h>

#include "sound.h"

void PlaySounds(Ports* ports, int port, uint8_t old_bits)
{
    static SDL_AudioSpec wav_spec;
    static Uint32 wav_length;
    static Uint8 *wav_buffer;
    static Uint8 *ufo_buffer = NULL;
    static int is_playing = 0;
    static SDL_AudioDeviceID device_id = 0;

    // Init
    if (device_id == 0) {
        wav_spec.freq = 44100;
        wav_spec.format = AUDIO_S16SYS;
        wav_spec.channels = 1;           // Mono
        wav_spec.samples = 4096;         // Buffer size
        wav_spec.callback = NULL;

        device_id = SDL_OpenAudioDevice(NULL, 0, &wav_spec, NULL, 0);
        if (device_id == 0) {
            printf("SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
            SDL_Quit();
            return;
        }
        SDL_LoadWAV("../Rom/Sounds/0.wav", &wav_spec, &ufo_buffer, &wav_length);
    }

    if (port == 3)
    {
        uint8_t bits = ports->output3;

        // UFO sound - 0.wav repeatedly
        if (bits & 0x1)
        {
            SDL_QueueAudio(device_id, ufo_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0);
            is_playing = 1;
        }
            // Stop UFO sound when the bit is cleared
        else if (is_playing && ((bits & 0x1) == 0))
        {
            SDL_ClearQueuedAudio(device_id);  // Clear the queue
            is_playing = 0;
        }

        // Shot sound - 1.wav
        if ((bits & 0x02) && !(old_bits & 0x02))
        {
            if (SDL_LoadWAV("../Rom/Sounds/1.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
                printf("SDL_LoadWAV failed: %s\n", SDL_GetError());
                return;
            }
            SDL_QueueAudio(device_id, wav_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0); // Unpause to start playback
        }

        // Flash (player die) - 2.wav
        if ((bits & 0x04) && !(old_bits & 0x04)) {
            if (SDL_LoadWAV("../Rom/Sounds/2.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
                printf("SDL_LoadWAV failed: %s\n", SDL_GetError());
                return;
            }
            SDL_QueueAudio(device_id, wav_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0); // Unpause to start playback
        }

        // Invader die - 3.wav
        if ((bits & 0x08) && !(old_bits & 0x08)) {
            if (SDL_LoadWAV("../Rom/Sounds/3.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
                printf("SDL_LoadWAV failed: %s\n", SDL_GetError());
                return;
            }
            SDL_QueueAudio(device_id, wav_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0); // Unpause to start playback
        }
    }
        // Port 5
    else
    {
        uint8_t bits = ports->output5;

        // Fleet movement 1 - 4.wav
        if ((bits & 0x02) && !(old_bits & 0x02))
        {
            if (SDL_LoadWAV("../Rom/Sounds/4.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
                printf("SDL_LoadWAV failed: %s\n", SDL_GetError());
                return;
            }
            SDL_QueueAudio(device_id, wav_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0); // Unpause to start playback
        }

        // Fleet movement 2 - 5.wav
        if ((bits & 0x02) && !(old_bits & 0x02))
        {
            if (SDL_LoadWAV("../Rom/Sounds/5.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
                printf("SDL_LoadWAV failed: %s\n", SDL_GetError());
                return;
            }
            SDL_QueueAudio(device_id, wav_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0); // Unpause to start playback
        }

        // Fleet movement 3 - 6.wav
        if ((bits & 0x04) && !(old_bits & 0x04)) {
            if (SDL_LoadWAV("../Rom/Sounds/6.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
                printf("SDL_LoadWAV failed: %s\n", SDL_GetError());
                return;
            }
            SDL_QueueAudio(device_id, wav_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0); // Unpause to start playback
        }

        // Fleet movement 4 - 7.wav
        if ((bits & 0x08) && !(old_bits & 0x08)) {
            if (SDL_LoadWAV("../Rom/Sounds/7.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
                printf("SDL_LoadWAV failed: %s\n", SDL_GetError());
                return;
            }
            SDL_QueueAudio(device_id, wav_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0); // Unpause to start playback
        }

        // UFO Hit - 8.wav
        if ((bits & 0x10) && !(old_bits & 0x10)) {
            if (SDL_LoadWAV("../Rom/Sounds/8.wav", &wav_spec, &wav_buffer, &wav_length) == NULL) {
                printf("SDL_LoadWAV failed: %s\n", SDL_GetError());
                return;
            }
            SDL_QueueAudio(device_id, wav_buffer, wav_length);
            SDL_PauseAudioDevice(device_id, 0); // Unpause to start playback
        }
    }
}
