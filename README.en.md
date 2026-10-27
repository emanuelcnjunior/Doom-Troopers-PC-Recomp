# Doom Troopers (USA) — PC Recomp

Windows x64 / Sega Genesis 68000 recompilation research.
**Stage 0 only: identity validation tools. No working game, validated boot or installer.**

Supply your own local ROM. Never upload ROMs, generated game code, extracted
assets, captures or save states to this repository. See [Portuguese README](README.md)
for commands and [architecture](docs/architecture.md) for the incremental plan.
Python 3.10+ and CMake 3.24+ are sufficient for the current tooling.
No Genesis runtime or SNES dependency is included.

An opt-in [boot experiment](docs/boot-experiment.md) now builds against a pinned
local Genesis framework. Game boot fails at TRAP #1; no playable port or
independent framebuffer/audio comparison has been validated.
