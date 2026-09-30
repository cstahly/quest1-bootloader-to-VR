#!/usr/bin/env python3
"""Repack a pmOS image copy; preserve GPT, UUIDs, file contents and metadata."""
import subprocess as s, pathlib, json, shutil, os
base=pathlib.Path('/home/<user>/pmos/4k-test')
base.mkdir(exist_ok=True)
src=pathlib.Path('/tmp/postmarketOS-export/oculus-monterey.img').resolve()
out=base/'pmos-system-4k.img'
assert not out.exists(), 'Refusing to overwrite previous test'
def run(*args, **kw):
 print('+', *map(str,args), flush=True)
 return s.run(list(map(str,args)), check=True, text=True, **kw)
def capture(*args): return run(*args,stdout=s.PIPE).stdout.strip()
table=json.loads(capture('sfdisk','--json',src))['partitiontable']
assert table['sectorsize']==512
shutil.copyfile(src,out)
for idx,p in enumerate(table['partitions'],1):
 start,size=p['start']*512,p['size']*512
 assert start%4096==0 and size%4096==0
 old=base/f'old-{idx}';new=base/f'new-{idx}'
 old.mkdir(exist_ok=True);new.mkdir(exist_ok=True)
 part=base/f'part-{idx}-4k.img'
 assert not part.exists()
 with part.open('wb') as f:f.truncate(size)
 loop=capture('losetup','--find','--show','--read-only','--offset',start,'--sizelimit',size,src)
 newloop=None
 try:
  uuid=capture('blkid','-s','UUID','-o','value',loop)
  label=capture('blkid','-s','LABEL','-o','value',loop)
  typ=capture('blkid','-s','TYPE','-o','value',loop)
  run('mount','-o','ro,noload' if typ=='ext4' else 'ro',loop,old)
  run('mkfs.'+typ,'-F','-b','4096','-i','8192','-O','^orphan_file,^metadata_csum,^metadata_csum_seed','-U',uuid,'-L',label,part)
  newloop=capture('losetup','--find','--show','--sector-size','4096',part)
  run('mount',newloop,new)
  run('cp','-a',str(old)+'/.',str(new)+'/')
  run('sync','-f',new)
  run('umount',new)
  run('e2fsck','-fn',newloop)
  run('mount','-o','ro',newloop,new)
  difference=capture('rsync','-aHAXnci','--delete',str(old)+'/',str(new)+'/')
  # mkfs creates lost+found independently; ignore only this directory's metadata.
  actual=[x for x in difference.splitlines() if not x.endswith('lost+found/')]
  assert not actual, actual
  run('dumpe2fs','-h',newloop)
  run('umount',new)
 finally:
  s.run(['umount',str(new)],stderr=s.DEVNULL)
  s.run(['umount',str(old)],stderr=s.DEVNULL)
  if newloop:run('losetup','-d',newloop)
  run('losetup','-d',loop)
 with out.open('r+b') as dest,part.open('rb') as source:
  dest.seek(start);shutil.copyfileobj(source,dest)
run('sha256sum',out)
