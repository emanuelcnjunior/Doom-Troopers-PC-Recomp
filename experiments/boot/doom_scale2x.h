#pragma once

/* Scale2x uses matching neighbours to round corners without inventing colours. */
static SDL_Texture *doom_scale2x(SDL_Renderer *renderer, SDL_Texture **cache,
                                  const uint32_t *pixels, int stride, int w, int h)
{
    int tw, th;
    if (*cache && (SDL_QueryTexture(*cache, NULL, NULL, &tw, &th) < 0 ||
                   tw != 2 * w || th != 2 * h)) {
        SDL_DestroyTexture(*cache);
        *cache = NULL;
    }
    if (!*cache) {
        *cache = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING, 2 * w, 2 * h);
        if (!*cache) return NULL;
        /* The new texture inherits the selected SDL sampling filter. */
    }
    void *buffer;
    int pitch;
    if (SDL_LockTexture(*cache, NULL, &buffer, &pitch) < 0) return NULL;
    for (int y = 0; y < h; ++y) {
        const uint32_t *row = pixels + y * stride;
        const uint32_t *above = y ? row - stride : row;
        const uint32_t *below = y + 1 < h ? row + stride : row;
        uint32_t *top = (uint32_t *)((uint8_t *)buffer + 2 * y * pitch);
        uint32_t *bottom = top + pitch / sizeof(*top);
        for (int x = 0; x < w; ++x) {
            uint32_t e = row[x], b = above[x], hpx = below[x];
            uint32_t d = x ? row[x - 1] : e;
            uint32_t f = x + 1 < w ? row[x + 1] : e;
            int corner = b != hpx && d != f;
            top[2*x]        = corner && d == b ? d : e;
            top[2*x + 1]    = corner && b == f ? f : e;
            bottom[2*x]     = corner && d == hpx ? d : e;
            bottom[2*x + 1] = corner && hpx == f ? f : e;
        }
    }
    SDL_UnlockTexture(*cache);
    return *cache;
}
