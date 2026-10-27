"""Local-only experiment: validate identity before invoking the pinned translator."""
import argparse
from pathlib import Path
import subprocess
from rom_tool import validate

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--rom', required=True, type=Path)
parser.add_argument('--compiler', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
validate(args.rom)
root = Path(__file__).resolve().parents[1]
output = args.output.resolve()
if not any(output.is_relative_to(root / directory) for directory in ('build', 'local')):
    parser.error('Output must stay under ignored build/ or local/.')
output.mkdir(parents=True, exist_ok=True)
subprocess.run([str(args.compiler.resolve()), str(args.rom.resolve()), '--game',
                str(root / 'config/doom_boot.toml'), '--output-dir', str(output)], check=True)
