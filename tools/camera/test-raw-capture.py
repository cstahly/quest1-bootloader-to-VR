#!/usr/bin/env python3
"""C producer/Python parser interoperability and malformed synthetic fixtures."""
import importlib.util
import os
from pathlib import Path
import struct
import subprocess
import tempfile

here = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('raw_capture', here/'read-raw-capture.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    binary, fixture = root/'producer', root/'synthetic.bin'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',
                    str(here/'test-raw-capture.c'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(fixture)],check=True)
    data = fixture.read_bytes()
    parsed = module.parse(data)
    assert parsed['skew_ns'] == 300000 and parsed['requested_ns'] == 1000000000
    assert [c['gain_q4'] for c in parsed['cameras']] == [16,32,48,64]
    assert all(c['exposure_us'] == 11628 and c['requested_us'] == 12000 for c in parsed['cameras'])
    def reject(name, modified):
        try:
            module.parse(modified)
        except ValueError:
            print(name, 'PASS')
        else:
            raise AssertionError(name+' was accepted')
    reject('truncated',data[:-1]);reject('extra-bytes',data+b'x')
    for name, offset, fmt, value in (
        ('version',4,'B',ord('9')), ('dimensions',16,'I',320),
        ('unknown-flags',36,'I',8), ('reserved-header',56,'Q',1),
        ('stale-before-request',72,'Q',1000000000),
        ('future-after-capture',72,'Q',1200000000),
        ('wrong-camera-order',64,'I',2), ('zero-sequence',68,'I',0),
        ('wrong-payload-offset',88,'Q',257), ('wrong-payload-size',96,'I',307839),
        ('reserved-camera',108,'I',1), ('controller-short-frame',80,'I',500),
        ('gain-mismatch',84,'I',99), ('skew',72,'Q',1020000000),
    ):
        changed = bytearray(data)
        struct.pack_into('<'+fmt,changed,offset,value)
        reject(name,changed)
    output = root/'images'
    subprocess.run([os.sys.executable,str(here/'read-raw-capture.py'),str(fixture),
                    '--extract',str(output)],check=True,stdout=subprocess.DEVNULL)
    assert output.stat().st_mode & 0o777 == 0o700
    for c in range(4):
        image = output/f'camera{c}.pgm'
        assert image.stat().st_mode & 0o777 == 0o600
        assert image.read_bytes() == b'P5\n640 480\n255\n'+bytes([c+1])*307200
    print('independent parser, physical order, exact pixels and private extraction PASS')
