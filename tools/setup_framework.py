"""Fetch the audited framework into ignored local/, without game disassemblies."""
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
lock = json.loads((root / 'config/framework_lock.json').read_text())
checkout = root / 'local/segagenesisrecomp'
def git(*args):
    return subprocess.check_output(['git', *args], text=True).strip()
if not checkout.exists():
    git('clone', '--no-recurse-submodules', lock['repository'], str(checkout))
if git('-C', str(checkout), 'status', '--porcelain'):
    raise SystemExit('Framework checkout has changes; refusing to switch revisions.')
git('-C', str(checkout), 'checkout', '--detach', lock['commit'])
git('-C', str(checkout), 'submodule', 'update', '--init', *lock['submodules'])
for path, revision in lock['submodules'].items():
    if git('-C', str(checkout / path), 'rev-parse', 'HEAD') != revision:
        raise SystemExit(f'Submodule pin mismatch: {path}')

patches = [
    (checkout, root / 'patches/framework-trap-runtime.patch'),
    (checkout, root / 'patches/framework-ym2612-timer-a.patch'),
    (checkout, root / 'patches/framework-launcher-runtime.patch'),
    (checkout / 'external/m68k-recomp-core', root / 'patches/m68k-core-doom-trap.patch'),
]
for target, patch in patches:
    reverse = subprocess.run(['git', '-C', str(target), 'apply', '--reverse', '--check', str(patch)],
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if reverse.returncode == 0:
        continue
    subprocess.run(['git', '-C', str(target), 'apply', '--check', str(patch)], check=True)
    subprocess.run(['git', '-C', str(target), 'apply', str(patch)], check=True)
print('Framework pins and Doom Troopers experiment patches verified.')
