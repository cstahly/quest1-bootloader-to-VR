#!/usr/bin/env python3
"""Offline synthetic v0 layout / footer / short-cmdline regression checks."""
from pathlib import Path
import runpy, struct, tempfile
h=runpy.run_path(str(Path(__file__).with_name('prepare-monterey-boot')))
p=runpy.run_path(str(Path(__file__).with_name('prepare-4k-boot.py')))
with tempfile.TemporaryDirectory() as d:
 d=Path(d); raw=bytearray(12288);raw[:8]=b'ANDROID!'
 struct.pack_into('<I',raw,8,16);struct.pack_into('<I',raw,16,16)
 struct.pack_into('<I',raw,36,4096)
 cmd=b'keep=1 pmos_force_initramfs pmos_boot_uuid=old pmos_root_uuid=old keep=2'
 raw[64:64+len(cmd)]=cmd
 template=bytearray(raw);struct.pack_into('<I',template,28,0xf00000)
 footer=bytearray(4096);footer[:2]=b'\x30\x82';footer[20:29]=h['BOOT_TARGET_DER']
 template+=footer
 (d/'raw').write_bytes(raw);(d/'template').write_bytes(template)
 p['prepare'](d/'raw',d/'template',d/'out')
 out=(d/'out').read_bytes();cmd=out[64:576].split(b'\0')[0]
 assert b'keep=1' in cmd and b'keep=2' in cmd and b'uuid=' not in cmd
 assert b'pmos_root=/dev/mapper/oculus-pmos-root' in cmd
 assert out[608:1632]==bytes(1024) and out[4096:12288]==raw[4096:12288]
 assert struct.unpack_from('>I',out,12288+29)[0]==12288
 try:p['prepare'](d/'raw',d/'template',d/'out')
 except FileExistsError:pass
 else:raise AssertionError('overwrite accepted')
 for offset,value in [(36,2048),(40,3),(8,1000000)]:
  bad=bytearray(raw);struct.pack_into('<I',bad,offset,value)
  try:h['boot_layout'](bad,'test')
  except h['ImageError']:pass
  else:raise AssertionError('invalid layout accepted')
 print('PASS: payload preserved, cmdline bounded, footer length, no overwrite, invalid layout rejection')
