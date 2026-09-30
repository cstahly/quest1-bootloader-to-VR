import socket,threading,time,subprocess
from pathlib import Path
log=Path('/Users/<user>/work/quest-pmos-bringup/reboot-ro-kernel.log').open('wb')
s=socket.create_connection(('172.16.42.1',2323),5);s.settimeout(1);s.sendall(b'dmesg -w\n')
end=time.monotonic()+50
def read():
 while time.monotonic()<end:
  try:d=s.recv(65536)
  except socket.timeout:continue
  except OSError:break
  if not d:break
  log.write(d);log.flush()
t=threading.Thread(target=read);t.start()
c=socket.create_connection(('172.16.42.1',2323),5);c.settimeout(2)
c.sendall(b'echo RO_CHROOT_REBOOT_TEST > /dev/kmsg; sync; chroot /tmp/root4k /usr/sbin/reboot-mode bootloader\n')
for _ in range(24):
 r=subprocess.run(['/Users/<user>/Library/Application Support/QuestStack/platform-tools/37.0.1/osx-universal/fastboot','devices'],capture_output=True,text=True)
 if '<SERIAL>' in r.stdout:print('FASTBOOT_RETURNED',flush=True);break
 time.sleep(2)
s.close();c.close();t.join();log.close()
print(Path('/Users/<user>/work/quest-pmos-bringup/reboot-ro-kernel.log').read_text(errors='replace')[-7000:])
