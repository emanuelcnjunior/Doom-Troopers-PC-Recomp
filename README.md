# Doom Troopers (USA) — PC Recomp

[English](README.en.md)

Pesquisa para um port Windows x64 de Mutant Chronicles: Doom Troopers,
do Sega Genesis/Mega Drive, por recompilação 68000.
**Etapa 0: ferramentas de validação. Não existe jogo executável, boot validado ou instalador.**

O [primeiro experimento de execução](docs/boot-experiment.md) compila em Release,
mas o boot falha em TRAP #1 no framework candidato. Há uma prova sintética
de exceção/retorno nativos; a comparação independente do jogo segue pendente.

A ROM não está incluída e nunca deve ser enviada ao GitHub. Forneça sua cópia
local binária; a extensão `.md` não significa Markdown neste caso.

```powershell
python tools/rom_tool.py validate "C:\caminho\Doom Troopers (USA).md"
python tools/rom_tool.py inspect "C:\caminho\Doom Troopers (USA).md"
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Python 3.10+ e CMake 3.24+ bastam para esta etapa. O build verifica as ferramentas;
não compila um runtime. `inspect` só processa a identidade aceita e imprime
metadados; não exporta código, gráficos ou áudio. Não versionar sua saída local.
Outras extensões são aceitas somente se tamanho e SHA-256 coincidirem.

Veja [arquitetura](docs/architecture.md), [opções técnicas](docs/research.md),
[auditoria](docs/audit.md) e [protocolo de comparação](docs/validation.md).
Nenhuma dependência de execução foi incorporada. SNESRecomp não é um backend Genesis.

A organização separa código próprio, geração local e documentação, inspirada no
[TMNT IV PC Recomp](https://github.com/emanuelcnjunior/TMNT-IV-Turtles-in-Time-PC-Recomp).
Nenhum código ou recurso desse projeto foi copiado. Direitos do jogo pertencem
aos respectivos titulares; este projeto não concede licença sobre a ROM.
