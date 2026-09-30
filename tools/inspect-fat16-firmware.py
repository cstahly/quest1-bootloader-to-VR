#!/usr/bin/env python3
"""List firmware entries in a stock FAT16 backup without mounting or modifying it."""
import argparse
import hashlib
import re
import struct
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('image', type=Path)
parser.add_argument('--verify-modem', action='store_true', help='Validate split ELF segment lengths without extracting or loading firmware')
parser.add_argument('--all', action='store_true', help='Inventory all short-name files, not just modem/WLAN firmware')
args = parser.parse_args()
data = args.image.read_bytes()
if len(data) < 512:
    raise SystemExit("Image is shorter than a filesystem boot sector")
def u16(offset): return struct.unpack_from('<H', data, offset)[0]
bps, spc, reserved, fats, roots, spf = u16(11), data[13], u16(14), data[16], u16(17), u16(22)
if len(data) < 512 or data[510:512] != b'\x55\xaa' or bps not in (512,1024,2048,4096) or not spc or spc & (spc-1) or not roots or not spf:
    raise SystemExit('Expected a FAT16 filesystem starting at byte zero')
root_offset = (reserved + fats * spf) * bps
root_bytes = roots * 32
cluster_base = root_offset + ((root_bytes + bps - 1) // bps) * bps
cluster_bytes = bps * spc
fat_offset = reserved * bps
total_sectors = u16(19) or struct.unpack_from('<I', data, 32)[0]
volume_bytes = total_sectors * bps
cluster_count = (volume_bytes - cluster_base) // cluster_bytes
if not reserved or not fats or volume_bytes > len(data) or not 4085 <= cluster_count < 65525:
    raise SystemExit('Invalid or non-FAT16 volume geometry')
max_cluster = min(spf*bps//2, (len(data)-cluster_base)//cluster_bytes+2)
firmware = {}
def chain(cluster):
    chunks, seen = [], set()
    while cluster < 0xfff8:
        if not 2 <= cluster < max_cluster or cluster in seen:
            raise ValueError('Invalid or cyclic FAT chain')
        seen.add(cluster)
        start = cluster_base + (cluster - 2) * cluster_bytes
        chunks.append(data[start:start+cluster_bytes])
        cluster = u16(fat_offset + 2*cluster)
    return b''.join(chunks)
def walk(raw, prefix='', visited=None):
    visited = set() if visited is None else visited
    for offset in range(0,len(raw),32):
        entry = raw[offset:offset+32]
        if len(entry)!=32 or not entry[0]: break
        if entry[0] == 0xe5 or entry[11] == 15 or entry[11]&8: continue
        base, ext = entry[:8].decode('ascii').rstrip(), entry[8:11].decode('ascii').rstrip()
        if base in ('.','..'): continue
        name = base + ('.'+ext if ext else '')
        cluster, size = struct.unpack_from('<H',entry,26)[0], struct.unpack_from('<I',entry,28)[0]
        path = prefix+name
        if entry[11]&16:
            if cluster in visited: raise ValueError('Cyclic directory')
            visited.add(cluster);walk(chain(cluster),path+'/',visited)
        elif args.all or re.search(r'(MODEM|MBA|WLAN)',name):
            contents = chain(cluster)[:size] if size else b''
            if len(contents)!=size: raise ValueError('Truncated file')
            firmware[path] = contents
            print(f'{path}\t{size}\t{hashlib.sha256(contents).hexdigest()}')
walk(data[root_offset:root_offset+root_bytes])
if args.verify_modem:
    mdt = firmware['IMAGE/MODEM.MDT']
    if mdt[:6] != b'\x7fELF\x01\x01' or len(mdt) < 52:
        raise SystemExit('Expected little-endian ELF32 modem metadata')
    phoff = struct.unpack_from('<I', mdt, 28)[0]
    phentsize, phnum = struct.unpack_from('<HH', mdt, 42)
    if phentsize != 32 or phoff + phnum * phentsize > len(mdt):
        raise SystemExit('Invalid modem program-header table')
    spans = []
    for i in range(phnum):
        typ, off, va, pa, filesz, memsz, flags, align = struct.unpack_from('<8I', mdt, phoff+i*32)
        # Qualcomm hash-table segments are metadata, not loadable payload.
        if typ != 1 or ((flags >> 24) & 7) == 2:
            continue
        if filesz > memsz:
            raise SystemExit(f'Segment {i}: file larger than memory')
        if filesz:
            name = f'IMAGE/MODEM.B{i:02d}'
            payload = firmware.get(name)
            if payload is None or len(payload) != filesz:
                raise SystemExit(f'Segment {i}: missing or wrong-sized {name}')
        spans.append((pa, pa+memsz))
        print(f'SEGMENT {i:02d}: address=0x{pa:08x} file={filesz} memory={memsz} flags=0x{flags:08x}')
    if not spans or not firmware.get('IMAGE/MBA.MBN'):
        raise SystemExit('Missing modem payload or MBA')
    lo, hi = min(s[0] for s in spans), max(s[1] for s in spans)
    print(f'PASS: {len(spans)} loadable segments complete; address span 0x{lo:08x}..0x{hi:08x} ({hi-lo} bytes).')
    print('This verifies completeness only, not signatures, device compatibility, or safe PIL startup.')
