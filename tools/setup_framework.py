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
print('Framework and infrastructure pins verified. No game disassembly requested.')
