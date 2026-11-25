"""Exercise profile migration through the integrated Win32 dialog, using disposable INIs."""
import configparser
import ctypes
from ctypes import wintypes
from contextlib import contextmanager
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ui = ctypes.WinDLL("user32")
ui.GetDlgItem.argtypes = (wintypes.HWND, ctypes.c_int)
ui.GetDlgItem.restype = wintypes.HWND
ui.SendMessageW.argtypes = (wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM)
ui.IsWindowEnabled.argtypes = (wintypes.HWND,)
ui.GetWindowThreadProcessId.argtypes = (wintypes.HWND, ctypes.POINTER(wintypes.DWORD))
callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


@contextmanager
def dialog(game, env):
    with (game.parent / "menu-runtime.log").open("wb") as log:
        proc = subprocess.Popen([game], cwd=game.parent, env=env, stderr=log)
    windows = []

    @callback
    def visit(hwnd, unused):
        pid = wintypes.DWORD()
        ui.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value == proc.pid and ui.GetDlgItem(hwnd, 234):
            windows.append(hwnd)
        return True

    try:
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline and proc.poll() is None:
            ui.EnumWindows(visit, 0)
            if windows:
                break
            time.sleep(0.05)
        assert windows, "Integrated dialog did not open"
        yield windows[0]
    finally:
        if proc.poll() is None:
            proc.terminate()
        proc.wait(timeout=10)


def check(executable, rom):
    with tempfile.TemporaryDirectory() as tmp:
        folder = Path(tmp)
        game = Path(shutil.copy2(executable, folder))
        shutil.copy2(executable.with_name("SDL2.dll"), folder)
        env = dict(os.environ, SDL_AUDIODRIVER="dummy", SDL_VIDEODRIVER="dummy",
                   SDL_RENDER_DRIVER="software", DOOM_TRANSITION_TRACE="1")
        for key in ("GENESIS_NO_LAUNCHER", "DOOM_SCALE2X"):
            env.pop(key, None)
        ini = folder / "launcher.ini"
        settings = folder / "settings.ini"
        for legacy, enhanced in [(None, False), ("0", False), ("1", True)]:
            if legacy is not None:
                ini.write_text(f"[doom_launcher]\nlanguage=pt\nscale2x={legacy}\n")
            with dialog(game, env) as hwnd:
                def control(i):
                    return ui.GetDlgItem(hwnd, i)
                def checked(i):
                    return bool(ui.SendMessageW(control(i), 0xF0, 0, 0))
                def click(i):
                    ui.SendMessageW(control(i), 0xF5, 0, 0)
                assert checked(235) == enhanced and checked(234) != enhanced
                assert checked(209) == enhanced
                assert bool(ui.IsWindowEnabled(control(209))) == enhanced
                assert not checked(236)
                assert bool(ui.IsWindowEnabled(control(236))) == enhanced
                click(235)
                assert not checked(237) and not checked(238) and checked(239)
                for ident in (237, 238, 239):
                    click(ident)
                click(236)
                assert checked(236)
                if not checked(209):
                    click(209)
                click(234)
                assert not checked(237) and not checked(238) and checked(239)
                assert not ui.IsWindowEnabled(control(237))
                assert not checked(209) and not checked(236)
                click(235)
                assert checked(237) and checked(238) and not checked(239)
                assert checked(209) and checked(236)
                if legacy == "1":
                    click(234)
                    path = ctypes.create_unicode_buffer(str(rom))
                    ui.SendMessageW(control(206), 0xC, 0, ctypes.addressof(path))
                    click(1)
                    assert settings.exists()
                    # Play must reach emulated game code after validating the
                    # ROM, not just save the INIs and close the dialog.
                    deadline = time.monotonic() + 10
                    while time.monotonic() < deadline:
                        if b"[DOOM_TRANSITION]" in (folder / "menu-runtime.log").read_bytes():
                            break
                        time.sleep(0.05)
                    else:
                        raise AssertionError("Play did not reach the guest frame loop")

        config = configparser.ConfigParser()
        config.read(ini)
        saved = config["doom_launcher"]
        assert (saved["profile"], saved["enhanced_scale2x"], saved["scale2x"],
                saved["language"]) == ("classic", "1", "0", "pt")
        assert saved["audio_mods"] == "1"
        with dialog(game, env) as hwnd:
            assert ui.SendMessageW(ui.GetDlgItem(hwnd, 234), 0xF0, 0, 0)
            ui.SendMessageW(ui.GetDlgItem(hwnd, 235), 0xF5, 0, 0)
            assert ui.SendMessageW(ui.GetDlgItem(hwnd, 209), 0xF0, 0, 0)
            ui.SendMessageW(ui.GetDlgItem(hwnd, 1), 0xF5, 0, 0)
        for mode, rate in [("enhanced", "60.0000"), ("classic", "59.9401")]:
            config.read(ini)
            assert config["doom_launcher"]["profile"] == mode
            run = subprocess.run([game, rom, "--max-frames", "120", "--turbo"],
                                 cwd=folder, env=env, capture_output=True, timeout=45, check=True)
            assert rate.encode() in run.stderr, run.stderr[-2000:]
            expected = b"bilinear=1 integer=1 vsync_requested=0" if mode == "enhanced" else b"bilinear=0 integer=0"
            assert expected in run.stderr, run.stderr[-3000:]
            config["doom_launcher"]["profile"] = "classic"
            with ini.open("w") as out:
                config.write(out)


if __name__ == "__main__":
    check(*(Path(arg).resolve() for arg in sys.argv[1:]))
