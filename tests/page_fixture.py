#!/usr/bin/env python3
"""UI fixture only: page local rclone output; production uses the Go Drive helper."""
import json,os,subprocess,sys,time
from pathlib import Path
args=sys.argv[1:]
tail=args[args.index('drivebrowser-page')+1:]
if tail[0]=='--':tail=tail[1:]
source,token,size=tail
size=int(size);offset=int(token or '0')
assert 1<=size<=1000 and offset>=0
with open('data/page-requests.jsonl','a') as f:f.write(json.dumps([source,offset,size])+'\n')
if offset and Path('data/fail-page-next').exists():sys.exit(1)
if Path('data/slow-page').exists():time.sleep(20)
config=args[args.index('--config')+1]
cmd=json.loads(os.environ['TEST_RCLONE_JSON'])+['--config',config,'lsjson','--',source]
items=json.loads(subprocess.check_output(cmd))
items.sort(key=lambda x:(x['Name'],not x['IsDir']))
print(json.dumps({'Entries':items[offset:offset+size],'NextPageToken':str(offset+size) if offset+size<len(items) else ''}))
