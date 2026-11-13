#!/usr/bin/env python3
"""Explicit package allowlist, never copies local config, data or credentials."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile

LIBRARIES = ["libSDL2-2.0.so.0", "libSDL2_ttf-2.0.so.0", "libfreetype.so.6",
             "libz.so.1", "libbz2.so.1.0"]

def copy_libraries(sdk, target):
    target.mkdir(parents=True, exist_ok=True)
    for lib in LIBRARIES:
        shutil.copyfile(sdk / "usr/lib" / lib, target / lib)
    compiler = sdk / "aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-g++"
    runtime = subprocess.check_output([str(compiler), "-print-file-name=libgcc_s.so.1"], text=True).strip()
    shutil.copyfile(runtime, target / "libgcc_s.so.1")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--sdk", type=Path, required=True)
    a = p.parse_args()
    stage = Path("build/package/Apps/DriveBrowser")
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    for f in ("launch.sh", "config.json", "icon.png"):
        shutil.copyfile(Path("app") / f, stage / f)
    (stage / "res").mkdir()
    for name in ("cacert.pem", "DejaVuSans.ttf"):
        shutil.copyfile(Path("app/res") / name, stage / "res" / name)
    shutil.copytree("app/licenses", stage / "licenses")
    (stage / "config").mkdir()
    for f in ("settings.example.json", "rclone.conf.example"):
        shutil.copyfile(Path("app/config") / f, stage / "config" / f)
    (stage / "bin").mkdir()
    shutil.copyfile("build/drive-browser-arm64", stage / "bin/drive-browser")
    strip = a.sdk / "aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-strip"
    subprocess.run([str(strip), str(stage / "bin/drive-browser")], check=True)
    shutil.copyfile(a.sdk / "rclone-v1.69.1-linux-arm64/rclone", stage / "bin/rclone")
    for f in (stage / "launch.sh", stage / "bin/rclone", stage / "bin/drive-browser"):
        f.chmod(0o755)
    copy_libraries(a.sdk, stage / "lib")
    shutil.copyfile(a.sdk / "rclone-page-arm64",stage / "bin/rclone-page")
    (stage / "bin/rclone-page").chmod(0o755)
    shutil.copytree(a.sdk / "rclone-page-licenses",stage / "licenses/pager")
    public_docs=("README.md", "README.en.md", "CONTRIBUTING.md", "CHANGELOG.md", "LICENSE")
    for name in public_docs:
        body=Path(name).read_text().replace('src="app/icon.png"','src="icon.png"').replace('](app/licenses/','](licenses/')
        (stage / name).write_text(body)
    shutil.copytree("docs", stage / "docs")
    for doc in (stage / "docs").rglob("*.md"):
        doc.write_text(doc.read_text().replace('](../app/licenses/','](../licenses/'))
    # Corresponding application source and build instructions are included.
    source = stage / "source"
    source.mkdir()
    for d in ("src", "scripts", "tests", "third_party"):
        shutil.copytree(d, source / d, ignore=shutil.ignore_patterns("__pycache__"))
    shutil.copyfile("Makefile", source / "Makefile")
    for name in public_docs:
        shutil.copyfile(name, source / name)
    shutil.copyfile("VERSION", source / "VERSION")
    for name in (".gitignore", ".gitattributes"):
        shutil.copyfile(name, source / name)
    shutil.copytree(".github", source / ".github")
    shutil.copytree("docs", source / "docs")
    source_app = source / "app"
    source_app.mkdir()
    for name in ("launch.sh", "config.json", "icon.png"):
        shutil.copyfile(Path("app") / name, source_app / name)
    # Reuse only the public resources/examples selected above, never the full
    # working app/ tree (which could contain config backups or user data).
    for directory in ("res", "licenses", "config"):
        shutil.copytree(stage / directory, source_app / directory)
    manifest = {str(f.relative_to(stage)): hashlib.sha256(f.read_bytes()).hexdigest()
                for f in sorted(stage.rglob("*")) if f.is_file()}
    (stage / "SHA256SUMS.json").write_text(json.dumps(manifest, indent=2) + "\n")
    Path("dist").mkdir(exist_ok=True)
    version=Path("VERSION").read_text().strip()
    out = Path(f"dist/DriveBrowser-{version}-arm64.zip")
    with zipfile.ZipFile(out, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for f in sorted(stage.rglob("*")):
            if f.is_file():
                z.write(f, f.relative_to("build/package"))
    digest = hashlib.sha256(out.read_bytes()).hexdigest()
    out.with_suffix(".zip.sha256").write_text(digest + "  " + out.name + "\n")
    print(out, out.stat().st_size, "bytes", digest)

if __name__ == "__main__":
    main()
