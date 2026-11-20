#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include "doom_scale2x.h"

int main(void)
{
    const uint32_t dark = 0xff000000u, light = 0xffffffffu;
    /* Three diagonal pixels, with a sentinel in each row's stride padding. */
    const uint32_t input[] = {
        dark,  light, light, 0,
        light, dark,  light, 0,
        light, light, dark,  0
    };
    const char *expected[] = {"001111", "010111", "100011",
                              "110001", "111010", "111100"};
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 6, 6, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *renderer = surface ? SDL_CreateSoftwareRenderer(surface) : NULL;
    SDL_Texture *texture = NULL;
    uint32_t output[36];
    int result = 1;
    if (!renderer || !doom_scale2x(renderer, &texture, input, 4, 3, 3) ||
        SDL_RenderCopy(renderer, texture, NULL, NULL) < 0 ||
        SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, output, 24) < 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        goto done;
    }
    for (int i = 0; i < 36; ++i) {
        if (output[i] != (expected[i / 6][i % 6] == '1' ? light : dark)) {
            fprintf(stderr, "Unexpected pixel at %d,%d\n", i % 6, i / 6);
            goto done;
        }
    }
    int w, h;
    if (doom_scale2x(renderer, &texture, input, 4, 1, 1) &&
        SDL_QueryTexture(texture, NULL, NULL, &w, &h) == 0 && w == 2 && h == 2)
        result = 0;
done:
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    return result;
}
