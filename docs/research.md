# Opções técnicas — 2026-10-27

Pesquisa documental, sem teste de compatibilidade com Doom Troopers.
Nenhum componente incorporado. Fixar revisão por SHA antes da integração.

| Opção | Arquitetura / maturidade | Licença / decisão |
|---|---|---|
| [segagenesisrecomp](https://github.com/mstan/segagenesisrecomp) | Tradução 68000 para C, runtime Genesis e fallback; projeto ativo ainda beta. | PolyForm Noncommercial 1.0.0. Primeiro candidato experimental; compatibilidade não demonstrada. |
| [Musashi](https://github.com/kstenerud/Musashi) | Interpretador 680x0 C com histórico de uso no MAME; não fornece hardware Genesis. | Permissiva com aviso obrigatório; possível oráculo CPU, não recompilação. |
| [BlastEm](https://www.retrodev.com/blastem/) | Emulador Genesis, candidato a referência depurável externa. | Conferir COPYING da revisão escolhida antes de incorporar/distribuir; licença ainda não auditada. |
| [Genesis Plus GX](https://github.com/ekeeke/Genesis-Plus-GX) | Emulador de plataforma estabelecido; segunda referência possível. | Licença específica não comercial; não copiar core automaticamente. |
| Tradutor próprio | Controle da semântica, mas exige decoder, flags, exceções, descoberta e runtime completo. | Código próprio; custo alto, alternativa se o candidato falhar. |

O candidato principal usa geração C e estimativas de timing; isso não prova
precisão dos acessos ao barramento. Sua seleção depende de testes diferenciais.
Revisar [licença](https://github.com/mstan/segagenesisrecomp/blob/master/LICENSE.md)
e [terceiros](https://github.com/mstan/segagenesisrecomp/blob/master/THIRD-PARTY-LICENSES.md)
na revisão fixada, incluindo submódulos 68000/Z80. Não há dependências neste bootstrap.
SNESRecomp não implementa esta plataforma.
