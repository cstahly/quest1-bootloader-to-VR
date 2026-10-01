#!/usr/bin/env python3
"""Reject malformed live-probe inputs before opening sensors or a VIT tracker."""
import pathlib,struct,subprocess,sys,tempfile
binary=str(pathlib.Path(sys.argv[1]).resolve())
header=struct.pack('<7I',0x514d4150,1,320,240,2,0,2)
invalid=struct.pack('<IBB2x',0xffffffff,0,0)
cases={
 'short-header':(b'','map header missing'),
 'wrong-header':(bytes(28),'invalid map header'),
 'wrong-camera-count':(struct.pack('<5I',0x514d4150,1,320,240,3),'invalid map header'),
 'four-wrong-order':(struct.pack('<9I',0x514d4150,1,320,240,4,0,1,2,3),'invalid map camera order'),
 'four-short-map':(struct.pack('<9I',0x514d4150,1,320,240,4,0,2,1,3),'short map'),
 'short-map':(header,'short map'),
 'outside-pixel':(header+struct.pack('<IBB2x',76800,0,0),'invalid map entry'),
 'invalid-weight':(header+struct.pack('<IBB2x',0,32,0),'invalid map entry'),
 'edge-weight':(header+struct.pack('<IBB2x',319,1,0),'invalid map entry'),
 'extra-bytes':(header+invalid*(2*76800)+b'x','extra map bytes'),
}
with tempfile.TemporaryDirectory() as directory:
 root=pathlib.Path(directory)
 for name,(data,error) in cases.items():
  source=root/name;source.write_bytes(data);output=root/(name+'.csv')
  result=subprocess.run([binary,'must-not-open-config',str(source),'must-not-open-feed',str(output),'1'],capture_output=True,text=True,timeout=5)
  assert result.returncode==2 and error in result.stderr,(name,result)
  assert not output.exists()
  print(name,'PASS')
