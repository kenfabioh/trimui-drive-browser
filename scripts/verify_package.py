#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import struct
import subprocess
import tempfile
import zipfile
from make_icon import check_icon

version=Path("VERSION").read_text().strip()
package = Path(f"dist/DriveBrowser-{version}-arm64.zip")
with zipfile.ZipFile(package) as z:
    assert z.testzip() is None
    for name in z.namelist():
        path = PurePosixPath(name)
        assert not path.is_absolute() and ".." not in path.parts
        assert name.startswith("Apps/DriveBrowser/")
        assert path.name not in {"rclone.conf", "settings.json"}, name
        assert ".git" not in path.parts and "data" not in path.parts
    prefix = "Apps/DriveBrowser/"
    manifest = json.loads(z.read(prefix + "SHA256SUMS.json"))
    for name, digest in manifest.items():
        assert hashlib.sha256(z.read(prefix + name)).hexdigest() == digest
    for name in ("bin/drive-browser", "bin/rclone", "bin/rclone-page"):
        data = z.read(prefix + name)
        assert data[:4] == b"\x7fELF" and data[4:6] == b"\x02\x01"
        assert struct.unpack_from("<H", data, 18)[0] == 183, "not AArch64"
        assert z.getinfo(prefix + name).external_attr >> 16 & 0o111
    for info in z.infolist():
        if "/lib/" not in info.filename and info.filename != prefix + "bin/drive-browser":
            continue
        with tempfile.NamedTemporaryFile() as f:
            f.write(z.read(info.filename)); f.flush()
            versions = subprocess.check_output(["readelf", "-V", f.name], text=True)
            for version in re.findall(r"Name: GLIBC_([\d.]+)", versions):
                assert tuple(map(int, version.split('.'))) <= (2, 23), (info.filename, version)
    image = z.read(prefix + "icon.png")
    assert image[:8] == b"\x89PNG\r\n\x1a\n"
    assert struct.unpack_from('!IIBB',image,16)==(300,300,8,6), 'Icon must be RGBA 300x300'
    check_icon(image)
    for doc in ('README.md','README.en.md','CONTRIBUTING.md','CHANGELOG.md','LICENSE','docs/AJUDA.md'):
        assert prefix+doc in z.namelist()
    actual={n[len(prefix):] for n in z.namelist() if n!=prefix+'SHA256SUMS.json'}
    assert actual==set(manifest), 'Manifest must cover every packaged file'
print("PASS ZIP: CRC, estrutura, hashes completos, icone RGBA 300x300, AArch64, GLIBC <=2.23, sem configuracao privada")
