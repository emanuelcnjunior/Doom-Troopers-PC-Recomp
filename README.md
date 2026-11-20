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

**Aviso sobre o launcher** — Na análise consultada, Arctic Wolf, Bkav Pro, CrowdStrike Falcon, Elastic e SecureAge sinalizaram o `Launcher.exe`. O Microsoft Defender local verificou o ZIP e os arquivos extraídos sem detectar ameaças. A hipótese de falso positivo ainda aguarda confirmação dos fornecedores. [Relatório do launcher](https://www.virustotal.com/gui/file/a6df110cd9e7e9d0a8dab5c7a68fd6549cfbdb10cd6274c8ddddd7a7ce68f898/detection).

SHA-256 do ZIP: `dc56ca516347bc22aeb8aba7c996b9b51d8022fe767cfd7aa21926945c780772`.

Extraia o [pacote Windows x64 v0.1.5](releases/Doom-Troopers-PC-Recomp-v0.1.5-win64.zip)
e abra `Launcher.exe`. Escolha sua ROM USA e clique em **Jogar**. A tela cheia
vem ativada. O jogo usa o filtro Scale2x e tem alvo de 60 fps. Desmarque
**Filtro Scale2x** no launcher para voltar aos pixels originais. O controle PS4 do jogador 1 usa quadrado para atirar, X para pular
e bolinha para atacar com a arma. O launcher permite trocar esses botões e
selecionar os 30 cheats da revisão USA. L2 salva no slot 1 e R2 carrega o mesmo slot. As opções são
salvas para a próxima partida. Também é possível arrastar a ROM sobre `Jogar.bat`.

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
