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
                    help="Optional Wine binary or Proton script")
parser.add_argument("--runner-kind", choices=("wine", "proton"), default="proton")
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
    if os.name == "nt":
        parser.error("Use --runner from the Linux host")
    # The Windows executable opens the ROM with a Windows file API.
    rom_arg = "Z:" + str(args.rom.resolve()).replace("/", "\\")
    command = [str(runner)]
    if args.runner_kind == "proton":
        command.append("run")
    command.extend([str(exe), rom_arg, "--no-launcher", *args.game_args])
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
    "extra_arguments_used": bool(args.game_args),
    "privacy": "Archive contains logs and metadata only; no ROM or save files.",
}
if runner:
    metadata["compatibility_runner"] = runner.name
    metadata["runner_kind"] = args.runner_kind
    if args.runner_kind == "proton":
        version_file = runner.parent / "version"
        metadata["compatibility_runner_version"] = (
            version_file.read_text(errors="replace")[:160].strip() if version_file.is_file() else "unreported")
    else:
        try:
            version = subprocess.run([str(runner), "--version"], capture_output=True, text=True, timeout=15)
            metadata["compatibility_runner_version"] = version.stdout.strip()[:160] or "unreported"
        except (OSError, subprocess.SubprocessError):
            metadata["compatibility_runner_version"] = "unreported"
(run_dir / "metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
archive = (args.output or run_dir.with_suffix(".zip")).resolve()
archive.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
    bundle.write(run_dir / "metadata.json", "metadata.json")
    # Keep full logs locally. Driver output, arbitrary arguments and save paths
    # can identify a user; share only recognized runtime diagnostics.
    prefixes = ("[DOOM_TRANSITION]", "[DOOM_SCHEDULER]", "[TRAP]", "[ILLEGAL]",
                "[HALT]", "[VIDEO] renderer", "[DOOM_VIDEO]", "[pacer]", "JSR stack mismatch")
    for name in ("stdout.log", "stderr.log"):
        with (run_dir / name).open(encoding="utf-8", errors="replace") as source:
            lines = (line for line in source if line.startswith(prefixes)
                     and "/" not in line and "\\" not in line)
            bundle.writestr(name, "".join(lines))
print(f"Diagnostic archive: {archive}")
raise SystemExit(result.returncode)
