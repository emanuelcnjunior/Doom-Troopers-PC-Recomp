# Auditoria da distribuição Windows x64 v0.1.6

## Investigação do alerta na própria v0.1.6

**Resultado individual informado pelo mantenedor em 20/11/2026.** Launcher.exe teve **6/71** e DoomBootProbe.exe **0/71**. O ZIP teve 2/64, Bkav Pro e Elastic, com os mesmos rótulos observados no launcher. Na seção Relations, SDL2.dll e msvcp140.dll tiveram 0/70, e vcruntime140.dll e vcruntime140_1.dll tiveram 0/71. Os resultados individuais identificam a amostra a encaminhar aos fabricantes. A interface pública do VirusTotal não disponibilizou os eventos detalhados nesta sessão. [Relatório do ZIP](https://www.virustotal.com/gui/file/ad9ce9a0bfe84419574dbb50ea5160626208195ca9a64672bfb23082d8ba4f74)

| Mecanismo | Rótulo informado para o launcher |
| --- | --- |
| Arctic Wolf | Unsafe |
| Bkav Pro | W32.Malware.1B22B8B7 |
| CrowdStrike Falcon | Win/malicious_confidence_90% (D) |
| Elastic | Malicious (moderate Confidence) |
| Microsoft | Trojan:Win32/Wacatac.B!ml |
| SecureAge | Malicious |

| Amostra original extraída para consulta manual | Bytes | SHA-256 e relatório individual |
| --- | ---: | --- |
| build/vt-individual/Launcher.exe | 325.120 | [`9f9a9aecb9ead6266b433b9bdabedec89e6c8c662cbbf87fccfc6945e011b518`](https://www.virustotal.com/gui/file/9f9a9aecb9ead6266b433b9bdabedec89e6c8c662cbbf87fccfc6945e011b518) |
| build/vt-individual/DoomBootProbe.exe | 963.072 | [`a57db69a949fa2a7d41384724d4b2805790a9275ea9f47bdc074478f8ff5f4e3`](https://www.virustotal.com/gui/file/a57db69a949fa2a7d41384724d4b2805790a9275ea9f47bdc074478f8ff5f4e3) |

### Revisão prioritária do launcher

**WMI e depuração.** O exame dos metadados CLR por reflexão somente de leitura resolveu as 176 referências a membros. O assembly referencia as bibliotecas do .NET Framework descritas abaixo. No fonte, nos membros referenciados e nas duas declarações P/Invoke não encontramos consulta WMI, chamada a Debugger.IsAttached ou API nativa de detecção de depurador. As referências a System.Diagnostics no caminho de execução são para ProcessStartInfo e Process.Start. Para atribuir um evento observado no sandbox a este processo, são necessários o PID, o processo pai, a operação e sua pilha ou linha de comando. Esses dados não estavam disponíveis. Uma análise estática não cobre todo o comportamento transitivo do Windows, do CLR e dos processos filhos.

**Ofuscação e recursos.** O script invoca diretamente o compilador, com otimização, ícone do projeto, cover.jpg e runtime.sha256. A comparação integral com o build do fonte e desses recursos explica o conteúdo do executável distribuído. Não identificamos etapa de ofuscação nem carga executável extraída dos recursos. O Base64 aparece na leitura e gravação do caminho UTF-8 da ROM em launcher.ini. A presença de um identificador gerado pelo compilador ou de Base64 não revela a causa das classificações.

**Recompilação de controle.** O csc.exe do .NET Framework x64 apresentou assinatura Microsoft Windows válida e versão de arquivo 4.8.9221.0. O build otimizado desta revisão ficou em build/launcher-review-release/Launcher.exe, com SHA-256 `c0d19744edbec4f06a5f5fab4e031aac097b2b2354a8b79b5d93c1869dd609c8`. Os bytes coincidiram com a amostra sinalizada após normalizar o timestamp PE e o GUID gerado pelo compilador em suas duas representações. O hash divulgado para revisão pelos fornecedores continua sendo o da amostra original da tabela. O compilador Roslyn instalado com os Build Tools é uma opção para builds futuros com saída determinística sob entradas fixas. A diferença comprovada é de rastreabilidade. Os achados não justificam trocar o compilador, migrar o WinForms ou alterar a experiência do jogador nesta investigação. [Opções oficiais do compilador C#](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/compiler-options/code-generation)

**Defender local.** MpCmdRun -SignatureUpdate retornou 0 e informou que não havia atualização necessária. O utilitário exibiu 1.463.378.0 ao iniciar a consulta. Em seguida, Get-MpComputerStatus informou antivírus ativo, assinaturas 1.463.387.0, última atualização 20/11/2026 às 06:37:17 e DefenderSignaturesOutOfDate=False. A varredura individual sem remediação do Launcher.exe original retornou 0 e informou zero ameaças. Permanece a divergência com Wacatac.B!ml no VirusTotal. Não há laudo do fabricante explicando essa diferença.

**Teste do comportamento.** Os quatro testes CTest passaram. Em uma extração isolada, o launcher em português recusou uma ROM incorreta e iniciou o DoomBootProbe.exe da pasta esperada com a ROM USA. Na segunda abertura, sem argumento, recuperou o caminho salvo e iniciou o mesmo executável. Foram conferidos os três mapeamentos PS4 não padronizados, o idioma, a máscara de cheat 1, Scale2x desligado e o modo janela nos INIs após as duas execuções. Outra cópia com SDL2 adulterada apresentou o erro de integridade. Esse ensaio cobre a persistência das opções; efeitos individuais dos 30 cheats e resposta de hardware permanecem nas pendências manuais.

**Decisão.** Foi atualizado o relatório. O binário original, o jogo e o ZIP v0.1.6 permanecem como amostras de referência. A recompilação é apenas uma evidência local. Não foi criada uma candidata v0.1.7.

### Comparação independente do fonte com o ZIP

**Launcher.** O PE x64 usa o subsistema GUI, linker identificado como 11.00 tanto na v0.1.5 como na v0.1.6, e as seções .text e .rsrc. A candidata declara versão 0.1.6.0. As quatro referências gerenciadas são System.Windows.Forms, System.Drawing, System e mscorlib, todas 4.0.0.0. A v0.1.6 mantém o JPEG e o ícone do projeto e acrescenta o recurso gerenciado runtime.sha256. Recompilamos Launcher.cs com o csc x64 do .NET Framework, o JPEG e o ICO do checkout e os cinco hashes recalculados sobre a extração. A comparação dos 325.120 bytes coincidiu após normalizar exclusivamente o timestamp PE e as duas representações do GUID do módulo. A validação de integridade, a versão e a retenção dos arquivos abertos durante a partida do processo explicam mudanças reais em relação à v0.1.5. A gravação dos dois INIs e a única chamada Process.Start estão descritas abaixo.

**DoomBootProbe.** O PE x64 usa o subsistema console, linker 14.44 em ambas as versões, e as seções .text, .rdata, .data, .pdata, .rsrc e .reloc. Nos dois EXEs da candidata, dumpbin mostra Dynamic Base e NX Compatible. Nenhuma seção tem simultaneamente escrita e execução. As versões declaradas nos cabeçalhos permitem comparar a família do linker. Elas não identificam sozinhas a revisão exata do compilador original.

O build Release foi refeito em diretório novo, com geração local do despacho, layout e 32 unidades de código traduzido. A primeira configuração limpa produziu o identificador interno de diagnóstico `0.1.0`; a candidata usava `dev`. Essa diferença alterou o posicionamento de strings e referências a elas. Reconfigurando o mesmo build com `-DGENESIS_GAME_VERSION=dev`, a seção .text coincidiu integralmente com a amostra distribuída. Apenas **9 de 963.072 bytes** diferiram no arquivo inteiro, em dois campos de timestamp e na string da hora de compilação. O rótulo de diagnóstico e a falta de versão PE do jogo limitam a identificação por propriedades do Windows. O SHA-256 identifica a amostra exata. A reprodução foi feita na máquina de build existente.

### Imports e comportamento do jogo

| Observação no PE ou no código | Interpretação técnica |
| --- | --- |
| Código compilado no jogo | Runner do framework, execução 68000 e despacho traduzido, áudio ymfm e Z80, troca de contexto e rbengine para estados. A geração ocorre na máquina de build. O jogo instalado carrega a ROM fornecida pelo jogador. |
| SDL2, CRT e APIs do Windows | SDL2 fornece janela, renderização, entradas e áudio. COMDLG32 importa GetOpenFileNameA. O único import observado de SHELL32 é CommandLineToArgvW. O seletor que usa popen está excluído do Windows pelo pré-processador. Não foram encontrados imports diretos de CreateProcess, ShellExecute, WinExec ou system no jogo. |
| WS2_32, importado por ordinal | O caminho de depuração em runner/cmd_server.c inicializa Winsock e um servidor TCP em INADDR_LOOPBACK. Em runner/main.c, a inicialização depende da presença de debug.ini ao lado do jogo. Esse arquivo está ausente do pacote. O build define GENESIS_HAS_RECOMP_NET=0. A análise é estática e não inclui captura de tráfego. |
| VirtualAlloc, VirtualProtect e GetProcAddress | Em runner/fiber_compat.c, a pilha da fibra é alocada como PAGE_READWRITE e a região de guarda recebe PAGE_NOACCESS. O código resolve GetProcessMitigationPolicy em kernel32 para consultar compatibilidade com shadow stacks. Isso explica o uso dessas APIs nesse caminho. |
| fopen, fwrite e remove | O runtime lê ROM e configurações e pode escrever saves, capturas e diagnósticos locais. No encerramento normal, descarrega SRAM pendente, fecha o depurador quando ativo, encerra áudio e controle, libera vídeo e chama SDL_Quit. O EXIT do roteiro automatizado tem seu próprio caminho de saída. |

### Decisão e regressão

Nesta investigação adicional, não foi encontrada divergência de lógica entre o fonte examinado e os EXEs no ZIP que justificasse outra correção. As DLLs, o runtime e o pacote foram preservados. Os resultados individuais identificam o launcher. Atribuir suas detecções a uma API, à ausência de assinatura ou a outro atributo requer evidência adicional dos fabricantes. A decisão técnica é manter o ZIP existente e usar as amostras originais na revisão pelos fornecedores.

Foram repetidos o build Windows Release, CTest da raiz (1/1) e CTest nativo (3/3). Na extração do ZIP, o launcher iniciou o jogo com a ROM USA, gravou as preferências e recusou a ROM incorreta. Uma cópia de teste com SDL2 adulterada provocou o aviso de integridade. A execução direta recusou uma ROM incorreta de mesmo tamanho com saída 1. O roteiro direto encerrou no quadro **4.402** com saída 0 e as quatro capturas coincidiram por SHA-256 com as do ensaio anterior. Áudio SDL simulado a 48 kHz e renderização por software estavam ativos.

O ZIP passou na conferência de CRC, membros e hashes do manifesto. A comparação do conteúdo descompactado com a sequência completa de bytes da ROM USA e a busca pelo prefixo de perfil Windows em ASCII e UTF-16LE não encontraram correspondências. Os dois EXEs e a SDL2 seguem sem assinatura, e os três CRTs mantêm assinatura Microsoft válida. Continuam pendentes as verificações físicas e de portabilidade descritas ao final do relatório. Nenhuma amostra foi transmitida nesta investigação.

## Histórico e auditoria inicial da v0.1.5

Em 20/11/2026, o SHA-256 do ZIP local v0.1.5 coincidiu com o informado no chamado, `dc56ca516347bc22aeb8aba7c996b9b51d8022fe767cfd7aa21926945c780772`. Todas as medições da v0.1.5 abaixo usam membros extraídos desse ZIP em diretório separado. As pastas de jogo usadas continham INIs e diagnósticos e seriam fontes inadequadas para reconstruir uma distribuição.

**Atribuição ainda aberta.** O chamado registra 2/66, Bkav Pro `W32.Malware.CA577264` e Elastic `Malicious (high Confidence)`. O hash do launcher do ZIP é exatamente o do histórico do README, que registra Arctic Wolf, Bkav Pro, CrowdStrike Falcon, Elastic e SecureAge. Isso vincula os alertas anteriores ao mesmo executável. O relatório agregado não informa qual membro motivou cada decisão. O acesso às páginas individuais do VirusTotal não retornou os resultados nesta auditoria. Usar os links abaixo para consulta manual. Nenhuma amostra foi enviada. [ZIP no VirusTotal](https://www.virustotal.com/gui/file/dc56ca516347bc22aeb8aba7c996b9b51d8022fe767cfd7aa21926945c780772)

## Inventário v0.1.5

| Arquivo | Versão no arquivo | Authenticode no Windows | SHA-256 e consulta individual |
| --- | --- | --- | --- |
| Launcher.exe | 0.0.0.0 | Sem assinatura | [`a6df110cd9e7e9d0a8dab5c7a68fd6549cfbdb10cd6274c8ddddd7a7ce68f898`](https://www.virustotal.com/gui/file/a6df110cd9e7e9d0a8dab5c7a68fd6549cfbdb10cd6274c8ddddd7a7ce68f898) |
| DoomBootProbe.exe | Sem versão PE | Sem assinatura | [`0a3696259c53e9cfd62fd0bcae81bc39c2b54f5bfb7026a9df28e6954c844b2f`](https://www.virustotal.com/gui/file/0a3696259c53e9cfd62fd0bcae81bc39c2b54f5bfb7026a9df28e6954c844b2f) |
| SDL2.dll | 2.28.5.0 | Sem assinatura | [`de23db1694a3c7a4a735e7ecd3d214b2023cc2267922c6c35d30c7fc7370d677`](https://www.virustotal.com/gui/file/de23db1694a3c7a4a735e7ecd3d214b2023cc2267922c6c35d30c7fc7370d677) |
| msvcp140.dll | 14.44.35211.0 | Microsoft Windows, válida | [`0f885b509a685d2bbfa652fed26b5fb31d88fbdab0a978c641d1c7b8aa460aa9`](https://www.virustotal.com/gui/file/0f885b509a685d2bbfa652fed26b5fb31d88fbdab0a978c641d1c7b8aa460aa9) |
| vcruntime140.dll | 14.44.35211.0 | Microsoft Windows, válida | [`d5e4d9a3e835fa679450145d6a7d94e36573a509317111904d9b3712c30d9066`](https://www.virustotal.com/gui/file/d5e4d9a3e835fa679450145d6a7d94e36573a509317111904d9b3712c30d9066) |
| vcruntime140_1.dll | 14.44.35211.0 | Microsoft Windows, válida | [`1f2d41c4aa5db0bc33ebf7b66d72943a817d7ce6cbe880502a9403823633093f`](https://www.virustotal.com/gui/file/1f2d41c4aa5db0bc33ebf7b66d72943a817d7ce6cbe880502a9403823633093f) |

## Código e executáveis

- **Launcher v0.1.5.** Recompilação do fonte de `07584c805bd7e2b4247aa27ba6229a035e6ed359`, com o csc do .NET Framework x64, o ícone e a capa do projeto. Ambos os PEs têm 323.584 bytes. A comparação integral localizou as diferenças no timestamp PE, no GUID do módulo e na representação textual desse GUID em `PrivateImplementationDetails`. O código executável e o restante do conteúdo coincidiram. A inspeção dos metadados identificou referências a mscorlib, System, System.Drawing e System.Windows.Forms e um recurso gerenciado, `cover.jpg`. Há também recursos Win32 de ícone e versão.
- **Inicialização.** Há uma chamada a Process.Start. O destino é o caminho absoluto de DoomBootProbe.exe na pasta do launcher. O processo recebe a ROM como argumento entre aspas, sem shell, e herda a pasta do pacote como diretório de trabalho. CreateNoWindow evita o console extra do executável nativo, preservando a janela SDL. As variáveis de ambiente específicas configuram pilha, Scale2x, máscara de cheats e interpretação quando aplicável. O fonte examinado não contém download, atualização automática, elevação, instalação de serviço ou execução de comandos obtidos da ROM.
- **Entrada e persistência.** O seletor, o argumento inicial e o caminho lembrado passam pela mesma validação de tamanho (2.097.152 bytes) e SHA-256 da revisão USA. As duas importações nativas do launcher são GetPrivateProfileString e WritePrivateProfileString de kernel32. As gravações ocorrem nos INIs locais, inclusive o caminho da ROM codificado em Base64. Esse caminho é recuperável e deve permanecer no computador do jogador. Erros de gravação são apresentados ao usuário. É necessária permissão de escrita na pasta extraída.
- **Falha corrigida.** O launcher verificava apenas a existência do EXE. Agora incorpora os hashes do jogo e das quatro DLLs no momento do empacotamento, verifica-os antes de salvar as preferências e mantém os arquivos verificados e a ROM abertos para leitura até Process.Start retornar. Um teste alterando um byte adicional na SDL2 provocou o aviso esperado. A proteção depende da integridade do próprio launcher. Abrir o jogo diretamente contorna essa verificação do launcher. O runtime valida tamanho e CRC32 da ROM no caminho direto.
- **Jogo nativo.** O build Release x64 usa o framework `f422ad52a04b9d253c0e9cbcdb3476aabd563ae8`, os patches do projeto e os núcleos fixados em config/framework_lock.json. CMake usa a ROM local validada para geração de código. A seção .text da candidata é idêntica à da v0.1.5. Só 20 bytes diferem nos dois PEs de 963.072 bytes. As diferenças ficam no cabeçalho e em .rdata. dumpbin confirmou SDL2, as três DLLs VC++, Universal CRT e APIs do Windows, inclusive WinSock. No fonte examinado, o servidor de depuração depende de debug.ini ao lado do EXE. O ZIP não contém esse arquivo. O jogo pode criar relatórios TOML durante o uso.
- **Reprodução do fonte.** Dois trechos de contexto no patch do launcher continham um travessão com codificação incorreta e impediam git apply em checkout limpo. A correção se limita ao contexto do patch. Os quatro patches do framework foram reaplicados em cópia limpa da revisão fixada. Os sete arquivos modificados coincidiram com os do checkout usado no build após normalização de finais de linha.

## Dependências e licenças

A SDL2 usada pelo jogo coincide byte a byte com runner/external/SDL2/SDL2-2.28.5/lib/x64/SDL2.dll no checkout local do framework. As três DLLs VC++ coincidem com os arquivos de VC/Redist/MSVC/14.44.35112/x64/Microsoft.VC143.CRT dos Build Tools 2022. A versão de arquivo das DLLs difere do nome da pasta do redistribuível. O script verifica os hashes individuais e exige assinatura válida da Microsoft nos três CRTs. Nenhuma DLL da candidata foi extraída de uma distribuição anterior. [Lista de redistribuição do Visual Studio 2022](https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution)

LICENSES.txt reúne o PolyForm Noncommercial do framework e do núcleo M68K, o inventário do upstream, SDL/zlib, ymfm/BSD, superzazu/MIT, clowncommon/ISC, minicoro e LuaCoco, xxHash, rbengine e os avisos das rotinas de áudio e cor. Os textos do framework e submódulos vêm dos objetos Git nas revisões fixadas. A redistribuição do CRT depende dos termos aplicáveis ao editor. A licença do framework não concede direitos sobre código ou arte do jogo.

## Candidata local

ZIP de teste `build/audit-v016/Doom-Troopers-PC-Recomp-v0.1.6-Windows-x64.zip`, 1.970.241 bytes. SHA-256 **ad9ce9a0bfe84419574dbb50ea5160626208195ca9a64672bfb23082d8ba4f74**. O arquivo .zip.sha256 identifica o ZIP, incluindo o manifesto. SHA256SUMS.txt contém os oito arquivos de carga, e seu próprio SHA-256 é `b52acb80dfbaf3cffa60a81a5e86b2d77095d6c41c5d876f998671c68a67dfa9`.

| Arquivo alterado ou novo | SHA-256 v0.1.6 |
| --- | --- |
| Launcher.exe (0.1.6.0, sem assinatura) | `9f9a9aecb9ead6266b433b9bdabedec89e6c8c662cbbf87fccfc6945e011b518` |
| DoomBootProbe.exe (sem versão PE, sem assinatura) | `a57db69a949fa2a7d41384724d4b2805790a9275ea9f47bdc074478f8ff5f4e3` |
| LEIA-ME.txt | `af5890c5e8cb3b6e1b17ad177690a2343104e2cd2dc001f1f4fba45ceb5a04b4` |
| LICENSES.txt | `9a969783df446f4d2e5b24e2c764d28ffb865c3d9cb5996f3dd8a168e4f9f701` |

As quatro DLLs mantêm os hashes e estados de assinatura da primeira tabela. Para repetir o empacotamento a partir de um build aprovado, executar no PowerShell com Python, Git, .NET Framework e os Build Tools instalados no computador de compilação. Obter o hash do jogo da etapa de build revisada. A opção -CrtDirectory aceita a pasta x64 verificada de outra instalação. Timestamps e identificadores dos compiladores variam entre builds.

```powershell
./launcher/package.ps1 -GameBuild build/patched/Release/DoomBootProbe.exe -GameSha256 a57db69a949fa2a7d41384724d4b2805790a9275ea9f47bdc074478f8ff5f4e3 -FrameworkRoot local/phase-framework -OutputDirectory build/repackage
```

O empacotamento usa staging novo por execução, lista fechada, validação PE x64, hashes por dependência, detecção da imagem USA integral, cabeçalhos de cartucho e caminhos de perfil Windows, seguida de releitura do ZIP. O fluxo de assinatura opcional assina o jogo antes de incorporar seu hash ao launcher. O pacote dispensa Jogar.bat e continua aceitando arrastar a ROM sobre Launcher.exe. No computador do jogador, o launcher usa o .NET Framework do Windows e o jogo usa as DLLs locais.

**Limite para distribuição.** O ZIP não contém a imagem integral da ROM, o binário intermediário, os fontes gerados, INIs, saves, logs, capturas de teste ou ferramentas de desenvolvimento. DoomBootProbe.exe contém tradução nativa de instruções do cartucho. O launcher também preserva a capa JPEG solicitada pelo desenho existente. Uma regra que exclua todo código traduzido é incompatível com a arquitetura desta candidata. O ZIP fica em build para verificação local até a definição do escopo de derivados.

## Testes executados

| Verificação | Resultado |
| --- | --- |
| Build Windows x64 Release em build/patched | Concluído, sem alteração de lógica no runtime. |
| CTest da raiz e de experiments/boot | 1/1 na raiz, incluindo sete testes Python. 3/3 nativos, incluindo Scale2x pixel a pixel, TRAP e RTE. |
| ROM USA e entradas inválidas | A ferramenta aceitou a ROM USA e rejeitou arquivo curto. O launcher iniciou o jogo com a ROM correta e rejeitou 2 MiB de zeros. A execução direta rejeitou esse arquivo com CRC32 8D89877E e saída 1. |
| Launcher e configurações | Janela WinForms e botão Play encontrados. O clique iniciou o processo extraído e gravou settings.ini e launcher.ini na cópia de teste. A SDL2 adulterada foi recusada antes da inicialização. |
| Jogo direto e entradas roteirizadas | Boot de 180 quadros concluído. O roteiro de gameplay gerou quatro capturas distintas e encerrou com código 0 no quadro 4.402. SDL usou renderizador software e saída simulada de áudio a 48 kHz. |
| Conteúdo e integridade | ZIP reaberto, CRCs válidos, lista exata de nove entradas sem duplicatas, oito hashes do manifesto e hash externo recalculados. Teste negativo recusou staging com arquivos extras. |
| Microsoft Defender local | MpCmdRun em modo de varredura personalizada, sem remediação, informou zero ameaças no diretório do ZIP final e na extração limpa. Esse resultado é específico dessas amostras e da verificação local. |

**Pendências manuais.** Testar em Windows x64 limpo sem ferramentas de desenvolvimento, com áudio físico, tela cheia, alternância do Scale2x e controle PS4. Conferir teclado, remapeamento e persistência após reabertura, save/load com L2/R2, menus PT/EN e os efeitos individuais dos cheats em gameplay. O teste automatizado usa entradas roteirizadas e dispositivos SDL simulados. O fluxo com certificado real e carimbo de tempo permanece sem teste. A reavaliação pelos fornecedores segue pendente.

## Solicitação de revisão pelos fornecedores

Priorizar Microsoft pelo alerta Wacatac.B!ml. No portal Security Intelligence, escolher Software developer e anexar a amostra original, o rótulo no VirusTotal, o estado do Defender local e a comparação com o fonte. Para Arctic Wolf, CrowdStrike e SecureAge, abrir solicitações separadas nos canais oficiais de suporte com o mesmo hash e os respectivos rótulos. Solicitar a cadeia de processos e os eventos que fundamentam os indicadores WMI e anti-debug. Nenhuma submissão foi feita nesta auditoria. [Microsoft Security Intelligence](https://www.microsoft.com/en-us/wdsi/filesubmission)

1. Consultar pelo hash do ZIP e de cada PE. Registrar data, produto, versão do mecanismo e nome exato da detecção. Manter os resultados da v0.1.5 separados dos da candidata.
2. Na solicitação, fornecer o SHA-256 específico, o relatório do VirusTotal, a revisão do fonte, o resultado da comparação binária e as etapas para reproduzir a detecção. Pedir identificação do membro ou comportamento que motivou o alerta. A ROM e os INIs pessoais ficam fora de qualquer submissão.
3. Para Elastic, usar o formulário oficial indicado pela equipe Endpoint Security, ou o suporte contratado para clientes. Para Bkav Pro, usar Support → Send support request to Bkav. Registrar os protocolos e a conclusão de cada fornecedor antes de atualizar o README. [Elastic](https://discuss.elastic.co/t/submitting-false-positives/232322) · [Bkav](https://www.bkav.com/en/support)
