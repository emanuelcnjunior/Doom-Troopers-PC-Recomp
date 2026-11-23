# Classic / Enhanced roadmap

This roadmap records implementation status without advertising controls that do
not exist. Work is intentionally gated by the level-results acceptance test.

| Area | Status | Current scope | Next acceptance gate |
|---|---|---|---|
| Level-results transition | Windows validated | `TRAP #0`, Continue and results-entry corrections; native Windows reached interactive level 2 | Repeat clean/configuration matrix and validate on Bazzite/Proton |
| Classic Mode | Planned | Existing behavior remains the only default; no gameplay-changing option was enabled | Specify and persist an explicit profile without changing current INI defaults |
| Enhanced Mode | Planned | No menu entry or inactive control is exposed | Add only implemented low-risk video choices behind an opt-in profile |
| Low-risk scaling | Partial/existing | Original 4:3 output and Scale2x remain; pixel regression test remains active | Audit nearest, integer scaling, bilinear and VSync capabilities individually |
| Real 16:9 | Research planned | Stretching 4:3 will not be presented as widescreen | Map VDP planes, camera, spawns, HUD and level edge data; prototype in isolation |
| Camera modes | Research planned | Original camera is unchanged | Identify camera RAM/code and test solo plus two-player bounds |
| Audio replacement | Planned | Original YM2612/PSG path remains the only path | Map event IDs before accepting external files; preserve synchronized fallback |
| Mod manifests | Planned | No external code, DLL or remote content loading | Define path-safe data-only manifests and validation tests |

No later phase should be described as available until its implementation and
objective test are present. See [level-results-crash-investigation.md](level-results-crash-investigation.md)
for the current release blocker.
