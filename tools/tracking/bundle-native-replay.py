#!/usr/bin/env python3
"""Create a private ARM replay dependency bundle; never installs system libraries.

Requires readelf on the build host. An explicit private musl loader must launch it
with --library-path pointing to the bundle's lib directory. No camera data included.
"""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('replay');p.add_argument('basalt_lib');p.add_argument('opencv_lib_dir')
p.add_argument('sysroot');p.add_argument('output')
a=p.parse_args();out=Path(a.output)
if out.exists():p.error('output already exists; choose a new directory')
root=Path(a.sysroot);search=[Path(a.basalt_lib).parent,Path(a.opencv_lib_dir),root/'usr/lib',root/'lib']
queue=[('replay-vit',Path(a.replay)),('lib/libbasalt.so.2',Path(a.basalt_lib))];files={}
while queue:
 name,src=queue.pop()
 if name in files:continue
 header=subprocess.check_output(['readelf','-h',str(src)],text=True)
 if 'AArch64' not in header:raise RuntimeError(f'Not an ARM64 ELF: {src}')
 deps=re.findall(r'Shared library: \[(.*?)\]',subprocess.check_output(['readelf','-d',str(src)],text=True))
 files[name]=src.resolve()
 for dep in deps:
  if '/' in dep:raise RuntimeError(f'Unexpected dependency path: {dep}')
  found=next((directory/dep for directory in search if (directory/dep).is_file()),None)
  if found is None:raise RuntimeError(f'Missing dependency {dep}')
  queue.append(('lib/'+dep,found))
size=sum(src.stat().st_size for src in files.values())
if size>64*1024*1024:raise RuntimeError(f'Unexpected dependency growth: {size} bytes')
(out/'lib').mkdir(parents=True)
manifest={}
for name,src in files.items():
 dest=out/name;shutil.copy2(src,dest)
 manifest[name]={'bytes':dest.stat().st_size,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest()}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({'files':len(files),'bytes':size,'output':str(out)}))
