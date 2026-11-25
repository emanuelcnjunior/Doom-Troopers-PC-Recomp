# Enhanced graphics and graphics-mod research

## Using the local integrated build

Select Enhanced on the Game tab. Leave Bilinear filtering and Scale2x
unchecked for nearest-neighbor sampling of the original artwork. Scale2x
reconstructs edges at twice the source dimensions; bilinear controls how SDL
samples the resulting texture or the original texture. Enable them separately
or together. New installations enable neither filter.

Integer scaling fits whole multiples of the 320 x 240 presentation canvas and
centers the picture. A 1920 x 1080 output, for example, fits 1280 x 960 at 4x.
Without integer scaling, the same output fits 1440 x 1080. SDL clears unused
space around the image. A window narrower than 320 or shorter than 240 uses
fractional fit so the full image remains visible. Windowed, fullscreen and
the F11 shortcut continue to use the same 4:3 canvas.

The stage framebuffer observed in the USA build is 320 x 224. Displaying it
in 4:3 corrects its pixel aspect. Integer scaling applies to the presentation
canvas; at many window sizes the source rows consequently have unequal
physical heights even with nearest sampling. The menu uses the term
"Integer scaling (4:3)" for this reason.

Request VSync lets SDL request a synchronized renderer. If creation fails,
the runner retries without the flag. The renderer log records the driver's
reported VSync and the requested setting. Benchmark and uncapped runs retain
their existing overrides. Hardware pacing requires measurement on a display.

Enhanced shares video/linear_filter in settings.ini with RecompLauncher.
integer_scale and vsync live in the doom_launcher section of launcher.ini.
Defaults are nearest, fractional fit and the runner's existing VSync request.
Classic ignores the Enhanced graphics preferences while retaining them for
the next switch. Existing explicit Scale2x selections are migrated.

## Mapping external art to the Genesis renderer (proposed)

The current build loads external audio. Sprite and background replacement
remain research. The following mapping describes how to identify assets
before implementing a file loader.

| Original resource | Proposed external representation and lookup |
|---|---|
| Sprite piece | In the two stage-1 snapshots the SAT at VRAM 0x0400 matched WRAM 0xFF0320. Each 8-byte entry gives Y, size/link, pattern attributes and X. Decode width and height as 1?4 cells each. At normal resolution each cell is 8 x 8, 4 bpp, so one piece is 8?32 pixels on either axis. Assemble cells in column-major order (base + column * height_in_cells + row). |
| Identity | For the USA ROM, record dimensions and a digest of the ordered original 4-bpp pattern bytes per piece before flipping. A future manifest could map this digest to a local indexed PNG. A transient tile number alone aliases other art as the game uploads patterns. Split visually identical pieces by actor/frame only after tracing their owners. |
| Color and draw order | Use indices 0?15 per pixel; index zero is transparent. Apply the SAT palette selector (four CRAM lines), X/Y flip and priority from the original attributes. Original palette changes then also affect the replacement. A first loader should validate exact piece dimensions and keep drawing coordinates, ordering and collision logic native. |
| Animation | Map each distinct original frame and its constituent pieces, including shared tiles and mirrored poses. Select the file when the corresponding pattern is displayed. The game owns frame durations and pose changes. Verify against idle, turn, jump, fire and co-op palette variants. |
| Planes and tiles | Stage-1 plane A/B name tables were at VRAM 0xE000/0xC000. Each cell references a pattern, palette, flip and priority. Resolve a plane-specific pattern digest at lookup time, leaving scrolling, window/HUD and animated table updates in game coordinates. Replacing the final framebuffer would lose these distinctions. |

The pinned VDP exposes gvdp_set_sprite_patterns_bank for marked SAT entries,
and gvdp_set_host_sprites for a host sprite layer. The bank hook is specific
to sprites; planes need their own presentation lookup. First trace an entire
pose into its SAT pieces and validate against the original composited frame.
A later data-only loader can preload validated local assets and fall back
per piece when identification or decoding fails. No stable character-frame
catalogue, external PNG support or graphics manifest is available yet.

Source references within the pinned framework: runner/video/genesis_vdp.c
(sprite_render_line and gvdp_render_scanline), runner/video/genesis_vdp.h and
runner/main.c. Stage sample evidence is in [the viewport investigation](widescreen-investigation.md).
