# Menu integrado v0.1.7 para Windows x64

O executável abre um diálogo Win32 quando iniciado sem argumentos. A interface usa controles nativos e a entrada de controle da SDL2. Jogar entrega a ROM validada ao ponto de entrada do runtime no mesmo processo. Um argumento de ROM, opções de automação ou a variável GENESIS_NO_LAUNCHER preservam o fluxo direto. O bit do cheat experimental Pular fases é descartado também na entrada direta.

## Reproduzir a candidata

No checkout com o framework fixado e os patches do projeto aplicados, configure um diretório separado. Substitua os caminhos do framework e da ROM pelos caminhos locais. O hash obtido ao final identifica essa compilação específica.

```powershell
cmake -S experiments/boot -B build/integrated -G "Visual Studio 17 2022" -A x64 "-DRECOMP_ROOT=C:/framework" "-DDOOM_ROM=D:/Jogos/Doom Troopers (USA).md" -DDOOM_INTEGRATED_MENU=ON "-DGENESIS_GAME_VERSION:STRING=0.1.7"
cmake --build build/integrated --config Release
ctest --test-dir build/integrated -C Release --output-on-failure
$exe = "build/integrated/Release/DoomBootProbe.exe"
./launcher/package.ps1 -Integrated -FrameworkRoot C:/framework -GameBuild $exe -GameSha256 (Get-FileHash $exe).Hash -OutputDirectory build/integrated-output
```

## Validação da candidata local

- SHA-256 do EXE entregue `f543d5f75003e081265ec790d061dbc4c0b5da2ede800a559a49c3c1a2b0f97a`.
- SHA-256 do ZIP entregue `20f4706c4285a9010c59955aa29b28b69ca55370eb68edeec4505eb24405a236`.
- Build Release, CTest da raiz e os três CTests nativos passaram. O ZIP contém oito entradas. CRCs, manifesto e SHA-256 externo conferidos, sem ROM integral, launcher separado, configurações pessoais ou saves.
- A automação da janela testou o seletor Windows, a rejeição de uma ROM inválida de mesmo tamanho, início do jogo, PT/EN, remapeamento, preferências de vídeo, leitura do caminho legado em Base64 e segunda abertura. A lista continha 30 opções e recusou a ativação de Pular fases.
- O caminho direto chegou ao quadro 4.402 e reproduziu as quatro capturas da candidata Direct por SHA-256. Com Scale2x desligado, outro roteiro conferiu a energia de P1 e a gravação e restauração de estado. Os testes usaram renderização por software e áudio SDL simulado a 48 kHz.
- Pendentes de validação manual em Windows x64 limpo: apresentação em monitor físico, áudio audível, navegação pelo controle, ações remapeadas em gameplay, gatilhos L2/R2 e efeitos individuais dos demais cheats. O seletor de ROM usa a navegação padrão do Windows.

Em 20/11/2026, o mantenedor informou 0/70 para o executável e 0/66 para o ZIP nos relatórios individuais do VirusTotal. Essas contagens são específicas da data e das amostras. A revisão dos direitos de distribuição das instruções recompiladas do jogo incorporadas ao executável permanece pendente. [Executável](https://www.virustotal.com/gui/file/f543d5f75003e081265ec790d061dbc4c0b5da2ede800a559a49c3c1a2b0f97a) · [ZIP](https://www.virustotal.com/gui/file/20f4706c4285a9010c59955aa29b28b69ca55370eb68edeec4505eb24405a236)
