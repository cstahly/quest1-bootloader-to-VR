# 5 — Wi-Fi

## The theory that was wrong, and the one that worked

Obvious guess: WLAN firmware is in the ADSP protection domain → PIL-boot the ADSP. We
did. Still no `wlan0`. The stock manifests say why: WLAN is a **modem** protection domain
(`MODEMUW.JSN` → `wlan_pd`, service `wlan/fw`), not ADSP. Red herring, gone.

What works: don't hand-vote subsystems — run Qualcomm's own **`cnss-daemon`** (from your
stock image) in a restricted **RAM chroot**. It does the native QMI handshake, brings up
the WLAN PD, and `wlan0` + `p2p0` show up. Needs device-specific firmware + libs you pull
from your stock image; not in this repo.

## Bring it up

**1. Stage your blobs** (from stage 1 / a rooted stock read): the WLAN firmware, the
`cnss-daemon` binary, and stock `vndk-29` + `vndk-sp-29` libs. Keep them private
(`0700`; the package uses `/usr/share/oculus-wifi/firmware`).

**2. Restricted chroot.** Gotcha: `/run` is `nodev`, so you can't `mknod` inside it —
**bind-mount only** `/dev/null`, `/dev/random`, `/dev/urandom` from the host. No block
devices, no persist. (`tools/wifi/` + `prepare-cnss-root.sh` in the private workdir.)

**3. Run it:**

```
nohup chroot /run/quest-cnss-root /apex/com.android.runtime/bin/linker64 \
      /vendor/bin/cnss-daemon -n -dd &
# watch for: ICNSS FW_READY, DRIVER_PROBED  →  wlan0 + p2p0
```

**4. Associate:**

```
iw dev wlan0 scan | grep SSID
# PSK in /etc/wpa_supplicant/wpa_supplicant-wlan0.conf (0600, keep it out of git/logs)
wpa_supplicant -B -s -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant-wlan0.conf
udhcpc -i wlan0
```

**5. Automatic on boot:** install `oculus-wifi-monterey` (OpenRC service `oculus-wifi`,
`default` runlevel). Cold boot associates on its own, ~90s–2min for cnss-daemon.

## Worked when

- `iw dev wlan0 link` shows association, `ip addr show wlan0` has a lease
- `ping -I wlan0 1.1.1.1` replies, DNS resolves
- after a cold boot with **no** manual commands, `ssh root@<wlan0 IP>` works

## Snags

- **cnss-daemon exits instantly:** the `/dev` nodes — *bind-mount* null/random/urandom,
  don't `mknod` (`/run` is `nodev`). Also stage the vndk-29 libs.
- **no wlan0:** missing/mis-pathed firmware — check the `-dd` log for the firmware load.
- Don't stop/restart the daemon in place — it holds an open modem FD that's only released
  at the next recovery reboot.
- No factory MAC recovered → driver uses a locally-administered one. Fine, just not
  globally unique.

Detail: `../adsp-wifi.md` (full journey + exact chroot setup + evidence).

→ [6 — display / tracking / optics](06-headset-extensions.md)
