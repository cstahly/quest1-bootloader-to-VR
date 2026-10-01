#!/usr/bin/env python3
"""Set Monterey's short root-device cmdline and finalize an unsigned v0 boot image.
Does not edit the ramdisk, remove recovery guards, or flash hardware.
"""
import argparse
from pathlib import Path
import runpy
import tempfile


def prepare(source, template, output):
    helper = runpy.run_path(str(Path(__file__).with_name('prepare-monterey-boot')))
    data = bytearray(source.read_bytes())
    helper['boot_layout'](data, 'input')
    primary = bytes(data[64:576]).split(b'\0', 1)[0]
    extra = bytes(data[608:1632]).split(b'\0', 1)[0]
    cmdline = (primary + extra).decode('ascii')
    tokens = [t for t in cmdline.split() if not t.startswith(
        ('pmos_boot_uuid=', 'pmos_root_uuid=', 'pmos_root='))]
    if 'pmos_force_initramfs' not in tokens:
        raise ValueError('Input must retain pmos_force_initramfs')
    tokens.append('pmos_root=/dev/mapper/oculus-pmos-root')
    encoded = ' '.join(tokens).encode('ascii')
    if len(encoded) >= 512:
        raise ValueError('Required cmdline does not fit in first 512 bytes')
    data[64:576] = encoded.ljust(512, b'\0')
    data[608:1632] = bytes(1024)
    if output.exists():
        raise FileExistsError('Refusing to overwrite output')
    with tempfile.TemporaryDirectory(prefix='monterey-boot-') as directory:
        raw = Path(directory)/'unsigned.img'
        raw.write_bytes(data)
        helper['finalize'](raw, template, output)
    print('cmdline_bytes='+str(len(encoded)))
    print('cmdline='+encoded.decode())


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('input', type=Path)
    p.add_argument('template', type=Path)
    p.add_argument('output', type=Path)
    args = p.parse_args()
    try:
        prepare(args.input, args.template, args.output)
    except (ValueError, OSError) as error:
        p.error(str(error))
