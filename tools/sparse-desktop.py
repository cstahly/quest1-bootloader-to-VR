from pathlib import Path
import struct,hashlib
base=Path('/private/tmp/claude-501/quest-pmos');src=base/'pmos-system-desktop.img';out=base/'pmos-system-desktop.sparse.img'
size=src.stat().st_size;bs=4096;chunks=0
with src.open('rb') as f,out.open('wb') as g:
 g.write(b'\0'*28)
 while data:=f.read(1024*1024):
  assert len(data)%bs==0
  if data==b'\0'*len(data):
   g.write(struct.pack('<HHII',0xcac2,0,len(data)//bs,16));g.write(b'\0'*4)
  else:
   g.write(struct.pack('<HHII',0xcac1,0,len(data)//bs,len(data)+12));g.write(data)
  chunks+=1
 g.seek(0);g.write(struct.pack('<IHHHHIIII',0xed26ff3a,1,0,28,12,bs,size//bs,chunks,0))
out.chmod(0o600)
h=hashlib.sha256();total=0
with out.open('rb') as f:
 f.read(28)
 for _ in range(chunks):
  typ,_,n,sz=struct.unpack('<HHII',f.read(12));payload=f.read(sz-12)
  data=payload if typ==0xcac1 else payload*(n*bs//4)
  assert len(data)==n*bs;h.update(data);total+=len(data)
 assert not f.read(1)
assert total==size and h.hexdigest()=='efe1bcff08d126b783f37c97a1438570c539fdb248429b0eb385c643b29ad3b0'
print('Exact sparse round-trip verified',total,out.stat().st_size)
