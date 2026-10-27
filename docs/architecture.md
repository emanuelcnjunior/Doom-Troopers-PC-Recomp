# Arquitetura proposta

Etapa 0, sem runtime. Destino Windows x64. Avaliar tradução estática 68000
para C com runtime Genesis e fallback medido. Se nenhum bloco traduzido for
executado, classificar o resultado como emulação. Código gerado permanece
em `build*/generated/`, nunca no Git. Dependências precisam de SHA fixo e notices.

## Hardware e evidência

| Região | Evidência ou hipótese a verificar |
|---|---|
| ROM 0x000000–0x1FFFFF | Cabeçalho validado |
| RAM 0xFF0000–0xFFFFFF | Cabeçalho validado |
| Z80 0xA00000–0xA0FFFF | Janela de hardware; espelhos e arbitragem pendentes |
| I/O 0xA10000 | Endereços da plataforma, uso pelo jogo ainda não rastreado |
| BUSREQ/RESET 0xA11100/0xA11200 | Arbitragem Z80 pendente |
| VDP dados/controle 0xC00000/0xC00004 | Hardware ainda não implementado |
| PSG 0xC00011 | Hardware ainda não implementado |

Revisar mapa, espelhos e timing contra as
[Sega Genesis Reference Sheets](https://segaretro.org/images/c/cf/SegaGenesisReferenceSheets_1992-11-05.pdf)
antes de implementar. A leitura integral desse manual não foi feita nesta execução.
Banco serial em 0x6000 do espaço Z80 dá acesso ao barramento 68000;
não é mapper do cartucho. Mapper especial e SRAM não foram demonstrados.
Ausência de marcador SRAM no cabeçalho não encerra essa investigação.

Separar estado 68000, barramento big-endian de 24 bits, Z80, VDP, áudio,
entradas e scheduler de ticks inteiros do master clock. SDL será host de
apresentação, nunca relógio da máquina. Validar NTSC, divisores 7/15 para
68000/Z80, linhas e modos H32/H40 com a referência de hardware.

68000 exige flags, supervisor, alinhamento, exceções, autovetores e ordem dos
acessos. Descobrir fluxo a partir dos vetores validados e traces; saltos
indiretos e código em RAM exigem invalidação/fallback. Não traduzir dados como código.
VDP exige VRAM/CRAM/VSRAM, latch, FIFO, DMA/contenção, tiles, sprites/limites,
scroll, prioridades, contadores e IRQs. HBlank não implica automaticamente
H-int: modelar contador, enable e acknowledge. Não inserir patches para pular telas.
Z80 exige RAM de 8 KiB, espelhos, reset, BUSREQ e banking. YM2612 requer FM,
timers/DAC e SN76489 requer PSG, preservando ticks e fase do resampler.
Entradas exigem handshake TH e evidência de dois jogadores.

## Roteiro e aceitação

1. Etapa 0 atual: identidade, ferramentas e pesquisa. Próximo: pin/build do
   candidato, revisão de licenças e referência depurável.
2. POC: comparar programa sintético próprio por interpretação e C traduzido,
   estado e ciclos. Depois gerar localmente da ROM validada, executar boot
   headless e medir blocos nativos/fallback. Acesso desconhecido falha com PC/tick.
3. Etapa 1: primeiro quadro real em janela e telas iniciais estáveis, comparadas
   por protocolo diferencial. Abrir janela vazia não passa.
4. Etapa 2: menus, opções, ambos os personagens, P1/P2 se suportado, primeira
   fase e múltiplas transições em Release; comparar colisões, tiros, HUD e scroll.
5. Etapa 3: launcher, picker SHA, controles, proporção original, escala e VSync.
   Saves somente com CPU, Z80, VDP/FIFO/DMA, RAM, áudio, relógios, periféricos e
   SRAM se houver. Integridade, versão e escrita atômica em
   `%LOCALAPPDATA%\DoomTroopersPCRecomp\saves`.
6. Etapa 4: baseline aprovado antes de filtros/HD e instalador. Testar Windows
   limpo, notices, checksums e docs bilíngues; preservar perfil em atualização
   e desinstalação salvo escolha explícita. Distribuição exige revisão própria.

Riscos: saltos indiretos, código em RAM, flags/exceções, DMA/IRQ, sincronização
de áudio e licenças restritivas. Tela de título isolada não fecha esses riscos.
