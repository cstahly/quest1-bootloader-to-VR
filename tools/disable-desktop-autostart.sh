#!/bin/sh
# Reversible removal of desktop boot links. Does not disable SSH or watchdogs.
set -eu
for link in /etc/runlevels/*/oculus-desktop-trial; do
 [ -L "$link" ] || continue
 level=${link%/*}
 level=${level##*/}
 ls -l "$link"
 rc-update del oculus-desktop-trial "$level"
done
if rc-service oculus-desktop-trial status >/dev/null 2>&1; then
 rc-service oculus-desktop-trial stop
fi
for pid in $(pidof Xorg || :); do kill -TERM "$pid"; done
