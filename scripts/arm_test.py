#!/usr/bin/env python3
import argparse
from pathlib import Path
import shlex
import subprocess

p = argparse.ArgumentParser()
p.add_argument("--sdk", type=Path, required=True)
p.add_argument("--qemu", type=Path, required=True)
a = p.parse_args()
sysroot = a.sdk / "aarch64-linux-gnu-7.5.0-linaro/aarch64-linux-gnu/libc"
wrapper = Path("build/rclone-arm-emulated").resolve()
wrapper.write_text("#!/bin/sh\nexec " + shlex.quote(str(a.qemu.resolve())) + " " +
                   shlex.quote(str((a.sdk / "rclone-v1.69.1-linux-arm64/rclone").resolve())) + ' "$@"\n')
wrapper.chmod(0o755)
# Explicit loader avoids QEMU -L remapping open("/") into the SDK tree.
subprocess.run([str(a.qemu), "-B", "0x100000000", str(sysroot / "lib/ld-linux-aarch64.so.1"),
                "--library-path", str(sysroot / "lib"), "build/core-test-arm64", str(wrapper)], check=True)
