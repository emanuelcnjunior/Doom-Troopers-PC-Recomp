"""Parse and validate RetroArch-style Genesis cheat files."""
import argparse
from dataclasses import dataclass
from pathlib import Path
import re

from rom_tool import validate

GENIE_ALPHABET = "ABCDEFGHJKLMNPRSTVWXYZ0123456789"
ASSIGNMENT = re.compile(r'^([a-zA-Z0-9_]+)\s*=\s*(?:"([^"]*)"|(\S+))\s*$')


@dataclass(frozen=True)
class Patch:
    kind: str
    address: int
    value: int
    width: int = 2


@dataclass(frozen=True)
class Cheat:
    index: int
    description: str
    code: str
    enabled: bool
    patches: tuple[Patch, ...]


def decode_game_genie(code):
    code = code.upper()
    if len(code) != 9 or code[4] != '-':
        raise ValueError(f"Invalid Genesis Game Genie code: {code}")
    chars = code[:4] + code[5:]
    try:
        n = [GENIE_ALPHABET.index(char) for char in chars]
    except ValueError as error:
        raise ValueError(f"Invalid Genesis Game Genie character in: {code}") from error
    data = (n[0] << 3) | (n[1] >> 2)
    address = (n[1] & 3) << 14
    address |= n[2] << 9
    address |= (n[3] & 0x0F) << 20 | (n[3] >> 4) << 8
    address |= (n[4] >> 1) << 16
    data |= (n[4] & 1) << 12
    data |= (n[5] & 1) << 15 | (n[5] >> 1) << 8
    address |= (n[6] & 7) << 5
    data |= (n[6] >> 3) << 13
    address |= n[7]
    return Patch("rom", address, data)


def decode_part(code):
    code = code.upper()
    if '-' in code:
        return decode_game_genie(code)
    normalized = code.replace(':', '')
    if not re.fullmatch(r'[0-9A-F]{10}', normalized):
        raise ValueError(f"Invalid raw Genesis cheat: {code}")
    address = int(normalized[:6], 16)
    value = int(normalized[6:], 16)
    return Patch("ram" if address >= 0xFF0000 else "rom", address, value)


def parse(path):
    values = {}
    for number, raw_line in enumerate(Path(path).read_text(encoding='utf-8-sig').splitlines(), 1):
        line = raw_line.strip()
        if not line or line.startswith('#'):
            continue
        match = ASSIGNMENT.fullmatch(line)
        if not match:
            raise ValueError(f"Invalid cheat file line {number}: {raw_line}")
        values[match.group(1)] = match.group(2) if match.group(2) is not None else match.group(3)
    try:
        count = int(values['cheats'])
    except (KeyError, ValueError) as error:
        raise ValueError("Missing or invalid cheats count") from error
    cheats = []
    for index in range(count):
        prefix = f'cheat{index}_'
        try:
            description = values[prefix + 'desc']
            code = values[prefix + 'code']
            enabled_text = values[prefix + 'enable'].lower()
        except KeyError as error:
            raise ValueError(f"Incomplete cheat {index}") from error
        if enabled_text not in ('true', 'false'):
            raise ValueError(f"Invalid enable flag for cheat {index}")
        patches = tuple(decode_part(part) for part in code.split('+'))
        cheats.append(Cheat(index, description, code, enabled_text == 'true', patches))
    return cheats


def select(cheats, expression):
    if not expression:
        return []
    requested = []
    for item in expression.replace(';', ',').split(','):
        try:
            index = int(item.strip())
        except ValueError as error:
            raise ValueError(f"Invalid cheat index: {item}") from error
        if index < 0 or index >= len(cheats):
            raise ValueError(f"Cheat index out of range: {index}")
        if index not in requested:
            requested.append(index)
    selected = [cheats[index] for index in requested]
    occupied = {}
    for cheat in selected:
        for patch in cheat.patches:
            key = (patch.kind, patch.address)
            previous = occupied.get(key)
            if previous is not None and previous.value != patch.value:
                raise ValueError(
                    f"Conflicting cheats at 0x{patch.address:06X}: "
                    f"{previous.value:04X} vs {patch.value:04X}")
            occupied[key] = patch
    return selected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cheat_file', type=Path)
    parser.add_argument('--rom', required=True, type=Path)
    args = parser.parse_args()
    rom = validate(args.rom)
    cheats = parse(args.cheat_file)
    for cheat in cheats:
        print(f"{cheat.index:02d} {cheat.description}")
        for patch in cheat.patches:
            if patch.kind == 'rom':
                if patch.address + 1 >= len(rom):
                    raise ValueError(f"ROM patch out of range: 0x{patch.address:06X}")
                original = int.from_bytes(rom[patch.address:patch.address + 2], 'big')
                print(f"   ROM 0x{patch.address:06X}: 0x{original:04X} -> 0x{patch.value:04X}")
            else:
                print(f"   RAM 0x{patch.address:06X} := 0x{patch.value:04X}")
    print(f"Validated {len(cheats)} cheats; all are disabled by default.")


if __name__ == '__main__':
    main()
