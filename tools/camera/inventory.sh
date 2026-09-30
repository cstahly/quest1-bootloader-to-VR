#!/bin/sh
# Read-only inventory. Never loads firmware, probes I2C, or writes camera controls.
set -eu
printf 'UPTIME\n'; cat /proc/uptime
printf '\nGUARD\n'; /usr/sbin/oculus-recovery-guard --check
cat /run/oculus-recovery/status
printf '\nVIDEO SYSFS\n'
for node in /sys/class/video4linux/*; do
 [ -d "$node" ] || continue
 printf '%s\n' "$node"
 for field in name dev; do [ ! -r "$node/$field" ] || cat "$node/$field"; done
 readlink -f "$node/device" || true
done
printf '\nMEDIA NODES\n'
ls -l /dev/video* /dev/media* /dev/v4l-subdev* /dev/msm_camera* 2>/dev/null || true
printf '\nCAMERA KERNEL MESSAGES\n'
dmesg | grep -iE 'camera|csid|csiphy|sensor|vfe|ispif' | tail -100 || true
printf '\nSTOCK CAMERA LIBRARY NAMES\n'
for dir in /run/oculus-stock-runtime/system/vendor/bin /run/oculus-stock-runtime/system/vendor/lib64 /run/oculus-stock-runtime/system/vendor/lib /run/oculus-stock-runtime/system/vendor/etc; do
 [ ! -d "$dir" ] || find "$dir" -maxdepth 3 -type f | grep -iE 'camera|sensor|tracking|qvr|slam|ov[0-9]|ar0[0-9]' || true
done
printf '\nSYNCBOSS CAMERA CAPABILITIES\n'
for field in /sys/bus/spi/devices/*/*camera*; do
 [ ! -f "$field" ] || { printf '%s\n' "$field"; cat "$field" 2>/dev/null || true; }
done
