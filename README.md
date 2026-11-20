# Doom Troopers (USA) — PC Recomp

[English](README.en.md)

Pesquisa para um port Windows x64 de Mutant Chronicles: Doom Troopers,
do Sega Genesis/Mega Drive, por recompilação 68000.
**Protótipo experimental: boot, menus e gameplay da fase 1 funcionam; ainda não
há instalador nem validação completa contra um emulador de referência.**

O [experimento de execução](docs/boot-experiment.md) compila em Release. O
suporte local a TRAP/RTE e à pilha de chamadas permitiu percorrer de forma
automatizada os logos, o título, a seleção de personagem, o briefing e a fase 1.
Os testes mantêm a verificação estrita da pilha JSR habilitada.

## Testar em outro computador

Extraia o [pacote Windows x64 v0.1.4](releases/Doom-Troopers-PC-Recomp-v0.1.4-win64.zip)
e abra `Launcher.exe`. Escolha sua ROM USA e clique em **Jogar**. A tela cheia
vem ativada. O controle PS4 do jogador 1 usa quadrado para atirar, X para pular
e bolinha para atacar com a arma. O launcher permite trocar esses bot?es e
selecionar os 30 cheats da revis?o USA. L2 salva no slot 1 e R2 carrega o mesmo slot. As op??es s?o
salvas para a pr?xima partida. Tamb?m ? poss?vel arrastar a ROM sobre `Jogar.bat`.

O pacote nunca inclui a ROM. Ele contém o executável experimental, SDL2 e as
bibliotecas redistribuíveis do Visual C++ necessárias para outro Windows x64.
Windows pode exibir um aviso por se tratar de um binário experimental sem
assinatura digital.

A ROM não está incluída e nunca deve ser enviada ao GitHub. Forneça sua cópia
local binária; a extensão `.md` não significa Markdown neste caso.

```powershell
python tools/rom_tool.py validate "C:\caminho\Doom Troopers (USA).md"
python tools/rom_tool.py inspect "C:\caminho\Doom Troopers (USA).md"
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Python 3.10+, CMake 3.24+, Visual Studio 2022 e SDL2 do framework fixado são
usados pelo protótipo. `inspect` só processa a identidade aceita e imprime
metadados; não exporta código, gráficos ou áudio. Não versionar sua saída local.
Outras extensões são aceitas somente se tamanho e SHA-256 coincidirem.

Veja [arquitetura](docs/architecture.md), [opções técnicas](docs/research.md),
[auditoria](docs/audit.md) e [protocolo de comparação](docs/validation.md).
Nenhuma dependência de execução foi incorporada. SNESRecomp não é um backend Genesis.

A organização separa código próprio, geração local e documentação, inspirada no
[TMNT IV PC Recomp](https://github.com/emanuelcnjunior/TMNT-IV-Turtles-in-Time-PC-Recomp).
Nenhum código ou recurso desse projeto foi copiado. Direitos do jogo pertencem
aos respectivos titulares; este projeto não concede licença sobre a ROM.
