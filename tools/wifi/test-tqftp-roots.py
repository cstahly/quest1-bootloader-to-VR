#!/usr/bin/env python3
"""Check the test TFTP translator cannot write firmware or escape its RAM root."""
import ctypes
import os
from pathlib import Path
import subprocess
import tempfile
source = Path(__file__).with_name('tqftp-translate-monterey.c')
with tempfile.TemporaryDirectory(prefix='quest-tftp-test-') as temp:
    root = Path(temp)
    for name in ('firmware', 'fallback', 'writable', 'outside'):
        (root / name).mkdir()
    (root / 'firmware' / 'test').write_bytes(b'firmware')
    (root / 'fallback' / 'other').write_bytes(b'fallback')
    (root / 'outside' / 'secret').write_bytes(b'outside')
    (root / 'writable' / 'link').symlink_to(root / 'outside', target_is_directory=True)
    (root / 'writable' / 'alias').symlink_to(root / 'outside' / 'secret')
    library = root / 'translator.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-Wall', '-Wextra', '-Werror',
                    f'-DQUEST_TFTP_FIRMWARE="{root / "firmware"}"',
                    f'-DQUEST_TFTP_FALLBACK="{root / "fallback"}"',
                    f'-DQUEST_TFTP_WRITABLE="{root / "writable"}"',
                    str(source), '-o', str(library)], check=True)
    lib = ctypes.CDLL(str(library), use_errno=True)
    lib.translate_open.argtypes = [ctypes.c_char_p, ctypes.c_int]
    lib.translate_open.restype = ctypes.c_int
    def call(path, flags): return lib.translate_open(path.encode(), flags)
    for name, content in [('test', b'firmware'), ('other', b'fallback')]:
        fd = call('/readonly/firmware/image/' + name, os.O_RDONLY)
        assert fd >= 0
        assert os.read(fd, 100) == content
        os.close(fd)
    flags = os.O_WRONLY | os.O_CREAT
    for path in ['/readonly/firmware/image/test', '/readwrite//tmp/escape',
                 '/readwrite/../outside/secret', '/readwrite/link/secret',
                 '/readwrite/alias', '/readwrite/./bad', '/readwrite/a//bad']:
        assert call(path, flags) == -1, path
    fd = call('/readwrite/nested/ok', flags)
    assert fd >= 0
    os.write(fd, b'temporary'); os.close(fd)
    assert (root / 'writable' / 'nested' / 'ok').read_bytes() == b'temporary'
    assert (root / 'firmware' / 'test').read_bytes() == b'firmware'
    assert (root / 'outside' / 'secret').read_bytes() == b'outside'
    print('PASS: reads, fallback, nested RAM writes, firmware protection, traversal and symlink rejection')
