from pathlib import Path
import subprocess
base=Path('/private/tmp/claude-501/quest-pmos')
data=bytearray((base/'pmos-boot-shell-raw.img').read_bytes())
old=data[64:576].split(b'\0')[0].decode()
cmd=old.split(' pmos_boot_uuid=')[0].rstrip()+' pmos_root=/dev/mapper/oculus-pmos-root'
assert len(cmd.encode())<512
assert 'pmos_force_initramfs' in cmd
# The image ID hashes payloads, not the command line; payloads are unchanged.
data[64:576]=cmd.encode().ljust(512,b'\0')
data[608:1632]=bytes(1024)
out=base/'pmos-boot-4k-path-raw.img';assert not out.exists();out.write_bytes(data)
print('Command line bytes:',len(cmd));print(cmd)
subprocess.run(['python3',str(base/'prepare-monterey-boot'),str(out),'/Volumes/vela/Backups/quest1-recovery/<SERIAL>/boot_b.img',str(base/'pmos-boot-4k-path-final.img')],check=True)
