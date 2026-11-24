"""Verify that the game's title music reaches the stereo mixer."""
from array import array
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import wave


def check(executable, rom):
    with tempfile.TemporaryDirectory() as directory:
        # Settings and diagnostic files are resolved relative to the executable.
        folder = Path(directory)
        game = Path(shutil.copy2(executable, folder))
        shutil.copy2(executable.with_name("SDL2.dll"), folder)
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software",
                   SDL_AUDIODRIVER="dummy", GENESIS_FORCE_INTERP="0", DOOM_CHEAT_MASK="0")
        subprocess.run([game, rom, "--no-launcher", "--turbo", "--max-frames", "1400",
                        "--wav", "title.wav"], cwd=folder, env=env, check=True, timeout=90)
        with wave.open(str(folder / "title.wav"), "rb") as capture:
            if capture.getnchannels() != 2 or capture.getsampwidth() != 2:
                raise AssertionError("Expected stereo 16-bit PCM")
            # The intro logos are quiet. Check three seconds during title music.
            rate = capture.getframerate()
            capture.setpos(19 * rate)
            for second in range(19, 22):
                samples = array("h", capture.readframes(rate))
                if len(samples) != rate * 2:
                    raise AssertionError("Incomplete title audio capture")
                for channel in (samples[::2], samples[1::2]):
                    if max(channel) - min(channel) < 512:
                        raise AssertionError(f"Silent title audio at {second} seconds")


if __name__ == "__main__":
    check(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve())
