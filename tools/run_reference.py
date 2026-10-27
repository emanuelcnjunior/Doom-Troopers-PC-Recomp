"""Headless BlastEm debugger observation; evidence never leaves local/."""
import argparse
import os
from pathlib import Path
import subprocess
from rom_tool import validate

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--rom', required=True, type=Path)
parser.add_argument('--exe', required=True, type=Path)
args = parser.parse_args()
validate(args.rom)
root = Path(__file__).resolve().parents[1]
run = root / 'local/reference'
run.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
commands = ('print/x pc sr a7\nadvance 0x054f78\nprint/x pc sr a7\n'
            'step\nprint/x pc sr a7\nframes 120\nprint/x pc sr a7\nquit\n')
with (run / 'stdout.log').open('w') as out, (run / 'stderr.log').open('w') as err:
    try:
        result = subprocess.run([str(args.exe.resolve()), '-t', '-b', '180', '-d',
                                 '-r', 'U', str(args.rom.resolve())],
                                input=commands, text=True, cwd=run, env=env,
                                stdout=out, stderr=err, timeout=30)
        print(f'BlastEm exit={result.returncode}; evidence={run}')
    except subprocess.TimeoutExpired:
        print(f'BlastEm WATCHDOG after 30s; evidence={run}')
        raise SystemExit(124)
if result.returncode:
    raise SystemExit(1)
