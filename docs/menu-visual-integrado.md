# Menu visual integrado — candidata local para Windows x64

Sem argumentos, DoomBootProbe.exe abre a janela de configuração. A primeira tela mostra **Controls**, como na referência. A capa na lateral é o botão **Game**. Os botões **Controls** e **Cheats** alternam o conteúdo à direita. O campo de ROM, Browse, Fullscreen, Scale2x e Play ficam acessíveis nas três telas. A coluna de teclado ao lado dos botões PS4 preserva o remapeamento da edição Integrated.

## Arquitetura e arquivos

| Arquivo | Responsabilidade na revisão visual |
| --- | --- |
| `experiments/boot/menu.rc` | Layout em unidades de diálogo do Windows, ordem de navegação e incorporação da capa existente `image/Screenshot_3.jpg` como recurso de leitura. |
| `experiments/boot/menu_ids.h` | Identificadores dos elementos visuais, da imagem e do botão de restauração. |
| `experiments/boot/menu_skin.h`, `experiments/boot/menu_skin.cpp` | Cores, fontes e pintura da barra lateral, capa, título e botões. O diálogo recebe as mensagens Win32 de desenho. GDI+ lê o JPEG de um fluxo em memória criado com SHCreateMemStream. Os recursos gráficos são liberados ao fechar o diálogo, antes do gameplay. |
| `experiments/boot/menu.cpp` | Textos PT/EN, seleção das telas e restauração dos seletores. Restore defaults usa os padrões já existentes para as três ações. As escolhas são gravadas pelos mesmos caminhos de **Play** em launcher.ini e settings.ini. |
| `experiments/boot/CMakeLists.txt` | Compila a camada visual no alvo integrado e vincula GDI+ e Shlwapi, fornecidos pelo Windows. |
| `docs/menu-visual-integrado.md` | Arquitetura, build e validação desta candidata. |

Os controles continuam sendo janelas nativas com foco, Tab e entrada pelo teclado. Os botões de navegação usam desenho próprio; as listas, caixas de seleção, campo de texto e seletor de arquivos continuam Win32. A navegação por controle usa o tratamento SDL2 da Integrated. O código do runtime, as regras do catálogo de cheats, a validação da ROM e o formato dos INIs são os da v0.1.7.

## Build e pacote

Use os Build Tools do Visual Studio 2022 com C++ x64 e o Windows SDK, CMake, Python e o framework fixado com os patches do projeto. Substitua os caminhos do framework e da ROM nas linhas abaixo. Use um diretório de saída novo para cada candidata.

```powershell
$framework = (Resolve-Path local/phase-framework).Path
$rom = "D:/Jogos/Doom Troopers (USA).md"
cmake -S experiments/boot -B build/restyle -G "Visual Studio 17 2022" -A x64 "-DRECOMP_ROOT=$framework" "-DDOOM_ROM=$rom" -DDOOM_INTEGRATED_MENU=ON "-DGENESIS_GAME_VERSION:STRING=0.1.7"
cmake --build build/restyle --config Release --parallel 4
ctest --test-dir build/restyle -C Release --output-on-failure
cmake -S . -B build/restyle-tools
ctest --test-dir build/restyle-tools -C Release --output-on-failure
$exe = "build/restyle/Release/DoomBootProbe.exe"
./launcher/package.ps1 -Integrated -FrameworkRoot $framework -GameBuild $exe -GameSha256 (Get-FileHash $exe).Hash -OutputDirectory build/restyle-output
```

## Validação

As verificações da janela usaram uma cópia isolada. O fluxo cobriu abertura sem argumentos, Game, Controls e Cheats, 30 opções, bloqueio de Skip levels, restauração dos padrões, idioma, seleção de ROM pelo diálogo nativo, rejeição de arquivo inválido e início com ROM USA. Uma segunda abertura recuperou o caminho e as preferências salvas. Os quatro CTests passaram.

A candidata contém um único EXE. A comparação entrada por entrada com o ZIP publicado mostrou diferenças somente em DoomBootProbe.exe e no manifesto SHA256SUMS.txt. O Defender local informou zero ameaças no novo EXE. A análise do VirusTotal para a versão publicada aplica-se ao hash daquela versão; o novo hash requer análise independente. O teste automatizado da janela não substitui a conferência física com controle PS4 e monitor em outras escalas de DPI.

## Artefato final desta revisão

O ZIP local de interface redesenhada é `build/restyle-candidate/Doom-Troopers-PC-Recomp-v0.1.7-Windows-x64-Integrated.zip`, SHA-256 `950db9526967fe51c54b31dd2dd9b73be4ab704235eabaeed7fd32b23e3d3dc4`. O executável `DoomBootProbe.exe` tem SHA-256 `2dca0b5ecc0709da0639ec41969767326e7cb16ab879c134bf71fa5bcd3c769b`. O ZIP passou pela verificação de CRC, hashes do manifesto e ausência da ROM.
