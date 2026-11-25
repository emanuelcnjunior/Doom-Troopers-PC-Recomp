# Level-results crash investigation

Status on 2026-11-23: the interpreter gap and two missing translated transition
entries were corrected. A native Windows manual run completed level 1, passed
through results and reached interactive level 2. The Bazzite report and the
remaining clean/configuration matrix stay open until their acceptance runs pass.

## Confirmed facts

- The audited source is `main` at `250be01345ecf394ed8af427e97ef83eeb736431`,
  which matched `origin/main`. Release v0.1.9 points to `5320885`.
- `run_actor_scheduler()` executes address `$000358` with the 68000 interpreter
  because actor callbacks may change the emulated stack. The generated caller
  expects to return to `$054EE0`.
- Before this change the interpreter returned `M68KI_HALT_UNIMPL` for every
  `TRAP`. The adapter recovered only opcode `$4E41` (`TRAP #1`) and called
  `abort()` for all other interpreter failures or unexpected stack results.
- The translated ROM contains conventional `TRAP #0` sites at `$04FE2E` and
  `$0564EC`, as well as non-returning `TRAP #1` state transitions. A `TRAP #0`
  reached below the interpreted actor scheduler therefore followed the fatal
  path instead of the already implemented exception-frame runtime.
- The runtime's `m68k_trap_vector()` constructs a six-byte MC68000 format-0
  frame (SR plus return PC), dispatches the vector and accepts the PC restored
  by `RTE`. The existing native synthetic test verifies this frame behavior.

## Implemented correction

The interpreter now supports `TRAP #0` by setting the architectural return PC,
calling vector 32 through `m68k_trap_vector()`, and continuing at the PC restored
by the handler's `RTE`. `TRAP #1` remains deliberately outside the generic path:
in Doom Troopers it discards the actor stack and transfers to another game
state, so `run_actor_scheduler()` retains its explicit handling. Other TRAPs
still fail loudly; no failure was converted into a silent return.

### Continue-screen reproduction

A traced native Windows run reproduced a second crash after player death. At
frame 6,017 a dynamic dispatch reached `$0573BE`, which was absent from the
translated function set. The interpreter fallback ran it until `TRAP #1` at
`$05750A`, correctly declined the non-returning stack transfer, and recorded an
interior-label miss. The scheduler then failed at PC `$000288`, opcode `$4553`,
with A7 `$00FFFFF6`. This proves the failure reached the scheduler and was not
an audio or renderer error.

The correction promotes `$0573BE` to a ROM-validated translated entry point.
Its `TRAP #1` is therefore emitted through the existing native vector path;
the generic interpreter behavior and the separate actor-scheduler handling of
state-changing `TRAP #1` remain unchanged.

The repeated native Windows run passed: the Continue screen opened, selecting
Continue restarted gameplay normally, the process exited with code 0, and the
diagnostic log shows native vector 33 returning from `$05750A` to `$05750C`.
There was no floor decline for `$0573BE`, scheduler fatal, HALT, illegal
instruction or strict JSR-stack mismatch. The tested executable SHA-256 was
`1ab3d08ffdff5abe7b71ab61464fcdd0c399accc00b746ba8e50761919cf5977`.

`cartridge_trap_contract` verifies that the interpreter dispatches `TRAP #0`
and still reports `TRAP #1` to the game adapter. `synthetic_native_trap_rte`
continues to verify the exception frame, SR, PC, A7 and RTE behavior.

### Level-results reproduction

The first native Windows level-completion run reproduced the corrupt transition
at frame 19,621. Runtime evidence identified `$0575FE` as an interior-label
miss. ROM bytes at that address decode as the valid entry beginning with
`BTST #4,$FF0033`, immediately after the `CONTINUE` and `GAME OVER` data. The
static ownership range had incorrectly hidden this code boundary behind the
preceding function and embedded strings.

The correction promotes ROM-validated entry `$0575FE`. Regeneration emitted
`func_0575FE` in the native dispatch table. A repeated windowed Windows run with
Scale2x and launcher mask 3 (infinite energy and lives for player 1) completed
level 1, passed through results and reached interactive level 2. The tested
executable SHA-256 was
`25f912b43eb00f58f05b5210b8b7043c079c01fd0e832efbbe4957ed980480a3`.

## Optional diagnostics

Set `DOOM_TRANSITION_TRACE=1` to log only relevant state changes. Each record
contains the host frame number, the last valid scheduler PC and A7, callback
address and explicitly named work-RAM offsets (their higher-level meanings are
not asserted without symbols). TRAP #1 transfers and fatal exits also include
the relevant PC, opcode, stack and return address. Existing runtime
logs already report renderer/audio initialization failures, illegal instructions,
floor failures and strict JSR-stack mismatches.

To collect a privacy-conscious package while playing manually:

```powershell
py -3 tools/run_transition_diagnostic.py `
  --rom "C:\path\Doom Troopers (USA).md" `
  --exe "build\doom-probe\Release\DoomBootProbe.exe"
```

The resulting ZIP contains only `metadata.json`, `stdout.log` and `stderr.log`.
It contains neither the ROM nor save states, and records hashes instead of file
paths. Review logs before sharing them.

On Bazzite, run the same collector from a repository checkout and point it at
the installed Proton script (adjust the Proton directory if needed):

```bash
export STEAM_COMPAT_CLIENT_INSTALL_PATH="$HOME/.local/share/Steam"
export STEAM_COMPAT_DATA_PATH="$PWD/local/doom-transition-prefix"
mkdir -p "$STEAM_COMPAT_DATA_PATH"
python3 tools/run_transition_diagnostic.py \
  --rom "/path/Doom Troopers (USA).md" \
  --exe "build/doom-probe/Release/DoomBootProbe.exe" \
  --runner "$HOME/.local/share/Steam/steamapps/common/Proton - Experimental/proton"
```

The collector records the executable SHA-256 and the runner's reported version
in `metadata.json`. It does not add the Proton prefix, ROM or saves to the ZIP.

## Required acceptance route

1. Start from reset with the validated USA ROM and no cheats.
2. Enter and complete level 1 through controller input.
3. Observe the results screen long enough to accept input.
4. Continue to level 2 and confirm interactive gameplay.
5. Repeat with Scale2x on/off, window/fullscreen, audio enabled/disabled and a
   clean configuration. Then repeat save-state and safe-cheat checks separately.
6. Run the same matrix on native Windows and Wine/Proton on Linux/Bazzite.

The existing 4,402-frame probe is not acceptance evidence because it does not
complete level 1. A deterministic controller replay for the full route is still
pending; no RAM write or packaged copyrighted save is introduced here.

## Hypothesis versus conclusion

The `TRAP #0` gap and missing `$0573BE`/`$0575FE` native entries are proven
defects. The captured level-results corruption coincided with the `$0575FE`
miss, and the same route passed after its promotion. This validates the native
Windows fix for the tested configuration. It does not yet establish Bazzite,
fullscreen, audio-disabled, save-state or clean/no-cheat matrix coverage.

## Tests unavailable in this environment

- Native Linux, Wine, Proton and Bazzite execution.
- Physical audio, fullscreen/display modes and PS4 controller combinations.

These limitations are release blockers for claiming “Bazzite fixed,” but not
for retaining the isolated interpreter correction and its synthetic regression.

## Tests executed in this checkout

- Release x64 build completed with MSVC.
- CTest: `scale2x_pixels`, `cartridge_trap_contract` and
  `synthetic_native_trap_rte` passed (3/3).
- Python tooling tests passed (7/7), and the diagnostic collector passed
  bytecode compilation.
- The existing deterministic controller replay ran from reset to its scripted
  gameplay exit at frame 4,402 with exit code 0 and strict JSR-stack checking.
  It exercised the corrected build and produced no fatal scheduler, HALT,
  illegal-instruction or stack-mismatch record. It still does not complete the
  level and is therefore a smoke test, not the required transition acceptance.
- A 120-frame smoke run confirmed the optional transition records and clean
  shutdown.
- A native Windows manual run with Scale2x, windowed output and launcher mask 3
  completed level 1, passed the results transition and reached interactive
  level 2. Screenshots captured both the pre-fix corruption and the corrected
  level-2 gameplay.

## Revalidação do build local em 25/11/2026

Executável Windows x64 SHA-256
`28e09b4bdf98708e0b74cddb152f229ce8ff84fec879a94912c7d55d033f47f8`.
A rota automatizada chegou ao quadro 4.402 em ambos os perfis. Uma tentativa
adicional, com energia/vidas de P1, avanço para a direita, tiro e saltos,
encerrou pelo comando do roteiro no quadro 16.401. A captura final ainda
mostra a selva. A tela de resultados e o controle na fase 2 seguem pendentes
para este hash. O passe manual anterior pertence ao hash registrado acima.

### Teste externo em Bazzite

Use uma pasta de teste gravável com o executável deste hash e suas DLLs.
Guarde os saves pessoais separadamente. Use Python 3 e o coletor em
`tools/run_transition_diagnostic.py`, junto de `tools/rom_tool.py` e
`config/rom_identity.json`. No exemplo Proton acima, ajuste os caminhos ao
Steam, ao executável e à sua ROM. O prefixo novo começa sem as preferências
anteriores de Wine. O jogo grava os próprios INIs ao lado do executável.
Para Wine, use os mesmos argumentos de ROM e EXE com
`--runner "$(command -v wine)" --runner-kind wine`. O acesso à ROM pressupõe
o mapeamento padrão Z: do Wine para a raiz Linux.

- [ ] Anotar versão do Bazzite, Wine/Proton, GPU e driver, perfil, filtro,
  janela/tela cheia, áudio e cheats. Começar com Classic e mods desligados.
- [ ] Terminar a primeira fase desde o início e aguardar a tela de resultados.
- [ ] Confirmar os resultados, movimentar e atirar na segunda fase.
- [ ] Repetir com a configuração original do relato, se conhecida.
- [ ] Informar o último passo concluído e anexar o ZIP gerado pelo coletor.

Os logs brutos ficam em `local/level-transition-*/`. O ZIP inclui metadados
e linhas selecionadas de transição, CPU, renderer e temporização. O jogador
pode revisar o ZIP antes de compartilhá-lo. No Windows, a geração e o conteúdo
do ZIP foram conferidos com 120 quadros. A execução do coletor em Wine/Proton
ainda requer o teste externo.
