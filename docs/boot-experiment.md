# Primeiro experimento de execução — 2026-10-27

**Resultado: build e prova sintética aprovados; boot do jogo falhou.**
Não há primeiro quadro validado, telas iniciais estáveis nem gameplay.

## Revisões e licenças

[Manifesto](../config/framework_lock.json) fixa o framework em
`f422ad52a04b9d253c0e9cbcdb3476aabd563ae8` e quatro submódulos de infraestrutura.
Clone local ignorado; nenhum disassembly Sonic foi inicializado. Não há código
upstream nem código do jogo copiado para este repositório.

Foram lidos LICENSE.md e THIRD-PARTY-LICENSES.md locais, além dos avisos dos
cores e submódulos. Framework, m68k-recomp-core e z80-recomp-core usam PolyForm
Noncommercial 1.0.0; rbengine/recomp-net usam MIT. O inventário upstream lista
ymfm BSD-3-Clause, superzazu MIT, clowncommon ISC, minicoro MIT-0/Unlicense com
atribuição LuaCoco, SDL2 zlib e xxHash BSD-2-Clause. Isso permite considerar
pesquisa não comercial, mas não equivale a liberação comercial ou distribuição
dos binários gerados. Não foi feito pacote nem auditoria jurídica completa.

BlastEm 1.0.0 foi baixado da página oficial; hash do ZIP no manifesto.
COPYING.txt contém GPLv3; avisos SDL/GLEW também estão no pacote local.
Foi mantido como processo independente, sem link ao experimento. O arquivo
fonte oficial da revisão 2b2a182f4522 foi consultado para as flags `-b/-d/-t`.
A extração sinalizou links Android incompatíveis com Windows; nenhuma compilação
do BlastEm foi realizada. A ferramenta não depende desses links para a consulta.

## Build e testes automatizados

MSVC 19.44 x64 / Release compilou GenesisRecomp, M68kStepRecomp e o runner
experimental DoomBootProbe com SDL2 e hardware do framework, sem launcher.
Os **64 testes CTest upstream passaram**, incluindo testes de outros subsistemas
como Sega CD. Isso é evidência da suíte, não de compatibilidade do cartucho.

Dois testes próprios adicionais passaram:

- `known_cartridge_trap_gap`: ROM sintética independente confirma que o
  interpretador de cartucho retorna HALT_UNIMPL em TRAP #1. Este teste passa
  quando reproduz a lacuna conhecida; não significa suporte à instrução.
- `synthetic_native_trap_rte`: fixture própria traduzida pelo M68kStepRecomp
  executa cinco operações nativas, verifica vetor 33, frame de exceção de seis
  bytes, PC de retorno, SR, A7, D0 e 70 ciclos nominais. Não é comparação contra
  oráculo independente nem prova de precisão de barramento.

## Execução real da ROM (headless)

O wrapper validou tamanho/SHA-256 antes de gerar C local. A descoberta inicial
reportou 101 funções e 121 após divisões de limites; três locais de fluxo
continuaram não resolvidos. Essas contagens são análise estática, não cobertura
de execução. Nenhum endereço desses relatórios foi promovido cegamente.

Modo traduzido: abortou em vetor 33 (TRAP #1), código Windows 0xC0000409.
O log mostra PC/opcode zerados no abort porque o emissor de cartucho não
atualiza essa informação nessa chamada; não é evidência de execução no PC zero.
Não foi medida a quantidade de blocos nativos retirados antes da falha.

Modo interpretado do mesmo framework: chegou a **PC 0x054F78, opcode 0x4E41**,
retornou HALT_UNIMPL e estacionou a CPU. O host continuou até 120 quadros e saiu
com código zero, portanto o wrapper agora rejeita logs HALT mesmo com exit=0.
A captura local produzida nesse modo não é um primeiro quadro correto.
Não foi validada imagem, áudio nem a ordem dos eventos do jogo. A detecção de
um controle físico pelo host também impede afirmar isolamento dos inputs.

Causa localizada no código upstream: emissor `MN_TRAP` do perfil Genesis chama
`m68k_trap_vector`; em `runner/glue.c` essa função aborta. O `exec_one` de
`runner/m68k_interp.c` não implementa TRAP. A ABI do runner usa handlers de
interrupção sem frame real em alguns caminhos; apenas trocar o abort por uma
chamada de função esconderia o problema de pilha/retorno e não foi feito.

## Referência independente: pendente

BlastEm oficial com `-t -b 180 -d -r U`, comandos por stdin para observar o
mesmo PC: terminou com **0xC0000005**, sem trace útil. O resultado se repetiu
fora do sandbox. Smoke headless sem depurador atingiu watchdog de 30 s;
tentativa SDL software com debugger também não terminou em 20 s. Esses
resultados não provam defeito no emulador nem na ROM; falta diagnosticar a
automação. A consulta `-h` abriu uma caixa de informação Windows, encerrada
sem alterar o sistema. Nenhuma comparação independente foi concluída.

## Reprodução

No PowerShell, no checkout:

```powershell
python tools/setup_framework.py
cmake -S local/segagenesisrecomp/tests -B build/framework-tests -G "Visual Studio 17 2022" -A x64
cmake --build build/framework-tests --config Release --parallel 4
ctest --test-dir build/framework-tests -C Release --output-on-failure
cmake -S experiments/boot -B build/doom-probe -G "Visual Studio 17 2022" -A x64 -DDOOM_ROM="C:/caminho/Doom Troopers (USA).md"
cmake --build build/doom-probe --config Release --parallel 4
ctest --test-dir build/doom-probe -C Release --output-on-failure
python tools/run_boot_probe.py --rom "C:/caminho/Doom Troopers (USA).md" --exe build/doom-probe/Release/DoomBootProbe.exe
python tools/run_boot_probe.py --rom "C:/caminho/Doom Troopers (USA).md" --exe build/doom-probe/Release/DoomBootProbe.exe --interpret
```

As duas execuções do jogo devem falhar nesta revisão. Logs/capturas permanecem
em `local/` e saída gerada em `build/`. Nunca anexá-los ao GitHub. O executável
experimental verifica CRC/tamanho upstream; **a porta SHA obrigatória está nos
wrappers**, por isso não usar o exe diretamente como launcher de distribuição.

Os testes próprios também foram compilados e executados sem fornecer ROM:

```powershell
cmake -S experiments/boot -B build/trap-only -G "Visual Studio 17 2022" -A x64
cmake --build build/trap-only --config Release --target TrapGapRegression TrapNativeProof --parallel 4
ctest --test-dir build/trap-only -C Release --output-on-failure
```

Para reproduzir a tentativa de referência, baixar manualmente o ZIP oficial
do manifesto, verificar SHA e extrair em `local/blastem/`; então:

```powershell
python tools/run_reference.py --rom "C:/caminho/Doom Troopers (USA).md" --exe local/blastem/blastem-win64-1.0.0/blastem.exe
```

## Próxima decisão técnica

Avaliar a ABI explícita por instrução, que passou na prova TRAP/RTE, integrada
ao barramento/scheduler Genesis; ou implementar exceções reais de forma
consistente na ABI de cartucho, incluindo retorno e suspensão de fibers.
Antes de executar mais quadros, validar contra oráculo independente: modo
usuário/supervisor, pilhas, trace, IRQs aninhadas e custos/acessos de exceção.
Resolver automação da referência (ou trocar por outra depurável com licença
conferida). Nenhum dos dois caminhos está implementado neste experimento.
