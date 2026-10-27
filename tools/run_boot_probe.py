"""Run an experimental build with a watchdog; all observations remain local."""
import argparse
import os
from pathlib import Path
import subprocess
from rom_tool import validate

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--rom', type=Path, required=True)
parser.add_argument('--exe', type=Path, required=True)
parser.add_argument('--frames', type=int, default=120)
parser.add_argument('--seconds', type=int, default=30)
parser.add_argument('--interpret', action='store_true')
args = parser.parse_args()
validate(args.rom)
root = Path(__file__).resolve().parents[1]
mode = 'interpreter' if args.interpret else 'translated'
run = root / 'local' / ('boot-' + mode)
run.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
           SDL_RENDER_DRIVER='software', GENESIS_RUN_DONE='1',
           GENESIS_FORCE_INTERP='1' if args.interpret else '0',
           GENESIS_SCREENSHOT_AT_EXIT=str(run / 'last.png'))
command = [str(args.exe.resolve()), str(args.rom.resolve()), '--max-frames',
           str(args.frames), '--hash-frames', '--turbo', '--no-launcher']
with (run / 'stdout.log').open('w') as out, (run / 'stderr.log').open('w') as err:
    try:
        result = subprocess.run(command, cwd=run, env=env, stdout=out, stderr=err,
                                timeout=args.seconds)
        print(f'{mode}: exit={result.returncode}; evidence={run}')
    except subprocess.TimeoutExpired:
        print(f'{mode}: WATCHDOG after {args.seconds}s; evidence={run}')
        raise SystemExit(124)
logs = ''.join((run / name).read_text(errors='replace') for name in ('stdout.log', 'stderr.log'))
if result.returncode or 'HALT' in logs or '[ILLEGAL]' in logs or '[DONE]' not in logs:
    print('FAILED: CPU halt, abort or incomplete session; host frame count is not boot success.')
    raise SystemExit(1)
print('Session completed; correctness still requires independent comparison.')
