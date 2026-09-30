# Quest 1 (monterey) pmOS — ADSP / Wi-Fi bring-up handoff

## Verified update — 2026-09-30 (supersedes the hypothesis below)

**ADSP boots successfully, but booting it does not produce wlan0. The original
claim that adspua.jsn hosts WLAN is incorrect.**

- Bounded test: open `/dev/subsys_adsp` on FD 3, retain FD while writing `ON` to
  `/sys/kernel/boot_wlan/boot_wlan`, wait 12 seconds, inspect state and links.
  ADSP reported `ONLINE` before and after the wait. Test exited 0, then closed
  the FD. Kernel reported ADSP brought out of reset and ADSP SSCTL connected.
  No wlan0 appeared and no ICNSS FW_READY was observed in this captured test.
- Exact running-kernel source confirms subsystem character-device open calls
  `subsystem_get_with_fwname`; release calls `subsystem_put`. The lpass missing
  base-register message is nonfatal; no device-tree fix is justified by it.
- Both installed firmware and the untouched stock modem_b backup identify
  `ADSPUA.JSN` as `msm/adsp/audio_pd`, providing `avs/audio`.
- Read-only FAT16 parsing of stock `modem_b.img` found `IMAGE/MODEMUW.JSN`:
  domain `modem`, subdomain `wlan_pd`, QMI instance 180, service `wlan/fw`.
  Exact kernel `icnss.c` defines its WLAN service name as `wlan/fw` and registers
  modem subsystem notifications. This contradicts the original ADSP-only theory;
  it does not establish a safe way to start WLAN independently of modem PIL.
- `/sys/kernel/boot_adsp/boot` exists in source (not boot_adsp/boot_adsp).
  Its loader can select modem from device-tree properties. Do not invoke that
  generic knob on an unverified assumption; the explicit ADSP character-device
  vote above was used instead.
- rmtfs stayed `-P -r`; modem was not booted, no NV writes were attempted, and
  the watchdog was left armed. No partitions were flashed during this test.
- The 300-second watchdog returned the device to fastboot. Serial
  `<SERIAL>` was verified in fastboot afterward. Guarded boot image remains
  installed; working USB SSH boot and stock restore path remain available.

Evidence: `adsp-wifi-evidence/` beside this document contains captured test logs,
exact kernel source excerpts/files, and all six stock firmware JSN manifests.
Source archive revision: `6929f734ce0e602018790ff3a52dc7bad646af60`.

Next work should be offline source/firmware investigation of the WLAN protection
 domain and the known modem-PIL wedge. Do not treat the original “modem not
required” statement as verified, and do not boot modem just to test that theory
on the user's only headset. Keep the recovery watchdog and read-only rmtfs.

---

## Original handoff (historical hypothesis; corrections above take precedence)

_For the agent picking up the Wi-Fi problem. 2026-09-30._
_Prereqs and full context: `MONTEREY-PMOS-HANDOFF.md` (boot) — read §9 there first._
_This doc is the focused Wi-Fi task; the boot problem is SOLVED._

---

## State: exactly where Wi-Fi stands

pmOS boots fully (SSH works — see the boot handoff). On the booted device:
- `echo ON > /sys/kernel/boot_wlan/boot_wlan` → **returns 0**, kernel logs
  `wlan: Loading driver v5.2.03.31F` then `wlan: driver loaded`.
- **But no `wlan0`** ever appears (`ip link` shows lo, usb0, bond0, dummy0, ifb*,
  tunnels — no wlan0).

**Why:** the WLAN driver (WCN3990, integrated) talks to firmware (**WLFW**) that runs
as a **user protection-domain on the ADSP** (`/lib/firmware/adspua.jsn`). The ADSP is
**never PIL-booted**, so WLFW never starts, so ICNSS never gets the `FW_READY` QMI
event, so no netdev is created. The WLAN driver loading is necessary but not
sufficient — ADSP must be up first.

The chain that must complete:
```
ADSP PIL boot  →  adspua.jsn user-PD loads  →  WLFW QMI service registers
              →  ICNSS receives FW_READY    →  WLAN creates wlan0
```
Everything left of the first arrow is missing. **That is the whole task: boot the ADSP.**

---

## Hard evidence (from the prior session's dmesg, `wifi-first-request.log`)

Present and working:
- `iommu: Adding device 18800000.qcom,icnss to group 1`
- `icnss 18800000.qcom,icnss: for wcss_msa0 segments only will be dumped.`
- `icnss: Platform driver probed successfully`  ← ICNSS is up and waiting
- `msm_sharedmem: sharedmem_register_qmi: qmi init successful`
- `fastrpc soc:qcom,msm-adsprpc-mem: for adsp_rh segments only will be dumped.`
- `ADSPRPC: gcinfo[0].heap_vmid 6`  ← adsprpc/fastrpc alive (a likely ADSP-boot lever)
- `spss_utils [spss_probe]: Initialization completed ok, firmware_name [spss2p].`

The ADSP PIL node and its complaint:
- `subsys-pil-tz 17300000.qcom,lpass: invalid resource`
- `subsys-pil-tz 17300000.qcom,lpass: Failed to iomap base register`
- `subsys-pil-tz 17300000.qcom,lpass: for adsp segments only will be dumped.`  (×2)

  **UNKNOWN whether this is fatal or benign.** On many MSM8998 downstream trees this
  "invalid resource / Failed to iomap base register / segments only will be dumped"
  is a *non-fatal* warning (it only disables full ramdump; PIL can still boot the
  subsystem). Determine empirically: does ADSP reach ONLINE when something votes it?

NOT needed (do not chase):
- `pil-q6v5-mss 4080000.qcom,mss: No pas_id found.`  ← the MODEM. It's the upstream
  maintainer's wedge and is **not** required for Wi-Fi. Leave it alone.
- `subsys-pil-tz cce0000.qcom,venus: Failed to locate venus.mdt(rc:-11)` ← video, separate.

Firmware all present in `/lib/firmware`: `adsp.mdt`, `adsp.b00`, `adspua.jsn`,
`wlanmdsp.mbn`, `bdwlan.*`. So ADSP PIL has its firmware locally — it likely does
**not** need rmtfs to serve it.

rmtfs currently runs `/usr/bin/rmtfs -P -r` (supervise-daemon): **-P = no PIL,
-r = read-only.** Deliberate (the port avoids PIL/modem). Revisit only if ADSP proves
to need rmtfs-served firmware or a writable share (probably it doesn't).

---

## Experiments, in order (least invasive first)

Run over SSH from the booted guarded image. **Keep the 300s watchdog armed** (do NOT
`echo … boot_wlan` then walk away; each boot self-recovers to fastboot at ~5 min).

### 1. Recon — enumerate subsystems and boot knobs (no side effects)
```
ls -l /sys/bus/msm_subsys/devices/ 2>/dev/null
for d in /sys/bus/msm_subsys/devices/subsys*; do echo "$d: $(cat $d/name) state=$(cat $d/state 2>/dev/null)"; done
ls /sys/kernel/ | grep -i boot          # is there a boot_adsp like boot_wlan?
ls -l /sys/kernel/boot_adsp/ 2>/dev/null
ls -l /dev/subsys_adsp /dev/adsprpc-smd /dev/fastrpc* 2>/dev/null
cat /sys/kernel/debug/pil/* 2>/dev/null
```
Find: (a) the ADSP subsys node + its state, (b) any `boot_adsp`/PIL sysfs, (c) the
fastrpc device nodes.

### 2. Vote the ADSP up — try these, watching dmesg after each
Most likely levers, best-guess order:
- **boot_adsp knob** (if recon found one): `echo 1 > /sys/kernel/boot_adsp/boot_adsp`
  (mirrors how `boot_wlan` is triggered).
- **fastrpc** — opening an adsprpc session usually auto-PILs the ADSP. Try a fastrpc
  test binary if present (`fastrpc_test`, `adsprpcd`), or open `/dev/adsprpc-smd`.
- **msm_subsys framework** — if the subsys node exposes a boot/restart trigger, use it;
  or in-kernel a `subsystem_get("adsp")` from any ADSP client.

After each attempt:
```
dmesg | tail -60
# WANT to see, roughly in order:
#   pil ... adsp ... loading /lib/firmware/adsp.mdt
#   subsys ... adsp: Brought out of reset  /  "adsp" subsystem online
#   ... adspua ... (user PD)  /  wlfw ... QMI server
#   icnss ... FW_READY / QMI WLFW service arrive
for d in /sys/bus/msm_subsys/devices/subsys*; do echo "$(cat $d/name)=$(cat $d/state)"; done
```

### 3. If ADSP boots but wlan0 still absent
Re-trigger WLAN after ADSP is ONLINE: `echo ON > /sys/kernel/boot_wlan/boot_wlan`,
then check `dmesg | grep -iE 'icnss|wlfw|wlan'` for the FW_READY handshake and
`ip link | grep wlan`.

### 4. If ADSP refuses to boot (fatal iomap error)
Then the `17300000.qcom,lpass` PIL node is genuinely missing its base-register
resource — a **device-tree / kernel fix** in `linux-oculus-monterey`. Compare the
lpass/adsp PIL node's `reg`/`qcom,*` properties against a known-good MSM8998 downstream
DT (e.g. other 8998 pmOS ports / the stock Quest DT). This is the deep path; only take
it after confirming §2 can't vote ADSP up.

### 5. rmtfs (probably NOT needed, test last)
Only if ADSP boot complains about missing firmware it expects rmtfs to serve: try
rmtfs writable (drop `-r`) and/or with PIL. ADSP firmware is local, so this is a
long shot; don't start here.

**Success = `wlan0` in `ip link`.** Then `iw dev`, scan, associate.

---

## Do NOT
- Boot/PIL the **modem** (mss) — not needed, it's the known wedge.
- Kill the recovery watchdog (hung the device for hours in an earlier session).
- Assume the lpass "Failed to iomap" is fatal without testing — it may be benign.

## Boot / SSH / reproduce
- Guarded image: `4k-bringup/pmos-boot-4k-rootready-final.img` (SHA256
  `621190ea2880728264953fbb559719e91a6bcbd1d3b36d7cb73262c094d675fb`) +
  `4k-bringup/pmos-system-4k-ssh.img` (SHA256
  `ed8cc6d45b11eafb188fea1ec31df9f57077064812898aea799395641c464199`).
- Flash both to slot B (fastboot), reboot; 30s pre-switch pause, then OpenRC + SSH.
- `ssh -i /Users/<user>/work/quest-pmos-bringup/id_ed25519 -o IdentitiesOnly=yes \`
  `  -o UserKnownHostsFile=/Users/<user>/work/quest-pmos-bringup/known_hosts root@172.16.42.1`
  (Mac needs 172.16.42.2 on the USB-NCM iface; DHCP from oculus-usb-recovery, else set static.)
- Stock-Android restore path is in the boot handoff §3 if anything goes sideways.
- NOTE: a clean reproducible port build now exists (device-oculus-monterey r33 folds
  in the block-size, rmtfs-alias, loopback, and unudhcpd.usb0 fixes) — you can rebuild
  a booting rootfs from source on kali rather than relying only on the captured image.

## Open questions worth asking the prior boot-session agent (optional, saves cycles)
1. **Full dmesg *after* `echo ON > boot_wlan`** (past ~140s) — the ICNSS→WLFW QMI
   timeout/retry lines. The captured log truncates right at `wlan: driver loaded`.
2. Did they find a **`boot_adsp`** knob (or try any ADSP vote via fastrpc/msm_subsys)?
3. What **state** did ADSP actually show (`/sys/bus/msm_subsys/devices/subsys*/state`) —
   ever ONLINE, or stuck, or erroring on the lpass iomap line?
