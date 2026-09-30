# Stage 4 — First boot and SSH

**Goal:** the headline result — Nura boots through `switch_root` into OpenRC, and you get
a root shell over USB.

## Concept

On boot, the initramfs maps the two Nura subpartitions out of the inner GPT inside
`system_b` (the `oculus-map-pmos-subpartitions` dm-linear mapper), then `switch_root`s
into the real rootfs and starts OpenRC. USB networking comes up as an NCM interface; the
device is `172.16.42.1`, your host gets `172.16.42.2`. A guarded bring-up image also
exposes a raw debug shell on TCP **2323** before switch_root.

## Steps

**1. Boot slot B and wait ~30 s.**

**2. Bring up USB networking on your control host** and confirm the interface exists:

```
ifconfig | grep 172.16.42        # the NCM iface should have 172.16.42.2
ping -c1 172.16.42.1
```

**3. (Optional) Pre-switch debug shell**, if using a guarded image:

```
nc 172.16.42.1 2323              # raw shell before switch_root; diagnose here if needed
```

**4. SSH in as root** (use your bring-up key; the private key lives only on your dev
machine and is **never** committed):

```
ssh -i <your bring-up key> -o IdentitiesOnly=yes \
    -o UserKnownHostsFile=<your known_hosts> root@172.16.42.1
```

**5. Confirm the mount is healthy** (this is the proof the 4Kn fix worked):

```
blockdev --getss /dev/mapper/oculus-pmos-root      # → 4096
mount | grep pmos-root                              # mounted rw, no oops
cat /var/log/oculus-openrc.log                      # reached OpenRC default runlevel
```

## Definition of done

- [ ] `ssh root@172.16.42.1` gives a root shell.
- [ ] `blockdev --getss /dev/mapper/oculus-pmos-root` prints **4096**.
- [ ] The device stays on the bus (no drop-off) once mounted — i.e. mounting root did
      **not** oops the kernel.
- [ ] PID 1 is `init`; dropbear + USB-recovery services are up.

**That's the milestone: a Meta-free Linux Quest 1 with a shell.** Everything after this
is extension.

## When it fails

- **No `172.16.42.x` interface:** macOS sometimes won't re-create the USB-NCM iface after
  many reboots — replug, or check `ifconfig` for the interface itself before blaming the
  device.
- **Shell dies right after `mount`:** that's the 1024-block bug — the image wasn't built
  with `-b 4096`. Go back to stage 2 and re-verify `dumpe2fs`.
- **Networking up but no SSH:** early boots failed because `/etc/network/interfaces`
  didn't exist and dropbear was blocked; the port packages now install a loopback
  `interfaces` file and remove the competing `unudhcpd.usb0` service. Confirm you built
  from the r34+ port packages.
- **Device returns to fastboot after ~5 min:** the recovery watchdog — expected on a
  guarded image. Reboot B to continue; **don't** disable the watchdog to stop it.

## Full detail

`../boot-bringup.md` "CONFIRMED FULL BOOT + SSH" (the exact known-good image, its SHA256,
and the root-side fixes) and Appendix A (the subpartition mapper).

→ Next: [`05-wifi.md`](05-wifi.md)
