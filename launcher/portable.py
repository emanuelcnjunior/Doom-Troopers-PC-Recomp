"""Assemble the notices from pinned Git objects and verify the portable archive."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
LOCK = json.loads((ROOT / "config/framework_lock.json").read_text())
ROM = json.loads((ROOT / "config/rom_identity.json").read_text())
DLLS = {
    "SDL2.dll": "de23db1694a3c7a4a735e7ecd3d214b2023cc2267922c6c35d30c7fc7370d677",
    "msvcp140.dll": "0f885b509a685d2bbfa652fed26b5fb31d88fbdab0a978c641d1c7b8aa460aa9",
    "vcruntime140.dll": "d5e4d9a3e835fa679450145d6a7d94e36573a509317111904d9b3712c30d9066",
    "vcruntime140_1.dll": "1f2d41c4aa5db0bc33ebf7b66d72943a817d7ce6cbe880502a9403823633093f",
}
PAYLOAD = [*DLLS, "DoomBootProbe.exe", "LICENSES.txt", "LEIA-ME.txt", "recomplauncher.json"]
MANIFEST = "SHA256SUMS.txt"


def pinned(repo, revision, path):
    return subprocess.check_output(["git", "-C", repo, "show", f"{revision}:{path}"])


def notices(repo, folder):
    def source(path):
        return pinned(repo, LOCK["commit"], path).decode("utf-8-sig")

    parts = []
    for path in ("LICENSE.md", "THIRD-PARTY-LICENSES.md", "runner/external/SDL2/SDL2-2.28.5/COPYING.txt",
                 "runner/external/ymfm/LICENSE", "runner/external/superzazu/LICENSE",
                 "runner/external/clowncommon/LICENCE.txt", "runner/external/minicoro/LICENSE"):
        parts.extend([path, source(path)])
    header = source("runner/external/minicoro/minicoro.h")
    start = header.index("Some of the following assembly code is taken from LuaCoco")
    parts.extend(["LuaCoco", header[start:header.index("*/", start)]])
    for path in ("external/m68k-recomp-core", "external/rbengine"):
        parts.extend([path, pinned(repo / path, LOCK["submodules"][path], "LICENSE").decode()])
    mit = pinned(repo / "external/rbengine", LOCK["submodules"]["external/rbengine"], "LICENSE").decode()
    parts.extend(["ShadowVerifier and color-science core", "Copyright Jrickey",
                  mit[mit.index("Permission is hereby granted") :]])
    parts.extend(["Microsoft Visual C++ Runtime (Microsoft Corporation)",
                  "https://learn.microsoft.com/visualstudio/releases/2022/redistribution"])
    (folder / "LICENSES.txt").write_bytes("\n\n".join(parts).encode("utf-8"))


def checksum(data):
    return hashlib.sha256(data).hexdigest()


def verify_dlls(folder):
    for name, digest in DLLS.items():
        if checksum((folder / name).read_bytes()) != digest:
            raise ValueError(f"Dependency pin mismatch: {name}")


def archive(folder, target, direct=False, integrated=False):
    payload = PAYLOAD.copy()
    if direct:
        payload.extend(["Jogar.cmd", "settings.ini"])
    elif not integrated:
        payload.append("Launcher.exe")
    payload.sort()
    if sorted(p.name for p in folder.iterdir()) != payload:
        raise ValueError("Unexpected or missing staging entries")
    entries = {name: (folder / name).read_bytes() for name in payload}
    for name, data in entries.items():
        for marker in re.finditer(b"SEGA (?:GENESIS|MEGA DRIVE)", data):
            start = marker.start() - 256
            if start >= 0 and checksum(data[start:start + ROM["size"]]) == ROM["sha256"]:
                raise ValueError(f"Embedded USA ROM found: {name}")
        if data[256:272].startswith((b"SEGA GENESIS", b"SEGA MEGA DRIVE")):
            raise ValueError(f"Cartridge image found: {name}")
        if name.endswith((".exe", ".dll")):
            offset = int.from_bytes(data[60:64], "little")
            if data[:2] != b"MZ" or data[offset:offset + 6] != b"PE\0\0\x64\x86":
                raise ValueError(f"Expected Windows x64 PE: {name}")
        for encoding in ("ascii", "utf-16le"):
            pattern = re.compile(re.escape("C:\\Users\\".encode(encoding)), re.I)
            if pattern.search(data):
                raise ValueError(f"Private profile path in {name}")
    manifest = "".join(f"{checksum(data)}  {name}\n" for name, data in entries.items())
    entries[MANIFEST] = manifest.encode("ascii")
    with zipfile.ZipFile(target, "x", zipfile.ZIP_DEFLATED) as output:
        for name, data in entries.items():
            output.writestr(name, data)
    with zipfile.ZipFile(target) as result:
        if result.namelist() != list(entries) or result.testzip():
            raise ValueError("Invalid archive directory or CRC")
        for name, data in entries.items():
            if result.read(name) != data:
                raise ValueError(f"Archive content mismatch: {name}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("notices", "archive", "archive-direct", "archive-integrated"))
    parser.add_argument("first", type=Path)
    parser.add_argument("second", type=Path)
    args = parser.parse_args()
    if args.action == "notices":
        verify_dlls(args.second)
        notices(args.first, args.second)
    else:
        verify_dlls(args.first)
        archive(args.first, args.second, direct=args.action == "archive-direct",
                integrated=args.action == "archive-integrated")
