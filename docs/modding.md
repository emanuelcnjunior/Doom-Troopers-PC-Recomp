# Mods de áudio na edição Integrated para Windows x64

Na aba **Jogo**, escolha **Aprimorado**, marque **Usar arquivos de som personalizados** e clique em **Jogar**. Para voltar ao áudio original, desmarque a opção ou selecione **Clássico**. A preferência fica em `launcher.ini`, na seção `[doom_launcher]`, chave `audio_mods=1`. Uma instalação nova começa com a opção desligada. A execução direta com a ROM usa as mesmas preferências.

## Pacotes com manifesto

Para instalar o exemplo, copie a pasta `mods/example-mod` do repositório para
`mods/example-mod` ao lado de `DoomBootProbe.exe`. Ela contém um tom de 440 Hz
sintetizado para o teste. O manifesto e o OGG podem ser usados como exemplo.
Crie `mods/enabled.txt` com uma linha contendo `example-mod`, selecione
**Aprimorado**, marque **Usar arquivos de som personalizados** e inicie o jogo.
O tom acompanha o evento da música do título. Para desativar só esse pacote,
remova sua linha e reinicie.

Cada pacote ocupa uma pasta própria. O nome da pasta na lista aceita letras
minúsculas ASCII, números, hífen e sublinhado (até 64 caracteres). A pasta
`audio` é reservada ao formato anterior. A lista aceita até 64 pacotes, um por
linha, e comentários iniciados por `#`. Use texto UTF-8. Copiar um novo pacote
para o diretório não o ativa. Para diagnóstico, no PowerShell:

~~~powershell
$env:DOOM_DISABLE_MODS = '1'
.\DoomBootProbe.exe
~~~

A variável também desativa o áudio instalado no formato anterior, inclusive ao
iniciar com uma ROM por argumento. Para voltar às preferências gravadas,
remova a variável com `Remove-Item Env:DOOM_DISABLE_MODS` antes de reabrir.

~~~json
{
  "schema": 1,
  "name": "Title tone",
  "version": "1.0",
  "author": "Doom Troopers PC Recomp",
  "compatibility": "doom-troopers-usa:11194414/audio-v1",
  "files": [
    {"type": "music", "id": "05", "path": "audio/music/05.ogg"}
  ]
}
~~~

O campo de compatibilidade identifica a revisão USA pelo CRC32 e a versão do
formato de áudio. O manifesto UTF-8 pode ter até 64 KiB e 256 recursos.
Use ID hexadecimal de dois dígitos maiúsculos. Os caminhos aceitos são
`audio/music/ID.ext` e `audio/sfx/ID.ext`, com extensão minúscula `wav`,
`ogg` ou `mp3`. O tipo deve corresponder à pasta. O carregador valida todos
os itens do manifesto antes de registrar o pacote. Rejeita campos ou tipos
desconhecidos, caminhos externos e componentes marcados pelo Windows como
pontos de nova análise, incluindo junctions e links simbólicos. Somente os
arquivos de áudio declarados seguem aos decodificadores locais. Sprites ainda
dependem do mapeamento descrito em [gráficos Enhanced](enhanced-graphics.md).

Se dois pacotes ativos, ou um pacote e a pasta de áudio anterior, definirem o
mesmo ID, essa sequência usa o driver do cartucho. Outros IDs continuam
disponíveis. Um pacote incompatível ou malformado é ignorado com diagnóstico
`[doom-mods]` em stderr. O controle de volume e loop por ID continua em
`mods/audio/audio.ini`, inclusive para recursos de pacotes.

## Arquivos

Crie as pastas ao lado de `DoomBootProbe.exe`. Use dois dígitos hexadecimais para o ID da sequência do cartucho USA. O exemplo substitui a música do título e um evento da primeira fase:

~~~text
mods/
  audio/
    audio.ini
    music/
      05.ogg
    sfx/
      19.wav
~~~

O jogo aceita **OGG Vorbis**, **MP3** e **WAV PCM de 16 bits**, mono ou estéreo, de 8 a 96 kHz. Instale apenas um arquivo por ID entre as duas pastas e os três formatos. IDs duplicados, arquivos ausentes, inválidos, acima do limite ou ainda em carregamento deixam o comando seguir para o driver original. Reinicie o jogo depois de trocar arquivos. O carregador procura arquivos locais de nome fixo em uma thread auxiliar. Os limites são 32 MiB por arquivo comprimido, 64 MiB para um WAV ou PCM decodificado por entrada e 128 MiB de amostras no banco. Um evento ocorrido durante a carga toca sua versão original até a próxima chamada.

Em `audio.ini`, cada seção usa o mesmo ID hexadecimal. O arquivo é opcional:

~~~ini
[05]
volume=80
loop=1
duration_ms=60000

[19]
volume=100
loop=0
~~~

O volume vai de 0 a 100 e o padrão é 100. `duration_ms` limita a duração do trecho começando no início do arquivo; zero ou a ausência da chave usa o arquivo inteiro. Com `loop=1`, esse trecho se repete até um comando de parada ou substituição. Música inicia com loop ativado, efeitos com loop desligado. O mixer tem uma voz de música e até 16 vozes de efeito. Com todas as vozes de efeito ocupadas, o próximo efeito usa o som do cartucho. O volume mestre continua atuando na saída dos alto-falantes.

## Eventos verificados

| ID | Observação no replay USA |
|---|---|
| `05` | Música do título. Substituição verificada na captura estéreo do mixer. |
| `00` | Início de sequência na entrada da primeira fase. O teste preserva esse comando no driver enquanto substitui `19`. |
| `19` | Evento repetido durante o trecho de movimentação e combate. O teste usou uma amostra curta de efeito; a ação exata ainda precisa de isolamento. |

Para identificar outros IDs, defina `DOOM_AUDIO_TRACE=1` e capture stderr na execução por argumento. O registro informa ID e origem (`file` ou `driver`). A associação é ao byte de sequência da revisão USA, não ao quadro de vídeo, tecla ou nome de fase.

O 68000 solicita o barramento do Z80 em `0xA11100`, escreve na fila circular em `0xA01B40` e avança o cursor em `0xA00036`. Na liberação do barramento, o adaptador reconhece o pacote completo de início `FF 10 ID`, as paradas individuais `FF 12 ID` e a limpeza `FF 16`. Somente quando uma voz externa pode começar, o adaptador retira aquele início da fila. Os demais comandos seguem para o Z80, que aciona o YM2612 e o PSG. O mixer adiciona o PCM externo no relógio de áudio da emulação, antes da captura WAV e da saída SDL. A sincronização inicial tem a granularidade do bloco de áudio do quadro.

## Estados salvos e diagnóstico

Estados anteriores compatíveis com GROWNS3 continuam legíveis. Quando há voz personalizada ativa, o save recebe um complemento com ID, voz e posição da amostra. Para retomar no mesmo ponto, mantenha o mesmo arquivo instalado. Ao carregar sem o arquivo, o jogo solicita ao driver a sequência original desde o início. Um save com esse complemento requer a nova compilação. Saves gravados sem voz personalizada ativa mantêm o formato anterior.

Com a ROM local validada, configure o build Integrated e execute `ctest --test-dir build/integrated -C Release --output-on-failure`. O teste `audio_mods` sintetiza arquivos temporários e compara a saída do mixer. FFmpeg é usado somente no teste para gerar os exemplos OGG e MP3. O jogo incorpora os decodificadores stb_vorbis (revisão `6b5799bae8a1fe3792691ef4b72ae26434c3e799`) e dr_mp3 (`e381981afa52800b9a3864df548353ff67252fbc`). As licenças acompanham o código em `experiments/boot/audio_codecs/` e entram em `LICENSES.txt` na preparação do pacote.

Para atualizar um checkout do framework que já contém os patches anteriores, aplique `git -C local/segagenesisrecomp apply ../../patches/framework-doom-audio.patch`. Checkouts novos preparados por `tools/setup_framework.py` já recebem esse patch.

Na Fase 8, os testes Windows de pacotes, codecs e recuperação de estado passaram no build identificado no [checklist de validação](enhanced-roadmap.md#fase-8--checklist-do-build-de-25112026). A compatibilidade do carregador com Proton aguarda execução externa.
