# Cheats da revisão USA

O catálogo em `config/doom_troopers_usa.cht` veio do arquivo fornecido para a
ROM USA validada. Todos os 30 itens permanecem desativados por padrão.

- 23 códigos Game Genie são decodificados em endereço/word de ROM e aplicados
  antes da geração de C. A mesma alteração é aplicada ao shadow de ROM para o
  fallback interpretado.
- 7 códigos `FFxxxx` são freezes de RAM e são reaplicados no hook pré-quadro.
- Cheats incompatíveis que escrevem valores diferentes no mesmo endereço são
  rejeitados no configure/build.

Uma variante é selecionada no configure por índices separados por vírgula:

```powershell
cmake -S experiments/boot -B build/doom-cheats `
  -DDOOM_ROM="C:/caminho/Doom Troopers (USA).md" `
  -DDOOM_CHEATS="8,13,15,25"
```

O arquivo original nunca é modificado. A cópia temporária usada pelo tradutor
fica em `build/*/generated/doom_cheat_input.bin`, ignorada pelo Git. “Código
decodificado e incorporado” ainda não significa “cheat validado em gameplay”:
a POC ainda não produz quadro correto, portanto cada opção deverá ser testada
quando o boot alcançar menus e jogo.
