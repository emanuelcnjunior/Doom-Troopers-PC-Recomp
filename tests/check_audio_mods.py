"""Exercise the USA sequence FIFO and mixer with locally synthesized audio."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import wave
from array import array


def check(game, rom):
    with tempfile.TemporaryDirectory() as temp:
        folder = Path(temp)
        exe = Path(shutil.copy2(game, folder))
        shutil.copy2(game.with_name("SDL2.dll"), folder)
        music = folder / "mods/audio/music"
        music.mkdir(parents=True)
        env = dict(os.environ, SDL_AUDIODRIVER="dummy", SDL_VIDEODRIVER="dummy",
                   SDL_RENDER_DRIVER="software", DOOM_AUDIO_TRACE="1")
        ini = folder / "launcher.ini"

        def run(profile="enhanced", active=True, script=None):
            ini.write_text(f"[doom_launcher]\nprofile={profile}\naudio_mods={int(active)}\n")
            args = [exe, rom, "--turbo", "--max-frames", "1400", "--wav", "mix.wav"]
            if script:
                (folder / "test.input").write_text(script)
                args.extend(["--input-script", "test.input"])
            result = subprocess.run(args, cwd=folder, env=env, capture_output=True, timeout=90, check=True)
            with wave.open(str(folder / "mix.wav")) as f:
                f.setpos(21 * f.getframerate())
                pcm = f.readframes(f.getframerate() // 4)
            return result.stderr, pcm

        logs, original = run(active=False)
        assert b"sequence=05 source=driver" in logs
        wav = music / "05.wav"
        wav.write_bytes(b"RIFF" + bytes(100))
        logs, invalid = run()
        assert invalid == original and b"sequence=05 source=driver" in logs
        for extension in ("ogg", "mp3"):
            invalid_file = wav.with_suffix("." + extension)
            wav.rename(invalid_file)
            logs, pcm = run()
            assert pcm == original and b"sequence=05 source=driver" in logs
            invalid_file.rename(wav)
        with wave.open(str(wav), "wb") as f:
            f.setparams((2, 2, 16000, 0, "NONE", ""))
            f.writeframes(array("h", [4000, -4000] * 1600).tobytes())
        logs, classic = run("classic")
        assert classic == original and b"sequence=05 source=driver" in logs
        (music.parent / "audio.ini").write_text("[05]\nvolume=50\nloop=1\nduration_ms=50\n")
        logs, replaced = run(script="WAIT 1150\nSAVE_STATE custom.state\nWAIT 80\nLOAD_STATE custom.state\n")
        assert b"sequence=05 source=file" in logs and b"[LOAD] loaded" in logs
        samples = array("h", replaced)
        # Native chip residual is mono here. The synthetic stereo signal has
        # a precisely known L-R difference independent of that DC offset.
        assert all(left-right == 4000 for left,right in zip(samples[::2], samples[1::2]))
        wav.unlink()
        logs, _ = run(script="WAIT 1150\nLOAD_STATE custom.state\n")
        assert b"[LOAD] loaded" in logs
        ffmpeg = shutil.which("ffmpeg")
        if ffmpeg:
            for ext in ("ogg", "mp3"):
                asset = music / f"05.{ext}"
                subprocess.run([ffmpeg, "-v", "error", "-f", "lavfi", "-i",
                                "sine=frequency=440:duration=0.2", str(asset)], check=True)
                logs, pcm = run()
                assert b"sequence=05 source=file" in logs and pcm != original and any(pcm)
                asset.unlink()
        else:
            print("Compressed fixtures skipped (ffmpeg encoder unavailable)")

        sfx = music.parent / "sfx"
        sfx.mkdir()
        with wave.open(str(sfx / "19.wav"), "wb") as f:
            f.setparams((1, 2, 8000, 0, "NONE", ""))
            f.writeframes(array("h", [600] * 400).tobytes())
        route = Path(__file__).with_name("doom_gameplay_probe.input").resolve()
        result = subprocess.run([exe, rom, "--turbo", "--max-frames", "4500",
                                 "--input-script", route], cwd=folder, env=env,
                                capture_output=True, timeout=90, check=True)
        assert b"sequence=19 source=file" in result.stderr and b"sequence=00 source=driver" in result.stderr


if __name__ == "__main__":
    check(*(Path(arg).resolve() for arg in sys.argv[1:]))
