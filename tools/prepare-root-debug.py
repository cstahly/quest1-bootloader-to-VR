from pathlib import Path
import struct,gzip,hashlib,subprocess
base=Path('/private/tmp/claude-501/quest-pmos');d=bytearray((base/'pmos-boot-4k-path-raw.img').read_bytes())
a=lambda n:(n+4095)//4096*4096
ks=struct.unpack_from('<I',d,8)[0];rs=struct.unpack_from('<I',d,16)[0];ss=struct.unpack_from('<I',d,24)[0]
k=bytes(d[4096:4096+ks]);off=4096+a(ks);second=bytes(d[off+a(rs):off+a(rs)+ss]);c=gzip.decompress(d[off:off+rs]);i=0;out=bytearray();changed=[]
while c[i:i+6]==b'070701':
 h=[int(c[i+6+j*8:i+14+j*8],16) for j in range(13)];name=c[i+110:i+110+h[11]];n=name[:-1].decode();beg=(i+110+h[11]+3)//4*4;v=c[beg:beg+h[6]];i=(beg+h[6]+3)//4*4
 if n=='hooks-cleanup/80-oculus-recovery.sh':
  v=b'#!/bin/busybox ash\necho "Retaining recovery watchdog across switch_root for bring-up test" > /dev/kmsg\n';changed.append(n)
 if n=='init_2nd.sh':
  old=b'if ! [ "$pid" = "1" ]; then';new=b'if [ "$pid" != "1" ] && [ "$pid" != "$(cat /run/oculus-initramfs-watchdog.pid 2>/dev/null)" ]; then'
  assert old in v;v=v.replace(old,new);changed.append(n)
  marker=b'init="/sbin/init"'
  inject=b'''# Bring-up debug access rooted in the real filesystem, surviving switch_root.
cp /sbin/oculus-debug-login /sysroot/usr/bin/oculus-debug-login
mkdir -p /sysroot/var/log
if [ ! -e /sysroot/etc/inittab.before-bringup ]; then
 cp -a /sysroot/etc/inittab /sysroot/etc/inittab.before-bringup
 sed -i 's@/sbin/openrc sysinit@/sbin/openrc sysinit >>/var/log/oculus-openrc.log 2>\\&1@; s@/sbin/openrc boot@/sbin/openrc boot >>/var/log/oculus-openrc.log 2>\\&1@; s@/sbin/openrc default@/sbin/openrc default >>/var/log/oculus-openrc.log 2>\\&1@' /sysroot/etc/inittab
fi
'''
  assert marker in v;v=v.replace(marker,inject+marker)
  marker=b'exec switch_root /sysroot "$init"'
  inject=b'chroot /sysroot /bin/busybox nc -lk -s 172.16.42.1 -p 2324 -e /usr/bin/oculus-debug-login &\n'
  inject += b"chroot /sysroot /bin/busybox setsid /bin/busybox ash -c 'sleep 300; /usr/sbin/reboot-mode bootloader' >/sysroot/var/log/oculus-root-watchdog.log 2>&1 </dev/null &\n"
  v=v.replace(marker,inject+marker)
 h[6]=len(v);out+=b'070701'+b''.join(f'{x:08x}'.encode() for x in h)+name;out+=b'\0'*((-len(out))%4);out+=v;out+=b'\0'*((-len(out))%4)
 if n=='TRAILER!!!':break
assert len(changed)==2,changed
out+=b'\0'*((-len(out))%512);rd=gzip.compress(out,mtime=0);header=d[:4096];struct.pack_into('<I',header,16,len(rd))
sha=hashlib.sha1()
for content in (k,rd,second):sha.update(content);sha.update(struct.pack('<I',len(content)))
header[576:608]=sha.digest().ljust(32,b'\0')
image=header+k+b'\0'*(a(len(k))-len(k))+rd+b'\0'*(a(len(rd))-len(rd))+second+b'\0'*(a(len(second))-len(second))
p=base/'pmos-boot-4k-rootdebug-raw.img';p.write_bytes(image)
subprocess.run(['python3',str(base/'prepare-monterey-boot'),str(p),'/Volumes/vela/Backups/quest1-recovery/<SERIAL>/boot_b.img',str(base/'pmos-boot-4k-rootdebug-final.img')],check=True)
print('Patched:',changed)
