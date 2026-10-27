# Protocolo diferencial proposto

Ainda não executado. Capturas/traces/áudio ficam em `local/` ignorado;
publicar somente resumos sem conteúdo do jogo.

Fixar hash da ROM, revisão/build do runtime e oráculo independente, modelo
Genesis NTSC USA, power-on/reset, RAM inicial, input por tick, modo de vídeo
e taxa de áudio. Registrar ticks master, estado CPU/SR, barramento, IRQ assert/ack,
DMA, escritas VDP/YM/PSG e fronteiras de quadro em ordem determinística.

Comparar CPU em limites de instrução/bloco e encontrar primeira divergência.
Por quadro comparar dimensões, hash e pixels brutos antes de shaders. Áudio:
comparar escritas/ticks antes de amostras, alinhar fase/latência do resampler,
declarar tolerância numérica antes do teste e medir drift. Não substituir
timing NTSC por 60 fps do host nem normalizar divergência silenciosamente.

Sequências: boot sem entrada, título/opções/seleção, personagens, primeira
fase, entradas simultâneas P1/P2 se suportado, transições e reset. Cada falha
deve incluir comando, versões, input reproduzível, tick inicial, esperado/obtido
e teste sintético mínimo quando possível. Não anexar dumps do jogo.

Separar build, testes automatizados e validação manual. Etapa 1 só passa com
sequência inicial estável repetível sem acessos desconhecidos/hacks, com
duração e checkpoints definidos pela referência antes de declarar sucesso.
