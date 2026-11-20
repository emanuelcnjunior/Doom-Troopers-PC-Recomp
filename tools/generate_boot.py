"""Local-only experiment: validate identity before invoking the pinned translator."""
import argparse
from pathlib import Path
import subprocess
from rom_tool import validate
from cheat_tool import parse, select

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--rom', required=True, type=Path)
parser.add_argument('--compiler', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
parser.add_argument('--cheat-file', type=Path)
parser.add_argument('--cheats', default='')
args = parser.parse_args()
rom = bytearray(validate(args.rom))
root = Path(__file__).resolve().parents[1]
output = args.output.resolve()
if not any(output.is_relative_to(root / directory) for directory in ('build', 'local')):
    parser.error('Output must stay under ignored build/ or local/.')
output.mkdir(parents=True, exist_ok=True)
catalog = parse(args.cheat_file) if args.cheat_file else []
selected = select(catalog, args.cheats)
patched_rom = output / 'doom_cheat_input.bin'
for cheat in selected:
    for patch in cheat.patches:
        if patch.kind == 'rom':
            rom[patch.address:patch.address + 2] = patch.value.to_bytes(2, 'big')
patched_rom.write_bytes(rom)

header = output / 'doom_cheats.h'
source = output / 'doom_cheats.c'
header.write_text(
    '#pragma once\n'
    'void doom_cheats_apply_rom(void);\n'
    'void doom_cheats_apply_ram(void);\n'
    'void doom_launcher_cheats(unsigned mask, int rom);\n'
    'const char *doom_cheats_summary(void);\n', encoding='utf-8')
rom_lines = []
ram_lines = []
for cheat in selected:
    for patch in cheat.patches:
        target = rom_lines if patch.kind == 'rom' else ram_lines
        array = 'g_rom' if patch.kind == 'rom' else 'g_ram'
        offset = patch.address if patch.kind == 'rom' else patch.address - 0xFF0000
        target.extend([
            f'    {array}[0x{offset:06X}u] = 0x{patch.value >> 8:02X}u;',
            f'    {array}[0x{offset + 1:06X}u] = 0x{patch.value & 0xFF:02X}u;',
        ])
launcher_lines = []
for cheat in catalog:
    for patch in cheat.patches:
        is_rom = patch.kind == 'rom'
        offset = patch.address if is_rom else patch.address - 0xFF0000
        limit = len(rom) if is_rom else 65536
        if not 0 <= offset < limit - 1:
            raise ValueError(f"Patch outside mapped memory: {cheat.description}")
        array = 'g_rom' if is_rom else 'g_ram'
        launcher_lines.extend([
            f'    if (rom == {int(is_rom)} && (mask & (1u << {cheat.index}))) {{',
            f'        {array}[0x{offset:X}] = {patch.value >> 8};',
            f'        {array}[0x{offset + 1:X}] = {patch.value & 255};',
            '    }',
        ])
summary = ', '.join(f'{cheat.index}:{cheat.description}' for cheat in selected) or 'none'
escaped_summary = summary.replace('\\', '\\\\').replace('"', '\\"')
source.write_text(
    '#include "doom_cheats.h"\n'
    '#include <stdint.h>\n'
    'extern uint8_t g_rom[0x400000];\n'
    'extern uint8_t g_ram[0x10000];\n'
    'void doom_cheats_apply_rom(void) {\n' + '\n'.join(rom_lines) + '\n}\n'
    'void doom_cheats_apply_ram(void) {\n' + '\n'.join(ram_lines) + '\n}\n'
    'void doom_launcher_cheats(unsigned mask, int rom) {\n' + '\n'.join(launcher_lines) + '\n}\n'
    f'const char *doom_cheats_summary(void) {{ return "{escaped_summary}"; }}\n',
    encoding='utf-8')

subprocess.run([str(args.compiler.resolve()), str(patched_rom), '--game',
                str(root / 'config/doom_boot.toml'), '--output-dir', str(output)], check=True)
print(f'Cheat selection: {summary}')
