/* Offline scanline experiment for the pinned Windows x64 GROWNS3 layout. */
#include "genesis_vdp.h"
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static GVDP vdp;

static int load_snapshot(const char *path) {
    /* main.c: header, glue_save_state (CPU, frame, cycles, threshold, IRQ),
       64 KiB WRAM, then GenesisMachine, whose first member is GVDP. */
    const long vdp_offset = 8 + 76 + 8 + 4 + 4 + 1 + 65536;
    FILE *f = fopen(path, "rb");
    char magic[8];
    /* Read only video memories and registers. Never import host pointers. */
    const size_t bytes = offsetof(GVDP, reg) + sizeof vdp.reg;
    int ok = f && fread(magic, 1, 8, f) == 8 && !memcmp(magic, "GROWNS3", 8)
               && fseek(f, vdp_offset, SEEK_SET) == 0 && fread(&vdp, 1, bytes, f) == bytes;
    if (f) fclose(f);
    if (!ok) fprintf(stderr, "Expected a local Windows x64 GROWNS3 snapshot.\n");
    return ok;
}

static int probe(int fixture) {
    int width = gvdp_screen_width(&vdp), height = gvdp_output_height(&vdp);
    /* At unchanged pixel aspect, 16:9 needs 4/3 of the 4:3 source width.
       Round each margin to the nearest whole source sample. */
    int margin = (width + 3) / 6;
    unsigned different_left = 0, different_right = 0;
    uint8_t native[GVDP_MAX_WIDTH], wide[GVDP_MAX_WIDTH];
    for (int y = 0; y < height; ++y) {
        gvdp_set_ws_extra(0);
        if (gvdp_render_scanline(&vdp, y, native) != width) return 1;
        gvdp_set_ws_extra(margin);
        if (gvdp_render_scanline(&vdp, y, wide) != width + 2 * margin ||
            memcmp(native, wide + margin, width)) {
            fprintf(stderr, "Central image changed on row %d\n", y);
            return 1;
        }
        for (int x = 0; x < margin; ++x) {
            different_left += wide[x] != native[0];
            different_right += wide[margin + width + x] != native[width - 1];
        }
        if (fixture && (native[0] != 1 || wide[margin - 1] != 3 || wide[margin + width] != 2))
            return 1;
    }
    printf("VDP=%dx%d wide=%d margin=%d center=identical different_from_edge=%u,%u\n",
           width, height, width + 2 * margin, margin, different_left, different_right);
    printf("VRAM A=%04X B=%04X window=%04X SATreg=%02X hscroll=%04X R10hex=%02X\n",
           (vdp.reg[2] & 0x38) << 10, (vdp.reg[4] & 7) << 13,
           (vdp.reg[3] & 0x3e) << 10, vdp.reg[5], (vdp.reg[13] & 63) << 10, vdp.reg[16]);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: DoomViewportProbe STATE | --self-test\n"); return 2; }
    int fixture = !strcmp(argv[1], "--self-test");
    if (fixture) {
        vdp.reg[1] = 0x40;
        vdp.reg[2] = 0x30;
        vdp.reg[4] = 5;
        vdp.reg[12] = 1;
        vdp.reg[13] = 0x3c;
        vdp.reg[16] = 1; /* 64 columns. Columns 40 and 63 are outside H40. */
        for (int tile = 1; tile <= 3; ++tile)
            memset(vdp.vram + tile * 32, tile * 0x11, 32);
        for (int row = 0; row < 32; ++row)
            for (int col = 0; col < 64; ++col)
                vdp.vram[0xc001 + 2 * (row * 64 + col)] = col == 63 ? 3 : col >= 40 ? 2 : 1;
    } else if (!load_snapshot(argv[1])) return 2;
    return probe(fixture);
}
