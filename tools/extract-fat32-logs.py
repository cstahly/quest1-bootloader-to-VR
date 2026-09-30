from pathlib import Path
import struct
base=Path('/Users/<user>/work/quest-pmos-bringup');d=(base/'pmos-logs.img').read_bytes()
bps=struct.unpack_from('<H',d,11)[0];spc=d[13];reserved=struct.unpack_from('<H',d,14)[0];nf=d[16];spf=struct.unpack_from('<I',d,36)[0];root=struct.unpack_from('<I',d,44)[0];off=(reserved+nf*spf)*bps;cs=bps*spc
out=base/'extracted-pmos-logs';out.mkdir(exist_ok=True)
def chain(c):
 result=bytearray();seen=set()
 while 2<=c<0x0ffffff8:
  assert c not in seen;seen.add(c)
  result+=d[off+(c-2)*cs:off+(c-1)*cs];c=struct.unpack_from('<I',d,reserved*bps+c*4)[0]&0xfffffff
 return bytes(result)
def walk(c,dest):
 raw=chain(c);lfn=[]
 for i in range(0,len(raw),32):
  e=raw[i:i+32]
  if not e[0]:break
  if e[0]==229:lfn=[];continue
  if e[11]==15:
   lfn.insert(0,e[1:11]+e[14:26]+e[28:32]);continue
  name=b''.join(lfn).decode('utf-16le').split('\0')[0].replace('\uffff','') if lfn else e[:8].decode().rstrip()+('.'+e[8:11].decode().rstrip() if e[8:11].strip() else '')
  lfn=[]
  if name in ('.','..') or e[11]&8:continue
  assert '/' not in name and '\\' not in name
  cl=(struct.unpack_from('<H',e,20)[0]<<16)|struct.unpack_from('<H',e,26)[0];size=struct.unpack_from('<I',e,28)[0];p=dest/name
  if e[11]&16:p.mkdir(exist_ok=True);walk(cl,p)
  else:p.write_bytes(chain(cl)[:size]);print(p,size)
walk(root,out)
