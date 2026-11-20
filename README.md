# Doom Troopers (USA) — PC Recomp

[English](README.en.md)

**v0.1.7 Integrated** é a distribuição portátil principal de Doom Troopers PC Recomp para Windows x64. O menu gráfico faz parte de `DoomBootProbe.exe`. O pacote inclui SDL2 e as bibliotecas necessárias do Visual C++ e dispensa instalador, launcher separado e .NET Framework.

[Baixar ZIP Windows x64](https://github.com/emanuelcnjunior/Doom-Troopers-PC-Recomp/releases/download/v0.1.7/Doom-Troopers-PC-Recomp-v0.1.7-Windows-x64-Integrated.zip) · [Arquivo SHA-256](https://github.com/emanuelcnjunior/Doom-Troopers-PC-Recomp/releases/download/v0.1.7/Doom-Troopers-PC-Recomp-v0.1.7-Windows-x64-Integrated.zip.sha256) · [Notas da versão](https://github.com/emanuelcnjunior/Doom-Troopers-PC-Recomp/releases/tag/v0.1.7)

## Instalação e execução

1. Extraia o ZIP inteiro em uma pasta gravável.
2. Abra `DoomBootProbe.exe` sem argumentos e escolha português ou inglês no topo da janela.
3. Em **Jogo**, selecione sua ROM de **Doom Troopers (USA)**, obtida legalmente. O menu confere o tamanho e o SHA-256 da revisão USA.
4. Ajuste as opções em **Jogo**, **Controles** e **Cheats**. Clique em **Jogar** para salvar as preferências e iniciar a partida.

Para abrir diretamente pelo PowerShell, passe o caminho da ROM entre aspas. Esse fluxo usa a validação de tamanho e CRC32 do runtime.

```powershell
.\DoomBootProbe.exe "D:\Jogos\Doom Troopers (USA).md"
```

## Controles e opções

| Função | Teclado | Controle PS4 do jogador 1 |
| --- | --- | --- |
| Mover / iniciar | Setas / Enter | Direcional / Options |
| Atirar / pular / atacar | X / C / Z | Quadrado / X (cruz) / bolinha |
| Salvar / carregar o slot 1 | Shift+F1 / F1 | L2 / R2 |

Em **Controles**, escolha três teclas e três botões distintos para remapear as ações. Em **Jogo**, escolha janela ou tela cheia e ligue ou desligue o filtro Scale2x. F11 alterna a tela cheia durante a partida. O menu reutiliza `launcher.ini` e `settings.ini`. Para migrar preferências, copie seus INIs com o jogo fechado. Eles podem conter o caminho local da ROM.

Em **Cheats**, selecione uma opção e use **Ativar/desativar** ou clique duas vezes. **Pular fases** permanece experimental e desabilitado por instabilidade. O efeito de energia de P1 foi testado. Os demais efeitos ainda exigem validação individual.

## Validação

O build Release e os quatro testes CTest passaram. O roteiro de gameplay chegou ao quadro 4.402 com quatro capturas idênticas às da candidata Direct. O teste automatizado gravou e restaurou um save state. Continuam pendentes os testes em outro Windows x64, monitor e áudio físicos, navegação e remapeamento no controle PS4, L2/R2 e os demais cheats. [Relatório e instruções de build](docs/integrated-menu-v0.1.7.md)

SHA-256 do ZIP: `20f4706c4285a9010c59955aa29b28b69ca55370eb68edeec4505eb24405a236`. No VirusTotal em **20/11/2026**, o executável registrou **0/70** e o ZIP **0/66**. São resultados daquela data e não garantem ausência de ameaças. O histórico do launcher separado permanece na auditoria da versão anterior. [Executável](https://www.virustotal.com/gui/file/f543d5f75003e081265ec790d061dbc4c0b5da2ede800a559a49c3c1a2b0f97a) · [ZIP](https://www.virustotal.com/gui/file/20f4706c4285a9010c59955aa29b28b69ca55370eb68edeec4505eb24405a236) · [Auditoria v0.1.6](docs/security-audit-v0.1.6.md)

## Projeto, créditos e licenças

O projeto pesquisa a recompilação de código 68000 de Mutant Chronicles: Doom Troopers (Sega Genesis/Mega Drive). A organização de código próprio, geração local e documentação foi inspirada no TMNT IV PC Recomp. Nenhum código ou recurso desse projeto foi copiado. [TMNT IV PC Recomp](https://github.com/emanuelcnjunior/TMNT-IV-Turtles-in-Time-PC-Recomp)

O arquivo `LICENSES.txt` reúne a licença PolyForm Noncommercial do framework e os avisos das bibliotecas distribuídas. Os direitos do jogo pertencem aos respectivos titulares. A ROM é fornecida pelo jogador e não acompanha a distribuição. **Permanece pendente a revisão dos direitos de distribuição das instruções do jogo recompiladas e incorporadas ao executável.** Os resultados do VirusTotal não resolvem essa questão.

Para desenvolvimento, consulte a documentação do projeto. [Experimento de execução](docs/boot-experiment.md) · [Arquitetura](docs/architecture.md) · [Pesquisa](docs/research.md) · [Auditoria](docs/audit.md) · [Protocolo de comparação](docs/validation.md)
