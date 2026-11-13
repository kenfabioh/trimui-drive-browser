#!/usr/bin/env python3
"""Pinned official artifacts; no system installation or TLS bypass."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tarfile
import zipfile

SDK_RELEASE = "https://github.com/trimui/toolchain_sdk_smartpro/releases/download/20231018/"
ARTIFACTS = [
    ("toolchain.tgz", SDK_RELEASE + "aarch64-linux-gnu-7.5.0-linaro.tgz", "604d97c1f5fadc88d6ebff577ea648d0fbc650411af98a3bd8783dbe4e3ed154"),
    ("sdk.tgz", SDK_RELEASE + "SDK_usr_tg5040_a133p.tgz", "b6b615d03204e9d9bb1a91c31de0c3402434f6b8fc780743fa88a5f1da6f3c79"),
    ("sdl.tgz", SDK_RELEASE + "SDL2-2.26.1.GE8300.tgz", "a37fbed6c8771f1050e6520fa9c48354e01519de813fe176ae53089f2087bb87"),
    ("rclone-arm64.zip", "https://github.com/rclone/rclone/releases/download/v1.69.1/rclone-v1.69.1-linux-arm64.zip", "a03de8f700fcda7a1aef6b568f88d44218b698fb4e1637596c024d341bb24124"),
    ("rclone-amd64.zip", "https://github.com/rclone/rclone/releases/download/v1.69.1/rclone-v1.69.1-linux-amd64.zip", "231841f8d8029ae6cfca932b601b3b50d0e2c3c2cb9da3166293f1c3eae7d79c"),
    ("qemu-aarch64-static", "https://github.com/multiarch/qemu-user-static/releases/download/v7.2.0-1/qemu-aarch64-static", "dce64b2dc6b005485c7aa735a7ea39cb0006bf7e5badc28b324b2cd0c73d883f"),
    ("SDL_ttf.c", "https://raw.githubusercontent.com/libsdl-org/SDL_ttf/release-2.0.15/SDL_ttf.c", "4d76797b639a86c40b28947803e1d267256378384617920f77152e500d2c9311"),
    ("SDL_ttf.h", "https://raw.githubusercontent.com/libsdl-org/SDL_ttf/release-2.0.15/SDL_ttf.h", "d8812df3f415637d49ea647d66d10d4ba7cead505445a5d8e6d089a92052d58b"),
]

def run(*args):
    subprocess.run(args, check=True)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--sdk", type=Path, default=Path(".deps/sdk"))
    p.add_argument("--cache", type=Path, default=Path(".deps/cache"))
    p.add_argument("--skip-native", action="store_true")
    args = p.parse_args()
    args.sdk.mkdir(parents=True, exist_ok=True)
    args.cache.mkdir(parents=True, exist_ok=True)
    for name, url, digest in ARTIFACTS:
        target = args.cache / name
        if not target.exists():
            temporary = target.with_suffix(target.suffix + ".tmp")
            run("curl", "-fL", "--retry", "2", "--output", str(temporary), url)
            if hashlib.sha256(temporary.read_bytes()).hexdigest() != digest:
                temporary.unlink()
                raise RuntimeError("Checksum incorreto: " + name)
            temporary.replace(target)
        if hashlib.sha256(target.read_bytes()).hexdigest() != digest:
            raise RuntimeError("Cache alterado: " + str(target))
        if name.endswith(".tgz"):
            marker = args.sdk / (".extracted-" + name + "-" + digest)
            if not marker.exists():
                # Pinned vendor archives only, never untrusted user archives.
                run("tar", "-xzf", str(target.resolve()), "-C", str(args.sdk))
                marker.touch()
        if name.endswith(".zip"):
            with zipfile.ZipFile(target) as z:
                z.extractall(args.sdk)
        if name == "qemu-aarch64-static":
            target.chmod(0o755)
    for arch in ("amd64", "arm64"):
        (args.sdk / f"rclone-v1.69.1-linux-{arch}/rclone").chmod(0o755)
    if not args.skip_native:
        run("gcc", "-shared", "-fPIC", "-O2", "-I" + str(args.cache),
            "-I" + str(args.sdk / "usr/include/SDL2"),
            "-I" + str(args.sdk / "usr/include/freetype2"), str(args.cache / "SDL_ttf.c"),
            "-o", str(args.sdk / "libSDL2_ttf-native.so"),
            "-l:libfreetype.so.6", "-l:libSDL2-2.0.so.0", "-lm")
    print("Dependencias verificadas e preparadas.")

if __name__ == "__main__":
    main()
