# Auditoria — 2026-10-27

Checkout inicialmente só `.git`, `main` sem commits; origin correto. Nenhum
AGENTS.md encontrado. README TMNT consultado via API, sem copiar código.
Windows reportado pelo CMake: 10.0.26200, SDK 10.0.26100.0. Git disponível,
CMake 4.4.4, Ninja 1.13.2, Python 3.13.15; VS2022 Build Tools e MSVC
14.44.35207 Hostx64/x64 instalados, `cl` fora do PATH. O projeto atual usa
LANGUAGES NONE e não certifica compilação C/C++.
Rede/Ninja bloqueados no sandbox funcionaram fora dele com autorização.
GitHub autenticado e remoto vazio confirmados; nenhum token armazenado aqui.

ROM encontrada em Downloads/temp/Doom Troopers (USA), sem copiar ao checkout.
`rom_tool.py inspect` confirmou 2097152 bytes, SHA-256 do manifesto, CRC32
11194414, SEGA GENESIS, MUTANT CHRONICLES DOOMTROOPERS e GM T-132066-00.
SP inicial 0x00FFFFFE, reset PC 0x000568A8, nível 4 0x0000020A,
nível 6 0x00000200. ROM declarada 0x00000000–0x001FFFFF e RAM
0x00FF0000–0x00FFFFFF; região textual 4, marcador SRAM RA ausente.
Metadados não comprovam execução, mapper nem ausência de SRAM.

Build: CMake e alvo check via MSBuild executados, sem gerar jogo.
Automatizados: dois testes sintéticos de identidade e metadados, cobrindo
tamanho, hash, extensão neutra, ausência de arquivo e big-endian.
Manual: validação/inspeção real da ROM. Nenhuma janela ou gameplay testado.
CTest inicial sem -C falhou no gerador Visual Studio; comandos corrigidos
para -C Release e repetidos antes do commit.

CPU, barramento, VDP, Z80, áudio, scheduler e tradução ainda não implementados.
Nenhum emulador executado, framebuffer produzido ou comparação por quadro.
Sem picker, saves, HD, launcher ou instalador. Entrega limitada à etapa 0.
