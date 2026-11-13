#!/usr/bin/env python3
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

p = argparse.ArgumentParser()
p.add_argument("--sdk", type=Path, required=True)
p.add_argument("--arch", choices=["native", "arm64"], required=True)
p.add_argument("--qemu", type=Path)
p.add_argument("--driver", default="dummy")
a = p.parse_args()
stage = Path("build/smoke-" + a.arch).resolve()
for d in ("bin", "res", "config", "data", "sd", "lib"):
    (stage / d).mkdir(parents=True, exist_ok=True)
shutil.copyfile("build/drive-browser-" + a.arch, stage / "bin/drive-browser")
(stage / "bin/drive-browser").chmod(0o755)
for asset in ("cacert.pem", "DejaVuSans.ttf"):
    shutil.copyfile("app/res/" + asset, stage / "res" / asset)
(stage / "config/settings.example.json").write_text(json.dumps({
    "sd_root": str(stage / "sd"), "allowed_roots": [str(stage / "sd/downloads")]}))
env = dict(os.environ, SDL_VIDEODRIVER=a.driver, SDL_AUDIODRIVER="dummy")
cmd = ["./bin/drive-browser", "--smoke", "20"]
if a.arch == "arm64":
    from package import copy_libraries
    copy_libraries(a.sdk, stage / "lib")
    sysroot = a.sdk / "aarch64-linux-gnu-7.5.0-linaro/aarch64-linux-gnu/libc"
    cmd = [str(a.qemu.resolve()), "-B", "0x100000000", str((sysroot / "lib/ld-linux-aarch64.so.1").resolve()),
           "--library-path", str(stage / "lib") + ":" + str((sysroot / "lib").resolve())] + cmd
subprocess.run(cmd, cwd=stage, env=env, check=True, timeout=40)
image = stage / "data/smoke.bmp"
assert image.read_bytes()[:2] == b"BM" and image.stat().st_size > 1280 * 720 * 3
print("SDL init, font, render, screenshot e encerramento OK:", image)
