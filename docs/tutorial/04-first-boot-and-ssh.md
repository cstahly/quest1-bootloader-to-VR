# 4 — First boot + SSH

The headline. On boot the initramfs maps the two Nura subpartitions out of the inner GPT
inside `system_b` (the `oculus-map-pmos-subpartitions` dm-linear mapper), `switch_root`s
into the rootfs, and starts OpenRC. USB net comes up as NCM: device `172.16.42.1`, host
`172.16.42.2`. A guarded image also drops a raw debug shell on TCP **2323** before
switch_root.

## Get in

```
# boot slot B, wait ~30s
ifconfig | grep 172.16.42          # host iface should have .2
ping -c1 172.16.42.1

nc 172.16.42.1 2323                # optional: pre-switch_root debug shell

ssh -i <your bring-up key> -o IdentitiesOnly=yes \
    -o UserKnownHostsFile=<your known_hosts> root@172.16.42.1
```

Your bring-up private key stays on your dev box — not in the repo.

## Confirm the mount is healthy (proves the 4Kn fix)

```
blockdev --getss /dev/mapper/oculus-pmos-root      # → 4096
mount | grep pmos-root                              # rw, no oops
cat /var/log/oculus-openrc.log                      # reached OpenRC default
```

## Worked when

- `ssh root@172.16.42.1` gives a shell
- `blockdev --getss` prints 4096 and mounting root didn't drop the device off the bus
- PID 1 is `init`, dropbear + USB-recovery are up

**That's a Meta-free Linux Quest 1 with a shell.** Everything past here is extension.

## Snags

- **No `172.16.42.x` iface:** macOS sometimes won't re-create USB-NCM after many
  reboots — replug, check `ifconfig` for the iface before blaming the device.
- **Shell dies right after `mount`:** 1024-block image — rebuild with `-b 4096` (stage 2).
- **Net up but no SSH:** early boots lacked `/etc/network/interfaces` and had a competing
  `unudhcpd.usb0`; the r34+ port packages fix both — confirm you built from them.
- **Back to fastboot after ~5 min:** watchdog, expected. Reboot B to keep going.

Detail: `../boot-bringup.md` "CONFIRMED FULL BOOT + SSH", Appendix A (the mapper).

→ [5 — Wi-Fi](05-wifi.md)
