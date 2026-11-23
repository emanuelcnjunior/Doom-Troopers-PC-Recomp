"""Run the game with transition tracing and create a ROM/save-free ZIP."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
from datetime import datetime, timezone
import zipfile

from rom_tool import validate


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--rom", required=True, type=Path)
parser.add_argument("--exe", required=True, type=Path)
parser.add_argument("--output", type=Path)
parser.add_argument("--runner", type=Path,
                    help="Optional compatibility runner, for example Proton's proton script")
parser.add_argument("game_args", nargs="*")
args = parser.parse_args()

validate(args.rom)
exe = args.exe.resolve(strict=True)
runner = args.runner.resolve(strict=True) if args.runner else None
stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
root = Path(__file__).resolve().parents[1]
run_dir = root / "local" / f"level-transition-{stamp}"
run_dir.mkdir(parents=True, exist_ok=False)

env = os.environ.copy()
env["DOOM_TRANSITION_TRACE"] = "1"
env["GENESIS_STRICT_JSR_STACK"] = "1"
if runner:
    command = [str(runner), "run", str(exe), str(args.rom.resolve()),
               "--no-launcher", *args.game_args]
else:
    command = [str(exe), str(args.rom.resolve()), "--no-launcher", *args.game_args]
print("Play through level 1, continue into level 2, then close the game.")
with (run_dir / "stdout.log").open("w", encoding="utf-8") as stdout, \
     (run_dir / "stderr.log").open("w", encoding="utf-8") as stderr:
    result = subprocess.run(command, cwd=run_dir, env=env, stdout=stdout, stderr=stderr)

metadata = {
    "created_utc": stamp,
    "exit_code": result.returncode,
    "platform": platform.platform(),
    "python": platform.python_version(),
    "executable_sha256": sha256(exe),
    "rom_sha256": sha256(args.rom),
    "trace": True,
    "strict_jsr_stack": True,
    "game_args": args.game_args,
    "privacy": "Archive contains logs and metadata only; no ROM or save files.",
}
if runner:
    metadata["compatibility_runner"] = runner.name
    try:
        version = subprocess.run([str(runner), "--version"], env=env,
                                 capture_output=True, text=True, timeout=15)
        metadata["compatibility_runner_version"] = (
            version.stdout.strip() or version.stderr.strip() or "unreported")
    except (OSError, subprocess.SubprocessError) as error:
        metadata["compatibility_runner_version"] = f"unavailable: {type(error).__name__}"
(run_dir / "metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
archive = (args.output or run_dir.with_suffix(".zip")).resolve()
archive.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
    for name in ("metadata.json", "stdout.log", "stderr.log"):
        bundle.write(run_dir / name, name)
print(f"Diagnostic archive: {archive}")
raise SystemExit(result.returncode)
