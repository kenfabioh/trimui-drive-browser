#!/usr/bin/env python3
"""Check first-party local documentation links in checkout and built package."""
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit

def verify(root):
    files=list(root.glob('*.md'))+list((root/'docs').rglob('*.md'))
    count=0
    for doc in files:
        body=re.sub(r'```.*?```','',doc.read_text(),flags=re.S)
        targets=re.findall(r'\]\(([^)\n]+)\)',body)
        targets+=re.findall(r'<img\b[^>]*\bsrc="([^"]+)"',body)
        for target in targets:
            parsed=urlsplit(target)
            if parsed.scheme or parsed.netloc or not parsed.path:
                continue
            path=(doc.parent/unquote(parsed.path)).resolve()
            if not path.is_file():
                raise RuntimeError(f'{doc}: broken link {target}')
            count+=1
    print(f'PASS {count} local documentation links: {root}')

verify(Path('.'))
package=Path('build/package/Apps/DriveBrowser')
if package.exists():
    verify(package)
    verify(package/'source')
