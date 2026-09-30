#!/usr/bin/env python3
"""Build a larger desktop trial from the verified image; never access hardware."""
from pathlib import Path
import subprocess as s
import shutil, json, hashlib

base = Path('/home/<user>/pmos')
src = base / 'pmos-system-4k-ssh-base.img'
out = base / 'desktop-trial/pmos-system-desktop.img'
mnt = out.parent / 'root'
def run(*args, **kw):
    print('+', *map(str, args), flush=True)
    return s.run(list(map(str, args)), check=True, text=True, **kw)
def cap(*args):
    return run(*args, stdout=s.PIPE).stdout.strip()
assert hashlib.file_digest(src.open('rb'), 'sha256').hexdigest() == 'ed8cc6d45b11eafb188fea1ec31df9f57077064812898aea799395641c464199'
assert not out.exists(), 'Never overwrite a previous trial'
mnt.mkdir(parents=True, exist_ok=True)
shutil.copyfile(src, out)
table = json.loads(cap('sfdisk', '--json', out))['partitiontable']
assert table['sectorsize'] == 512 and len(table['partitions']) == 2
p = table['partitions'][1]
assert p['start'] == 999424
with out.open('r+b') as f:
    f.truncate(2400 * 1024 * 1024)
run('sgdisk', '-e', out)
run('sgdisk', '-d', '2', '-n', f'2:{p["start"]}:0', '-t', f'2:{p["type"]}',
    '-u', f'2:{p["uuid"]}', '-c', f'2:{p.get("name", "pmOS_root")}', out)
table2 = json.loads(cap('sfdisk', '--json', out))['partitiontable']
root = table2['partitions'][1]
# GPT's last usable LBA need not be 4K-aligned; trim the last partition to 4K.
size = root['size'] // 8 * 8
run('sgdisk', '-d', '2', '-n', f'2:{p["start"]}:{p["start"] + size - 1}', '-t', f'2:{p["type"]}', '-u', f'2:{p["uuid"]}', '-c', f'2:{p.get("name", "pmOS_root")}', out)
assert table2['partitions'][0] == table['partitions'][0]
loop = cap('losetup', '--find', '--show', '--sector-size', '4096',
           '--offset', p['start'] * 512, '--sizelimit', size * 512, out)
mounted = False
try:
    run('e2fsck', '-f', '-p', loop)
    run('resize2fs', loop)
    run('mount', loop, mnt); mounted = True
    shutil.copy2(base / 'work/chroot_rootfs_oculus-monterey/usr/bin/qemu-aarch64-static', mnt / 'usr/bin/qemu-aarch64-static')
    resolv = mnt / 'etc/resolv.conf'
    old_resolv = resolv.read_bytes()
    resolv.write_text('nameserver 1.1.1.1\n')
    # Limit package resolution to public repositories; installed device packages remain intact.
    repos = mnt / 'etc/apk/repositories'
    old_repos = repos.read_bytes()
    repos.write_text('http://mirror.postmarketos.org/postmarketos/main\nhttps://mirrors.edge.kernel.org/alpine/edge/main\nhttps://mirrors.edge.kernel.org/alpine/edge/community\n')
    try:
        run('chroot', mnt, '/sbin/apk', 'add', 'labwc', 'xorg-server', 'xf86-video-fbdev', 'xinit', 'dbus', 'font-dejavu', 'xterm', 'setpriv')
    finally:
        resolv.write_bytes(old_resolv); repos.write_bytes(old_repos)
    pkg = base / 'pmaports/device/testing/postmarketos-ui-oculus-labwc'
    files = {'oculus-labwc-launch':'usr/sbin/oculus-labwc-launch',
             'oculus-labwc-session':'usr/bin/oculus-labwc-session',
             'oculus-labwc-xinit':'usr/libexec/oculus-labwc-xinit',
             'xorg-oculus-fbdev.conf':'etc/X11/xorg.conf.d/20-oculus-fbdev.conf',
             'Xwrapper.config':'etc/X11/Xwrapper.config',
             'labwc-environment':'etc/xdg/oculus-labwc/environment',
             'labwc-rc.xml':'etc/xdg/oculus-labwc/rc.xml'}
    for source, target in files.items():
        dest = mnt / target; dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(pkg/source, dest)
    auto = mnt/'etc/xdg/oculus-labwc/autostart'
    auto.write_text('#!/bin/sh\nxterm -title "Quest desktop trial" &\n')
    auto.chmod(0o755)
    for name in ['usr/sbin/oculus-labwc-launch','usr/bin/oculus-labwc-session','usr/libexec/oculus-labwc-xinit']:
        (mnt/name).chmod(0o755)
    # Trial desktop is started manually over SSH; do not add boot dependencies.
    print((mnt/'etc/passwd').read_text(), flush=True)
    run('chroot', mnt, '/sbin/apk', 'info', '-e', 'labwc', 'xorg-server', 'xf86-video-fbdev')
    (mnt/'usr/bin/qemu-aarch64-static').unlink()
    run('sync', '-f', mnt)
    run('umount', mnt); mounted = False
    run('e2fsck', '-fn', loop)
    run('dumpe2fs', '-h', loop)
finally:
    if mounted: run('umount', mnt)
    run('losetup', '-d', loop)
run('sgdisk', '-v', out)
run('sha256sum', out)
