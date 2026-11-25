# Phase 3 — Enlarging the Enhanced viewport

## Result

The pinned VDP can render previously clipped columns from its plane maps and
sprite table. The isolated experiments/widescreen target builds its own copy of
the scalar VDP renderer. It never links into the game. Each row is rendered at
native width and again with wider clipping, then the center is compared by
palette index. A synthetic fixture places three known tile patterns across the
old left boundary, center and old right boundary. It checks the expected sample
at each boundary on every row. This exercises actual tile fetches.

The local USA replay used H40, 320 × 224. The 4:3 display gives these samples
a pixel aspect ratio of 14:15. At the same height and pixel aspect, a 16:9
display covers 426 2/3 horizontal source samples. The experiment takes the
nearest symmetric integer width, 426 (53 extra samples per side). A final
presenter can fetch the enclosing tile columns and crop at fractional source
coordinates. No horizontal interpolation of the old frame is involved.

| Replay frame | Center comparison | Left / right samples different from nearest native edge |
|---:|---|---:|
| 2,001 | identical | 11,872 / 11,872 |
| 2,601 | identical | 11,872 / 11,872 |
| 3,501 | identical | 5,239 / 5,678 |
| 4,401 | identical | 5,303 / 4,387 |

Each margin contains 11,872 samples. The third and fourth captures cover stage
1 before and after the movement segment. Counts measure palette indices and
can include stale tiles, the window layer or background. Freeze-frame rendering
uses one register snapshot for all scanlines and cannot reconstruct writes
made during the original raster. The center comparison is against the same
snapshot at native width. It does not certify a complete live frame.

## Cartridge and renderer evidence

- In the pinned framework, runner/video/genesis_vdp.c functions
  gvdp_render_scanline and sprite_render_line implement scrolling planes A/B,
  the window plane, sprite links, priority and clipping. The sampled register
  0x10 is 0x11, giving 64 × 64 tiles per plane. VRAM bases were 0xE000 (A),
  0xC000 (B), 0x0400 (sprite attribute table) and 0x0000 (horizontal scroll).
  The window register also points at zero; that alone does not identify the HUD.
  Scroll mode register 0x0B was 0x0A in the two stage captures, so A and B
  use scroll entries per eight lines. Reading one scroll word as a camera for
  every layer would lose parallax.
- The two stage snapshots matched all 640 SAT bytes at WRAM 0xFF0320 and
  VRAM 0x0400. ROM PC 0x057006 loads that WRAM address during DMA setup
  (generated doom_part16.c). The sprite lists contained 20 and 24 entries.
  Both included a sprite starting beyond the old right edge (x=324 and 333).
  This establishes partial offscreen submission. It does not establish that
  every enemy, projectile or effect is submitted there.
- In both stage captures WRAM words 0xFF2DEA and 0xFF2DEE matched the negative
  of plane A's first horizontal scroll word (14 and 1188). PC 0x053328 takes
  the first address in the VDP write path, PC 0x05430C loads the same base,
  and PC 0x054CD6 reads the second word. These are candidates for tracing
  foreground scroll. Their role as requested/current camera positions, bounds
  or player coordinates is unverified.
- config/doom_boot.toml site 0x000358 is the actor stack scheduler. Its
  widescreen_site declaration is a game_hook. Generated doom_layout.c has
  ws_capable=0 and zero eligibility, two-player and redraw addresses. The
  runner's generic widening path in runner/main.c propagates a margin to the
  VDP and supported recompiled sites. Merely setting the capability flag
  cannot supply missing Doom streaming, visibility or stage logic.

## Changes needed before enabling live play

| Area | Evidence, missing mapping and acceptance check |
|---|---|
| Stage bounds and streaming | Each resident plane is a wrapping 512-pixel-wide tile ring in these captures. Trace writes to both name tables while crossing a column in each direction. Match each revealed world column with a later native view at the same world position, accounting for animated tiles and parallax. Trace the initial fill, map dimensions and both stage ends. Widen prefetch by the required margin; at a boundary with insufficient data use the validated 4:3 view. |
| Actors and spawning | Separate SAT clipping, actor activation, despawn and AI triggers. Trace writes to the 0xFF0320 staging buffer and follow callers back to actor coordinates. Compare identical-input runs for enemy counts, projectiles and encounter timing. Recompiled hooks must also cover actor routines executed through the interpreter in doom_spec.c. |
| HUD and fixed elements | Trace writes with P1 health changes (known health word 0xFF06BE) and isolate window/plane or sprite owners by row and priority. Check health and player HUD for duplicated items in the revealed margins. Keep the original UI anchors until their ownership is known. |
| Physics and collisions | The renderer uses a row pixel budget based on output width and can set sprite overflow/collision flags. The offline process isolates these side effects. A live renderer needs an independent presentation pass or equivalent isolation so visual expansion cannot change guest VDP status, actor simulation, hits or collision bounds. Compare deterministic state across runs. |
| Co-op and scene eligibility | Identify the active player count, both actors and camera leash from two-player traces. Validate separation, opposite-edge motion, death and respawn. Gate title, results, transitions, unmapped stages and co-op to 4:3 until individually verified. Stage observation points in doom_spec.c include WRAM 0xFF002A–0xFF002E. No two-player or full-stage-edge trace was collected here. |

## Reproduction on Windows x64

Use the project's pinned framework and a locally built integrated game with the
validated USA ROM. From the repository root:

~~~powershell
cmake -S experiments/widescreen -B build/widescreen -G "Visual Studio 17 2022" -A x64
cmake --build build/widescreen --config Release
ctest --test-dir build/widescreen -C Release --output-on-failure
$d = (Resolve-Path build/widescreen).Path
Copy-Item build/integrated/Release/DoomBootProbe.exe,build/integrated/Release/SDL2.dll $d
(Get-Content tests/doom_gameplay_probe.input) -replace '^SCREENSHOT (.+)\.png','SAVE_STATE $1.state' | Set-Content "$d/route.input" -Encoding ascii
$env:SDL_AUDIODRIVER='dummy'
$env:SDL_VIDEODRIVER='dummy'
$env:SDL_RENDER_DRIVER='software'
# Set $rom to the absolute path of your own USA ROM.
Start-Process "$d/DoomBootProbe.exe" -WorkingDirectory $d -ArgumentList ('"'+$rom+'" --turbo --max-frames 4600 --input-script "'+$d+'\route.input"') -Wait
Get-ChildItem "$d/doom_*.state" | ForEach-Object { & "$d/Release/DoomViewportProbe.exe" $_.FullName }
~~~

The reader expects this build's x64 GROWNS3 prefix. It reads only video
memories and registers, never restores a game and never imports serialized
callback pointers. All captures and binaries stay in the ignored build folder.
