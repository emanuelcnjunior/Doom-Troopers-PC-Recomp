"""Compare the Classic/Enhanced guest framebuffers along the stage-one input route."""
from hashlib import sha256
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time


def check(game, rom):
    route = Path(__file__).with_name("doom_gameplay_probe.input").resolve()
    captures = [s.split()[1] for s in route.read_text().splitlines() if s.startswith("SCREENSHOT ")]
    reference = None
    with tempfile.TemporaryDirectory() as tmp:
        for profile in ("classic", "enhanced"):
            folder = Path(tmp) / profile
            folder.mkdir()
            exe = shutil.copy2(game, folder)
            shutil.copy2(game.with_name("SDL2.dll"), folder)
            (folder / "launcher.ini").write_text(f"[doom_launcher]\nprofile={profile}\nenhanced_scale2x=1\n")
            env = dict(os.environ, SDL_AUDIODRIVER="dummy", SDL_VIDEODRIVER="dummy",
                       SDL_RENDER_DRIVER="software", GENESIS_STRICT_JSR_STACK="1",
                       DOOM_CHEAT_MASK="0", DOOM_DISABLE_MODS="1")
            env.pop("DOOM_SCALE2X", None)
            start = time.perf_counter()
            run = subprocess.run([exe, rom, "--turbo", "--input-script", route, "--max-frames", "4600"],
                                 cwd=folder, env=env, capture_output=True, timeout=120, check=True)
            elapsed = time.perf_counter() - start
            assert b"EXIT 0 at frame 4402" in run.stderr, run.stderr[-1500:]
            images = [sha256((folder / name).read_bytes()).hexdigest() for name in captures]
            assert len(set(images)) == len(captures)
            if reference:
                assert images == reference, "Guest screenshot differs between presentation profiles"
            reference = images
            print(f"{profile}: 4402 frames, {elapsed:.2f}s wall, {4402/elapsed:.1f} frames/s including startup")


if __name__ == "__main__":
    check(*(Path(arg).resolve() for arg in sys.argv[1:]))
