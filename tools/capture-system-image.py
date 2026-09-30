#!/usr/bin/env python3
"""Read a quiescent system_b through the initramfs shell; never writes device."""
import socket,os,hashlib,time
from pathlib import Path
base=Path('/Volumes/vela/Backups/quest1-recovery/pmos/4k-bringup')
p=base/'pmos-system-4k-ssh.img.partial';out=base/'pmos-system-4k-ssh.img'
assert not p.exists() and not out.exists()
size=809500672;digest=hashlib.sha256();count=0;next_report=128*1024**2
s=socket.create_connection(('172.16.42.1',2323),5);s.settimeout(30)
s.sendall(b'exec 2>/dev/null; dd if=/dev/sda7 bs=1048576 count=772; exit\n')
with os.fdopen(os.open(p,os.O_CREAT|os.O_EXCL|os.O_WRONLY,0o600),'wb') as f:
 while count<size:
  b=s.recv(min(1024*1024,size-count))
  if not b:raise RuntimeError(f'Truncated image: {count}/{size}')
  if count==0:prefix=b
  f.write(b);digest.update(b);count+=len(b)
  if count>=next_report:print(f'Read {count//1024**2} / 772 MiB',flush=True);next_report+=128*1024**2
 f.flush();os.fsync(f.fileno())
s.close()
with p.open('rb') as f,Path('/private/tmp/claude-501/quest-pmos/pmos-system-4k.img').open('rb') as original:
 assert f.read(1024*1024)==original.read(1024*1024), 'Unexpected GPT/leading bytes'
p.rename(out)
(base/'pmos-system-4k-ssh.img.sha256').write_text(digest.hexdigest()+'  '+out.name+'\n')
print('Captured',count,'bytes; SHA256',digest.hexdigest(),flush=True)
