# Classic / Enhanced roadmap

This roadmap records implementation status without advertising controls that do
not exist. Work is intentionally gated by the level-results acceptance test.

| Area | Status | Current scope | Next acceptance gate |
|---|---|---|---|
| Level-results transition | Windows validated | `TRAP #0`, Continue and results-entry corrections; native Windows reached interactive level 2 | Repeat clean/configuration matrix and validate on Bazzite/Proton |
| Classic Mode | Implemented locally | Fresh installs use the original VDP frame, 4:3 presentation, unfiltered pixels and NTSC pacing | Validate on physical displays and Proton |
| Enhanced Mode | Implemented locally | 4:3 presentation, optional Scale2x and audio replacement, 60 Hz presentation target | Validate graphics on physical renderers |
| Low-risk scaling | Implemented locally | Nearest, optional bilinear, Scale2x, integer 4:3 canvas scaling and VSync request. See [graphics and asset mapping](enhanced-graphics.md) | Measure physical presentation and Proton compatibility |
| Real 16:9 | Offline VDP prototype tested | Four snapshots preserved the center while sampling extra columns on both sides. See [Phase 3 investigation](widescreen-investigation.md) | Verify world-column streaming, actor culling, HUD, level bounds and co-op before exposing a menu option |
| Camera modes | Research planned | Original camera is unchanged | Identify camera RAM/code and test solo plus two-player bounds |
| Audio replacement | Implemented locally | Enhanced can replace USA sequence events with WAV, OGG Vorbis or MP3. See [audio mods](modding.md) | Identify more sequence names and test physical output and Proton |
| Mod manifests | Implemented locally | Explicit package allowlist, data-only audio manifest, conflict fallback and global disable. See [modding](modding.md) | Validate on Proton before release; extend resource types when their renderer is ready |

No later phase should be described as available until its implementation and
objective test are present. See [level-results-crash-investigation.md](level-results-crash-investigation.md)
for the current release blocker.

## Player feedback and follow-up

The player reported a crash at the first results screen on Bazzite. Their
other requests describe optional enhancements. The report did not identify
the tested build or Proton version. Implementation status above refers to
the local development build.

| Player observation or request | Follow-up and evidence needed |
|---|---|
| Crash after finishing stage 1 | Highest priority. Record build, Proton version and settings, then replay through results into controllable stage 2. If it fails, collect the transition log. The Windows manual pass covers one configuration. The frame-4,402 smoke test ends before results. |
| Preserve the original and offer enhancements separately | Keep Classic as the fresh-install default. Test every new Enhanced option with Classic selected and verify that switching back restores the original presentation. |
| See approaching enemies earlier | Compare each experimental side column with the same world position later seen in the native viewport. Check when enemies become visible and when they start acting, including at stage edges. |
| Reduce the camera swing on direction reversal | Trace foreground scroll and both players' positions on a repeatable turn. Prototype adjustable look-ahead in Enhanced. Check jitter, map limits and framing when co-op players separate or reverse independently. |
| Smoother frame pacing | The profiles select different presentation targets. Measure actual frame intervals and missed refreshes using the same scrolling replay on Windows and Proton before claiming smoother delivery. |
| Fan soundtrack and effects | The local build supports event-driven replacements. Expand the verified sequence-ID catalogue and audition transitions, overlapping effects and loops on physical output. |
| More visual choices, including play without filters | Keep Scale2x optional. Verify display behavior on physical hardware for the implemented scaling modes and VSync request. For sprite replacements, map tile identity across animation frames, palettes, transparency and priority before exposing a control. |
| Optional gameplay fixes | No specific gameplay change was requested. Record a reproducible example and its effect on solo and co-op before proposing a separately selectable fix. |

## Integrated profiles (Windows x64)

Select Classic or Enhanced on the Game tab. Play validates the USA ROM and
saves the selection. Cancel discards edits. Enhanced remembers its Scale2x
setting while Classic is active. Both profiles use the same game code, camera,
input mapping, fullscreen setting and explicitly
selected cheats. The 320 × 240 SDL presentation canvas fits the full VDP image
into a 4:3 rectangle. It does not change the game's framebuffer or viewport.

In launcher.ini, section `[doom_launcher]`, `profile` accepts `classic` or
`enhanced`. With no profile, an explicit legacy `scale2x=1` selects Enhanced
with filtering. A missing or disabled filter selects Classic. An unknown mode
also selects Classic. Play stores the Enhanced preference in `enhanced_scale2x`
and mirrors the active filter in the legacy `scale2x` key. Shared settings stay
in their existing sections.

ROM-argument launches read the profile next to the executable. An explicit
`DOOM_SCALE2X` override remains available for direct launches. Classic uses
the runner's NTSC presentation default (60000/1001 Hz), Enhanced uses 60 Hz.
Explicit runner pacing overrides retain precedence.

The integrated Release build registers `integrated_profiles` with CTest when
configured with a validated local ROM. It uses temporary INIs to exercise the
Win32 controls, migrate both legacy filter values, retain the inactive filter
preference, reopen the dialog and launch both profiles directly. The existing
audio and CPU tests remain separate. Physical PS4, display and Bazzite/Proton
validation are pending.

## Fase 8 — checklist do build de 25/11/2026

Windows 11 x64, build 26300. Executável de desenvolvimento:
`28e09b4bdf98708e0b74cddb152f229ce8ff84fec879a94912c7d55d033f47f8`.
As marcações referem-se a este binário.

- [x] Build Release x64 integrado com a ROM USA validada.
- [x] CTest do jogo (8/8), da raiz (1/1, abrangendo sete testes Python) e
  do protótipo isolado de viewport (1/1).
- [x] Menu Win32: selecionar a ROM e acionar Jogar chega ao loop de quadros
  do jogo. A seleção de perfil e preferências é relida ao reabrir o menu.
- [x] Execução por argumento: Classic e Enhanced percorrem o mesmo replay
  até o quadro 4.402, com quatro capturas do framebuffer do jogo idênticas
  entre os perfis. O teste exige quatro imagens distintas dentro de cada run.
- [x] Saves: gravação e restauração com áudio personalizado, carga sem o
  arquivo de mod e carga de estado da primeira fase durante a medição.
- [x] Vídeo e áudio: pixels e barras em SDL por software, Direct3D 11 em
  janela com VSync reportado, captura PCM estéreo, codecs e fallback.
- [x] Pacotes de mods: ativação explícita, conflitos, caminhos inválidos,
  desativação global e compatibilidade com áudio avulso.
- [ ] Concluir a fase 1, passar pelos resultados e controlar a fase 2 no
  executável atual. O teste manual histórico passou em outro binário.
- [ ] Repetir a transição em Bazzite/Proton. O host dispunha apenas da
  distribuição WSL docker-desktop parada, sem sessão de jogo Linux disponível.
- [ ] Conferir monitor, som audível, controle PS4 físico e fluidez prolongada.
- [ ] Câmera com antecipação ajustável permanece em pesquisa; o protótipo
  de widescreen foi validado apenas como renderização offline.

**Amostra de desempenho.** Em execuções sequenciais de 360 quadros, carregamos
o mesmo estado da selva, avançamos para a direita e usamos Direct3D 11 em
janela, VSync e saída SDL de 48 kHz. Com `--measure-runtime 360`, o Classic
mediu 60,000 fps, média de 16,667 ms e maior intervalo de 42,985 ms; o
Enhanced com Scale2x mediu 60,116 fps, média de 16,635 ms e maior intervalo
de 30,348 ms. Ambos completaram 360 flushes e zero descartes por overflow
de áudio, com hashes finais de simulação e áudio iguais. Há apenas uma execução
curta por perfil; os máximos não estabelecem ganho de desempenho entre modos.

Para repetir o replay comparativo, rode
`ctest --test-dir build/integrated -C Release -R gameplay_profiles -V`.
O teste usa vídeo/áudio simulados e turbo para comparar o conteúdo do jogo.
Para repetir a medição com apresentação real, use um estado local compatível
da primeira fase e um roteiro com `WAIT 3`, `LOAD_STATE nome.state` e
`HOLD RIGHT`, uma instrução por linha. Inicie o jogo com a ROM por argumento,
`--measure-runtime 360 --input-script caminho-do-roteiro` e
`GENESIS_HOST_TIMING_DUMP=1`. O procedimento de aceitação externo está na
[investigação da transição](level-results-crash-investigation.md).
