"""Identity gate and metadata-only Genesis inspection. No ROM output files."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

IDENTITY = json.loads((Path(__file__).resolve().parents[1] / 'config/rom_identity.json').read_text())


def validate(path, identity=IDENTITY):
    with Path(path).open('rb') as stream:
        data = stream.read(identity['size'] + 1)
    if len(data) != identity['size']:
        raise ValueError(f"Wrong size: expected {identity['size']} bytes; got {len(data)} (read capped at expected+1). No conversion is performed.")
    digest = hashlib.sha256(data).hexdigest()
    if digest != identity['sha256']:
        raise ValueError(f"SHA-256 mismatch: {digest}. Expected {identity['sha256']}. Modified, swapped, headered or other-region ROM rejected.")
    return data


def inspect(data):
    def text_at(start, end):
        return data[start:end].decode('ascii', errors='replace').strip()
    def u32(offset):
        return struct.unpack_from('>I', data, offset)[0]
    return {
        'console': text_at(0x100, 0x110),
        'domestic_title': text_at(0x120, 0x150),
        'overseas_title': text_at(0x150, 0x180),
        'product': text_at(0x180, 0x18E),
        'crc32': f'{zlib.crc32(data):08x}',
        'initial_sp': f'0x{u32(0):08X}',
        'reset_pc': f'0x{u32(4):08X}',
        'h_interrupt_vector': f'0x{u32(0x70):08X}',
        'v_interrupt_vector': f'0x{u32(0x78):08X}',
        'declared_rom': [f'0x{u32(p):08X}' for p in (0x1A0, 0x1A4)],
        'declared_ram': [f'0x{u32(p):08X}' for p in (0x1A8, 0x1AC)],
        'sram_marker_present': data[0x1B0:0x1B2] == b'RA',
        'region': text_at(0x1F0, 0x200),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['validate', 'inspect'])
    parser.add_argument('rom', type=Path)
    args = parser.parse_args()
    try:
        data = validate(args.rom)
    except (OSError, ValueError) as error:
        print(f'ROM rejected: {error}', file=sys.stderr)
        return 1
    print('ROM identity verified (size + SHA-256).')
    if args.command == 'inspect':
        print(json.dumps(inspect(data), indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
