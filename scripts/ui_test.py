#!/usr/bin/env python3
"""Headless end-to-end workflow; synthetic controller, real local rclone."""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

p=argparse.ArgumentParser()
p.add_argument('--sdk',type=Path,required=True)
p.add_argument('--arch',choices=['native','arm64'],required=True)
p.add_argument('--qemu',type=Path)
a=p.parse_args()
stage=Path(tempfile.mkdtemp(prefix='drivebrowser-ui-'+a.arch+'-',dir='/tmp'))
for d in ['bin','res','config','data','source/jogos','source/outros','source/zz-alfabeto/A-pasta','source/zz-alfabeto/V-pasta','sd/downloads/destino interno','lib']:
    (stage/d).mkdir(parents=True,exist_ok=True)
for f in ['DejaVuSans.ttf','cacert.pem']:
    shutil.copyfile(Path('app/res')/f,stage/'res'/f)
(stage/'config/settings.example.json').write_text(json.dumps({
    'sd_root':str(stage/'sd'),'allowed_roots':[str(stage/'sd/downloads')],'download_buffer_mib':32}))
(stage/'config/rclone.conf').write_text('[fixture]\ntype = alias\nremote = '+str(stage/'source')+'\n')
(stage/'source/jogos/Áção final.zip').write_bytes(b'first folder content')
(stage/'source/outros/Áção final.zip').write_bytes(b'second folder data!')
for name in ['9.zip','A00.zip','A01.zip','A02.zip','B00.zip','C00.zip','D00.zip','E00.zip','F00.zip','G00.zip','M00.zip','M01.zip','Z00.zip','Álbum.zip','_extra.zip','世界.zip']:
    (stage/'source/zz-alfabeto'/name).write_bytes(b'letter payload')
dest=stage/'sd/downloads/destino interno'
(dest/'manter.txt').write_bytes(b'keep')
(dest/'.drivebrowser-hidden.part').write_bytes(b'partial')
(dest/'link').symlink_to(stage/'source',target_is_directory=True)
shutil.copyfile('build/ui-test-'+a.arch,stage/'bin/ui-test')
(stage/'bin/ui-test').chmod(0o755)
rclone=[str((a.sdk/'rclone-v1.69.1-linux-amd64/rclone').resolve())]
env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
cmd=['./bin/ui-test']
if a.arch=='arm64':
    from package import copy_libraries
    copy_libraries(a.sdk,stage/'lib')
    sysroot=(a.sdk/'aarch64-linux-gnu-7.5.0-linaro/aarch64-linux-gnu/libc/lib').resolve()
    cmd=[str(a.qemu.resolve()),'-B','0x100000000',str(sysroot/'ld-linux-aarch64.so.1'),
         '--library-path',str(stage/'lib')+':'+str(sysroot)]+cmd
    rclone=[str(a.qemu.resolve()),str((a.sdk/'rclone-v1.69.1-linux-arm64/rclone').resolve())]
    env['SDL_VIDEODRIVER']='offscreen'
wrapper=stage/'bin/rclone'
wrapper.write_text('#!/bin/sh\nfor arg in "$@"; do\n'
                   '  if [ "$arg" = listremotes ]; then\n'
                   '    printf \'[{"name":"fixture","type":"drive"}]\\n\'\n'
                   '    exit 0\n  fi\n'
                   '  if [ "$arg" = --recursive ] && [ -f data/slow-search ]; then sleep 20; fi\n'
                   'done\nexec '+shlex.join(rclone)+' "$@"\n')
wrapper.chmod(0o755)
subprocess.run(cmd,cwd=stage,env=env,check=True,timeout=100)
assert (dest/'Áção final.zip').read_bytes()==b'second folder data!'
assert (dest/'manter.txt').read_bytes()==b'keep'
assert (dest/'M01.zip').read_bytes()==b'letter payload'
assert list(dest.glob('.drivebrowser-*.part'))==[dest/'.drivebrowser-hidden.part']
for image in ['ui-download.bmp','ui-letters.bmp','ui-qwerty.bmp']:
    assert (stage/'data'/image).stat().st_size>1280*720*3
print('PASS UI exact source/destination content, no overwrite, no new partials:',stage)
