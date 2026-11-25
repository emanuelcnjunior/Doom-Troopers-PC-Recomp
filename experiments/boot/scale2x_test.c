#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include "doom_scale2x.h"

/* Read the final surface, including the borders outside SDL's logical viewport. */
static int check_canvas(int width, int height, SDL_bool integer, SDL_Rect expected)
{
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *renderer = surface ? SDL_CreateSoftwareRenderer(surface) : NULL;
    int ok = renderer && SDL_RenderSetLogicalSize(renderer, 320, 240) == 0 &&
             SDL_RenderSetIntegerScale(renderer, integer) == 0;
    if (ok) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect canvas = { 0, 0, 320, 240 };
        ok = SDL_RenderFillRect(renderer, &canvas) == 0;
        SDL_RenderPresent(renderer);
        for (int y = 0; ok && y < height; ++y) {
            const uint32_t *row = (const uint32_t *)((const uint8_t *)surface->pixels + y * surface->pitch);
            for (int x = 0; x < width; ++x) {
                int inside = x >= expected.x && x < expected.x + expected.w &&
                             y >= expected.y && y < expected.y + expected.h;
                if ((row[x] & 0xffffff) != (inside ? 0xffffffu : 0u)) {
                    fprintf(stderr, "Unexpected viewport pixel at %d,%d in %dx%d\n", x,y,width,height);
                    ok = 0;
                    break;
                }
            }
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    return ok;
}

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
    if (!check_canvas(800, 480, SDL_FALSE, (SDL_Rect){80, 0, 640, 480}) ||
        !check_canvas(640, 600, SDL_FALSE, (SDL_Rect){0, 60, 640, 480}) ||
        !check_canvas(800, 600, SDL_TRUE, (SDL_Rect){80, 60, 640, 480}))
        return 1;
    return result;
}
