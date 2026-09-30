# 5 — Wi-Fi: modem WLAN domain and guarded startup

Wi-Fi scanning, WPA2 association, DHCP, router/internet access, DNS and SSH over Wi-Fi
have passed, including automatic startup after reboot. This uses owner stock assets
and native compatibility fixes, not just a single Android daemon command.

## Required pieces

WLAN belongs to the modem protection domain (`modemuw.jsn`, service `wlan/fw`).
ADSP-only boot did not create `wlan0`. The successful chain combines:

1. Matching owner modem/MBA/WLAN firmware and manifests, hash-checked privately.
2. RMTFS with Monterey relative shared-buffer offsets and read-only NV backing;
   attempted writes use RAM shadows.
3. Native protection-domain mapper and TFTP server, including MSM IPC compatibility
   and TFTP buffer/acknowledgement fixes.
4. Stock `cnss-daemon` and matching Bionic/vndk runtime in a restricted RAM root.
5. A retained modem vote and the normal WLAN driver, then supplicant and DHCP.

The canonical implementation is [runtime/oculus-wifi](../../runtime/oculus-wifi);
[package instructions](../../packages/oculus-wifi-monterey/README.md) define the
prerequisites. It mounts stock code read-only, gives the helper RAM `/data`, and
binds only null/random/urandom character devices. No writable NV/factory partitions
or block devices enter that helper root. `/run` is `nodev`, so creating device nodes
there does not substitute for the binds.

## Installation boundary

Build `oculus-wifi-monterey` and its patched dependencies using the supplied package
recipes. Device stock-runtime and IPC-policy helpers must already be installed.
Place the matching owner files plus `SHA256SUMS` under
`/usr/share/oculus-wifi/firmware`, according to the runtime's expected filenames.
Firmware extraction is not yet a complete generic tutorial command; consult the
[detailed bring-up record](../adsp-wifi.md) and inspect the runtime before staging.
Do not substitute another unit's NV/calibration or a random modem firmware set.

A private 0600 `/etc/wpa_supplicant/wpa_supplicant-wlan0.conf` supplies the network
profile. Keep credentials out of Git and console logs. On a **fresh guarded boot**,
the package requires startup before uptime 150 seconds and checks the legacy recovery
processes. The recovery service adopts their timers later. Do not bypass preflight
checks to force a late start, or start duplicate daemon instances manually.

After a successful manual packaged boot, enable the OpenRC `oculus-wifi` service in
`default`. The owner's validated boot reached association around 90–120 seconds.

## Verify

```
rc-service oculus-wifi status
cat /run/oculus-wifi/status
iw dev wlan0 link
ip addr show wlan0
ping -I wlan0 -c 2 1.1.1.1
```

Also test DNS and SSH at the assigned Wi-Fi address, then verify startup after a
normal reboot without manual intervention. Preserve USB recovery while doing so.
The service saves a stable locally administered MAC in `/etc/oculus-wifi/mac-address`;
this is not a recovered factory MAC or a guarantee of global uniqueness.

Do not stop/restart Wi-Fi in place: the modem vote is retained until reboot. Inspect
`/run/oculus-wifi` logs for firmware, RMTFS, mapper, TFTP and CNSS failures.

[Next: native VR scene](06-headset-extensions.md)
