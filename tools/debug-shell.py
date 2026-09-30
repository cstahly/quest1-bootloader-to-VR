#!/usr/bin/env python3
import socket,sys,time,os
s=socket.create_connection(('172.16.42.1',int(os.environ.get('QUEST_DEBUG_PORT','2323'))),timeout=5)
s.settimeout(2)
cmd=sys.argv[1] if len(sys.argv)>1 else 'uname -a'
s.sendall(('exec 2>&1; '+cmd+'; echo __COMMAND_DONE__\n').encode())
end=time.monotonic()+25
while time.monotonic()<end:
 try: data=s.recv(65536)
 except socket.timeout: continue
 if not data:break
 sys.stdout.buffer.write(data);sys.stdout.buffer.flush()
 if b'__COMMAND_DONE__' in data:break
s.close()
