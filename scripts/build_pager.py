#!/usr/bin/env python3
"""Build read-only pagination extension from pinned rclone/Go source artifacts."""
import argparse,hashlib,os,shutil,subprocess
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('--qemu',type=Path);p.add_argument('--sdk',type=Path,required=True);p.add_argument('--cache',type=Path,required=True);p.add_argument('--github-module-cache',action='store_true',help='Fetch two large modules from official GitHub source, verifying canonical Go h1 sums')
a=p.parse_args();a.sdk=a.sdk.resolve();a.cache=a.cache.resolve()
repo=Path(__file__).resolve().parent.parent
artifacts=[('go1.24.0.linux-amd64.tar.gz','https://go.dev/dl/go1.24.0.linux-amd64.tar.gz','dea9ca38a0b852a74e81c26134671af7c0fbe65d81b0dc1c5bfe22cf7d4c8858'),('rclone-v1.69.1-src.tar.gz','https://codeload.github.com/rclone/rclone/tar.gz/refs/tags/v1.69.1','2fe258d1b4257aef98b53794c27d1b254ee0f2e307634f94fbf3c71786e3b1b5')]
a.cache.mkdir(parents=True,exist_ok=True)
for name,url,sha in artifacts:
 target=a.cache/name
 if not target.exists():subprocess.run(['curl','-fL','--retry','2','-o',str(target),url],check=True)
 assert hashlib.sha256(target.read_bytes()).hexdigest()==sha,name+' checksum mismatch'
source=a.sdk/'rclone-page-source';go_root=a.sdk/'go1.24'
if source.exists():shutil.rmtree(source)
source.mkdir();go_root.mkdir(exist_ok=True)
subprocess.run(['tar','-xzf',str(a.cache/'rclone-v1.69.1-src.tar.gz'),'--strip-components=1','-C',str(source)],check=True)
if not (go_root/'go/bin/go').exists():subprocess.run(['tar','-xzf',str(a.cache/'go1.24.0.linux-amd64.tar.gz'),'-C',str(go_root)],check=True)
f=source/'backend/drive/drive.go';s=f.read_text()
def replace(old,new):
 global s
 expected=2 if old.startswith(('fields :=','if files.IncompleteSearch','if files.NextPageToken')) else 1
 assert s.count(old)==expected,old[:80]
 s=s.replace(old,new,1)
replace('type Fs struct {','type Fs struct {\n browserPage *browserPageRequest')
replace('fields := fmt.Sprintf("files(%s),nextPageToken,incompleteSearch", f.getFileFields(ctx))','fields := fmt.Sprintf("files(%s),nextPageToken,incompleteSearch", f.getFileFields(ctx))\n if f.browserPage!=nil {list.PageSize(f.browserPage.Size).PageToken(f.browserPage.Token).OrderBy("name,folder")}')
replace('if files.IncompleteSearch {','if f.browserPage!=nil {f.browserPage.Next=files.NextPageToken; if files.IncompleteSearch {return false,fmt.Errorf("incomplete paginated listing")}}\n\t\tif files.IncompleteSearch {')
replace('if files.NextPageToken == "" {\n\t\t\tbreak','if f.browserPage!=nil {break}\n\t\tif files.NextPageToken == "" {\n\t\t\tbreak')
replace('var commandHelp = []fs.CommandHelp{{','var commandHelp = []fs.CommandHelp{{Name: "drivebrowser-page", Short: "Read one page without scanning the folder"}, {')
replace('switch name {\n\tcase "get":','switch name {\n case "drivebrowser-page": return f.browserListPage(ctx,arg)\n\tcase "get":')
f.write_text(s)
for name in ['drivebrowser_page.go','drivebrowser_page_test.go']:
 if (repo/'third_party/rclone-page'/name).exists():shutil.copyfile(repo/'third_party/rclone-page'/name,source/'backend/drive'/name)
(source/'drivebrowser').mkdir();shutil.copyfile(repo/'third_party/rclone-page/main.go',source/'drivebrowser/main.go')
env=dict(os.environ,CGO_ENABLED='0',GOMAXPROCS='2',GOTOOLCHAIN='local',GOCACHE=str(a.sdk/'go-build-cache'),GOMODCACHE=str(a.sdk/'go-mod-cache'))
go=str(go_root/'go/bin/go')

if a.github_module_cache:
 sums={line.split()[0]+' '+line.split()[1]:line.split()[2] for line in (source/'go.sum').read_text().splitlines()}
 for module,version,github in [('google.golang.org/api','v0.211.0','googleapis/google-api-go-client'),('github.com/klauspost/compress','v1.17.11','klauspost/compress')]:
  cached=a.sdk/'go-mod-cache/cache/download'/module/'@v'/(version+'.zip')
  if cached.exists():continue
  archive=a.cache/(github.replace('/','-')+'-'+version+'.tgz')
  if not archive.exists():subprocess.run(['curl','-fL','--retry','2','-o',str(archive),'https://codeload.github.com/'+github+'/tar.gz/refs/tags/'+version],check=True)
  directory=a.sdk/(github.replace('/','-')+'-'+version)
  directory.mkdir(exist_ok=True)
  subprocess.run(['tar','-xzf',str(archive),'--strip-components=1','-C',str(directory)],check=True)
  temporary=a.cache/(github.replace('/','-')+'-'+version+'.zip')
  digest=subprocess.check_output([go,'run','-mod=readonly',str(repo/'third_party/rclone-page/modulezip.go'),str(directory),str(temporary),module,version],cwd=source,env=env,text=True).strip()
  assert digest==sums[module+' '+version],module+' canonical Go checksum mismatch'
  cached.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(temporary,cached)
  cached.with_suffix('.ziphash').write_text(digest)

if (source/'backend/drive/drivebrowser_page_test.go').exists():subprocess.run([go,'test','-mod=readonly','./backend/drive','-run','^TestBrowser','-count=1'],cwd=source,env=env,check=True)
for arch in ['amd64','arm64']:
 subprocess.run([go,'build','-mod=readonly','-trimpath','-ldflags=-s -w','-o',str(a.sdk/('rclone-page-'+arch)),'./drivebrowser'],cwd=source,env=dict(env,GOOS='linux',GOARCH=arch),check=True)
print('Read-only page helper built for amd64 and arm64')

if a.qemu:
 subprocess.run([go,'test','-mod=readonly','-c','-o',str(a.sdk/'rclone-page-test-arm64'),'./backend/drive'],cwd=source,env=dict(env,GOOS='linux',GOARCH='arm64'),check=True)
 subprocess.run([str(a.qemu.resolve()),str(a.sdk/'rclone-page-test-arm64'),'-test.run=^TestBrowser','-test.count=1'],check=True)
licenses=a.sdk/'rclone-page-licenses';licenses.mkdir(exist_ok=True)
modules=subprocess.check_output([go,'list','-mod=readonly','-deps','-f','{{with .Module}}{{.Path}}|{{.Version}}|{{.Dir}}{{end}}','./drivebrowser'],cwd=source,env=env,text=True)
rows=['# Compiled pagination helper dependencies','', 'Source: rclone v1.69.1 with the MIT-licensed Drive Browser extension.','']
for line in sorted(set(filter(None,modules.splitlines()))):
 module,version,directory=line.split('|');directory=Path(directory)
 target=licenses/(module.replace('/','_')+'@'+(version or 'v1.69.1-patched'));target.mkdir(exist_ok=True)
 found=[]
 for name in ['LICENSE','LICENSE.txt','LICENSE.md','COPYING','COPYING.txt','NOTICE','NOTICE.txt']:
  if (directory/name).is_file():shutil.copyfile(directory/name,target/name);found.append(name)
 rows.append('* '+module+' '+version+' — '+', '.join(found))
shutil.copyfile(go_root/'go/LICENSE',licenses/'Go-LICENSE')
(licenses/'MODULES.md').write_text('\n'.join(rows)+'\n')
