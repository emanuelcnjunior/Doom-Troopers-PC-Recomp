"""Exercise manifest opt-in, resource isolation and diagnosis at the real sequence FIFO."""
import copy
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def check(executable, rom):
    example = Path(__file__).resolve().parents[1] / "mods/example-mod"
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        game = shutil.copy2(executable, root)
        shutil.copy2(executable.with_name("SDL2.dll"), root)
        mods = root / "mods"
        pack = mods / "example-mod"
        shutil.copytree(example, pack)
        manifest = json.loads((pack / "mod.json").read_text())
        (root / "launcher.ini").write_text("[doom_launcher]\nprofile=enhanced\naudio_mods=1\n")
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
                   SDL_RENDER_DRIVER="software", DOOM_AUDIO_TRACE="1", DOOM_DISABLE_MODS="0")

        def launch(source):
            run = subprocess.run([game, rom, "--turbo", "--max-frames", "1300"], cwd=root,
                                 env=env, capture_output=True, check=True, timeout=60)
            assert f"sequence=05 source={source}".encode() in run.stderr, run.stderr[-2000:]
            return run.stderr

        launch("driver")
        (mods / "enabled.txt").write_text("example-mod\n")
        launch("file")
        env["DOOM_DISABLE_MODS"] = "1"
        launch("driver")
        env["DOOM_DISABLE_MODS"] = "0"
        shutil.copytree(example, mods / "duplicate")
        (mods / "enabled.txt").write_text("duplicate\nexample-mod\n")
        assert b"conflicting sequence 05" in launch("driver")

        names = ["example-mod"]
        for i, bad_path in enumerate(["../example-mod/audio/music/05.ogg", "https://host/05.ogg",
                                      "audio/music/05.dll"]):
            name = f"invalid-{i}"
            dest = mods / name
            dest.mkdir()
            bad = copy.deepcopy(manifest)
            bad["files"][0]["path"] = bad_path
            (dest / "mod.json").write_text(json.dumps(bad))
            names.append(name)
        for name, data in [("malformed", "{"), ("wrong-game", json.dumps(dict(manifest, compatibility="other")))]:
            dest = mods / name
            dest.mkdir()
            (dest / "mod.json").write_text(data)
            names.append(name)
        outside = root / "external-assets"
        shutil.copytree(example, outside)
        subprocess.run(["powershell", "-NoProfile", "-Command",
                        "New-Item -ItemType Junction -Path $env:MOD_LINK -Target $env:MOD_TARGET | Out-Null"],
                       env=dict(env, MOD_LINK=str(mods / "linked"), MOD_TARGET=str(outside)), check=True)
        names.append("linked")
        (mods / "enabled.txt").write_text("\n".join(names), encoding="utf-8-sig")
        logs = launch("file")
        for name in names[1:]:
            assert f"rejected {name}".encode() in logs


if __name__ == "__main__":
    check(*(Path(arg).resolve() for arg in sys.argv[1:]))
