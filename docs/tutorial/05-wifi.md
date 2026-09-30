# Stage 5 — Wi-Fi

**Goal:** `wlan0` up, associated to your network, coming back automatically on every boot.

## Concept — the theory that was wrong, and the one that worked

The obvious hypothesis was: *WLAN firmware lives in the ADSP protection domain, so PIL-boot
the ADSP.* We booted the ADSP — and still got **no `wlan0`.** Reading the stock manifests
showed why: WLAN is actually a **modem** protection domain (`MODEMUW.JSN` → `wlan_pd`,
service `wlan/fw`), not ADSP-only. That whole ADSP path was a red herring.

**What actually works:** don't hand-vote subsystems. Run Qualcomm's own **`cnss-daemon`**
(from your stock image) inside a restricted **RAM chroot**. It does the native QMI
handshake, brings up the WLAN protection domain, and `wlan0` + `p2p0` appear. This needs
device-specific firmware and libraries you extract from *your* stock image — they are not
in this repo.

## Steps

**1. Stage your own blobs** (from stage 1's stock image / a rooted stock read):
- WLAN firmware and the `cnss-daemon` binary,
- the stock `vndk-29` and `vndk-sp-29` libraries it links against.
Keep these **private**, outside git, under a `0700` dir (the packaging uses
`/usr/share/oculus-wifi/firmware`).

**2. Build the restricted chroot.** The critical gotcha: `/run` is `nodev`, so you can't
`mknod` inside it — instead **bind-mount only** `/dev/null`, `/dev/random`, `/dev/urandom`
from the host. Expose no block devices, no persist. (Reference: `tools/wifi/` and the
`prepare-cnss-root.sh` in the private working dir.)

**3. Run cnss-daemon** inside the chroot:

```
nohup chroot /run/quest-cnss-root /apex/com.android.runtime/bin/linker64 \
      /vendor/bin/cnss-daemon -n -dd &
# watch for: ICNSS FW_READY, DRIVER_PROBED  →  wlan0 + p2p0 appear
```

**4. Scan / associate** (iw + wpa_supplicant from signed Alpine APKs):

```
iw dev wlan0 scan | grep SSID
# put your PSK in /etc/wpa_supplicant/wpa_supplicant-wlan0.conf, chmod 0600
wpa_supplicant -B -s -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant-wlan0.conf
udhcpc -i wlan0
```

**5. Make it automatic.** Install the `oculus-wifi-monterey` package (OpenRC service
`oculus-wifi`, enabled in the `default` runlevel). On a cold boot it associates on its
own — no manual commands.

> 🔒 **Never** put your Wi-Fi PSK in the repo, logs, or notes. Store it only in the
> device's `0600` `wpa_supplicant` config.
>
> **Don't stop/restart the Wi-Fi daemon in place** — it holds an open modem FD that's
> only released at the next recovery reboot.

## Definition of done

- [ ] `iw dev wlan0 link` shows an association (WPA2/CCMP).
- [ ] `ip addr show wlan0` has a DHCP lease; `ping -I wlan0 1.1.1.1` gets replies; DNS
      resolves.
- [ ] After a **cold boot with no manual commands**, Wi-Fi comes up on its own (allow
      ~90 s–2 min for cnss-daemon), and `ssh root@<wlan0 IP>` works.

## When it fails

- **`cnss-daemon` exits immediately:** almost always the `/dev` nodes — you must
  *bind-mount* null/random/urandom, not `mknod` them (`/run` is `nodev`). Also confirm
  the vndk-29 libs are staged.
- **`wlan0` never appears:** you're missing firmware or staged it in the wrong path;
  check the daemon's `-dd` log for the firmware load.
- **Associated but no route:** verify the DHCP lease and that you bound pings to `wlan0`.
- **Factory MAC not recovered:** the driver falls back to a locally-administered MAC —
  fine for use, don't assume it's globally unique.

## Full detail

`../adsp-wifi.md` — the whole journey (ADSP red herring → cnss-daemon → automatic
startup verified), with the exact chroot setup and evidence logs.

→ Next: [`06-headset-extensions.md`](06-headset-extensions.md)
