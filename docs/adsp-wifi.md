# Final shutdown after USB unplug — 2026-09-30

User confirmed cutting USB. Verified over Wi-Fi at uptime394s that
/sys/class/power_supply/usb/online=0; guard active, renewal count1 in charger boot.
Issued sync and oculus-recovery-guard --poweroff over Wi-Fi successfully. Subsequent
SSH connection timed out, consistent with shutdown. Do not boot the headset again.
Autostart scene, HUD, Wi-Fi and real trigger renewals were verified before shutdown.
User's complaint about the missing “DJ version” meant the CHAT/conversation, not
an alternate renderer. Do not roll back scene files based on that phrase.

# Shutdown observation: USB charger boot — 2026-09-30

Explicit poweroff completed, then the connected USB supply caused a new boot with
androidboot.mode=charger. Confirmed cmdline atuptime79s; pstore empty. This is a
charging-mode reboot, not evidence of a kernel crash. pmOS currently treats that
boot like normal startup, so Wi-Fi and test scene start again. Both autostarted
again successfully, including latest HUD label-only fix. Do not repeatedly power
cycle while the cable remains attached.

Asked user to unplug USB and reply unplugged, then planned to invoke the new
/usr/sbin/oculus-recovery-guard --poweroff over Wi-Fi<HEADSET_WIFI_IP>. WAIT for reply;
do not claim it is fully off yet. USB cable must be removed for this shutdown test.
SSH over Wi-Fi verified; use HostKeyAlias172.16.42.1 with W/known_hosts and existing key.
User wanted power-off at stopping point and Power to return to scene in morning.
Handling charger-only boot as a separate low-power mode remains future work.

Canonical scene/guard details: /Volumes/vela/src/quest1-nura-port/docs/TEST-SCENE-AUTOSTART.md

# Wi-Fi automatic startup VERIFIED — 2026-09-30 ~06:09 EDT

Installed signed oculus-wifi-monterey 0.1-r2 and verified both manual service startup
on a fresh boot and a subsequent boot with OpenRC default startup. The second test
used no manual service, wpa_supplicant, or DHCP startup commands. At uptime88s:
WPA2/CCMP association complete on 5GHz, DHCP address<HEADSET_WIFI_IP>/22, gateway<LAN_GATEWAY_IP>.
Router and internet (1.1.1.1) each3/3 ping replies, zero loss, explicitly bound wlan0;
DNS example.com resolves. SSH directly to the Wi-Fi address also passed at uptime113s.
Modem ONLINE, hardware watchdog disable=0. Stable locally administered MAC preserved.
Private network profile remains0600; no password/PSK saved in repo or these notes.

Evidence (private working directory /Users/<user>/work/quest-pmos-bringup):
- wifi-source/wifi-r2-coldboot-autostart.log
- wifi-source/wifi-r2-lan-ssh.log
- wifi-source/oculus-wifi-monterey-0.1-r2.apk (signed build)

OpenRC: oculus-wifi enabled in default; dependency cache refreshed. Existing real vs
virtual swap warning is unrelated. r2 removes unsupported wpa_supplicant -f flag.
Canonical runtime and packaged copies match; local APKBUILD source SHA512 checks,
sh syntax and git diff --check pass. Package build/shared-memory tests passed.
Device package revision35 IPC helpers were installed manually; its APK was NOT
rebuilt/reinstalled. APKBUILD source checksum ordering has been corrected locally.

IMPORTANT: existing 300s recovery timer remains unchanged and will return this boot
to fastboot. Wi-Fi works automatically during pmOS boots; this is NOT yet an
unattended persistent everyday OS session. Do not remove guards just to keep Wi-Fi
up. Do not restart/stop the Wi-Fi daemon in place: its open modem FD is retained
until recovery reboot. No desktop enabled; no modem/boot-chain/NV/factory partitions
flashed. RMTFS backing writes remain RAM-only; owner firmware stays private.

Hourly continuation automation continue-quest-wi-fi-bring-up is PAUSED because its
success condition was met. Do not keep rebooting/testing automatically. User can
resume native VR/OS integration next. Source changes remain uncommitted for the
requested separate sorting/commit agent; preserve unrelated renderer/Monado work.

# Connected Wi-Fi — 2026-09-30 ~05:58

User supplied the target network/credential privately. DO NOT copy its password or
PSK into repo, logs, notes, or handoffs. Root-only profile stored on device at
/etc/wpa_supplicant/wpa_supplicant-wlan0.conf (0600), temporary host copy removed.
Connected to the configured network at5240MHz (802.11ac, WPA2-PSK/CCMP), DHCP lease obtained.
Router and internet pings each3/3 replies with0% loss, DNS works.
Evidence W/wifi-source/wifi-first-connectivity.log and wpa-first-status.log (no keys).
Modem remains ONLINE. Recovery watchdog unchanged; desktop disabled.

Signed package oculus-wifi-monterey0.1-r1 installed. Owner firmware installed under
/usr/share/oculus-wifi/firmware, chmod700 directory/go-rwx files; helper stages /run.
Package runtime initial Wi-Fi initialization succeeded, but Alpine wpa_supplicant
omits -f logging flag. Manually starting wpa without-f then DHCP produced success.
Source fixed to -B -s with startup output redirection; package revision2 rebuilding.
Manual network status file from r1 can remain wifi-authenticating despite actual
working connection; use wpa_cli/ip to verify. Next boot with r2 fixes startup.
Saved local MAC at/etc/oculus-wifi/mac-address is used instead of driver fallback.

NEXT: install signedr2, fresh boot/manual rc-service oculus-wifi start before150s,
verify automatic association/DHCP, enable default init only after that passes,
then coldboot verify automatic startup. Keep recovery watchdog. No firmware/NV flash.
Hourly continuation automation continue-quest-wi-fi-bring-up active; stop when
Wi-Fi startup and connection are verified, as its prompt directs.

# Wi-Fi radio works — 2026-09-30 ~05:49

wlan0 and p2p0 appeared after stock cnss-daemon ran in the restricted RAM chroot.
ICNSS FW_READY at140.611, DRIVER_PROBED. iw scan succeeded: 11 BSS entries across
2412/2437/5240/5500MHz. Evidence W/wifi-source/first-wifi-scan.log (private SSIDs),
cnss-relative-kmsg-live.log and cnss-relative-status.log. iw+wpa_supplicant installed
from signed Alpine APKs transferred via Kali (only four new packages, no upgrades).
Association/internet NOT tested: no preexisting network profile found yet.

cnss chroot problem solved: /run is nodev, so mknod /dev/null inside it is unusable.
Bind ONLY /dev/null,/dev/random,/dev/urandom from host to corresponding root nodes.
No block devices/persist exposed. Also include stock lib64/vndk-29 and vndk-sp-29.
After that, cnss-daemon -n -dd stays running, talks native IPC and initializes WLAN.
W/wifi-source/prepare-cnss-root.sh contains fixed setup; it currently runs help at
end (exit1 on expected help). Runtime start: env LD_PRELOAD= LD_LIBRARY_PATH with
chroot paths, nohup chroot /run/quest-cnss-root /apex/com.android.runtime/bin/linker64
/vendor/bin/cnss-daemon -n -dd. Data/vendor/wifi and calibration outputs remain RAM.
Read-only persist mount /run/quest-persist-ro has no WLAN/MAC filename matches;
factory MAC currently NOT recovered, driver uses fallback 00:0a:f5:16:ce:65.
Do not assume fallback MAC is unique for production use.

NEXT: package/repeat startup on a fresh boot, preserve watchdog. Stage firmware
from owner backup in persistent pmOS rootfs data if needed; never flash modem/NV
partitions. Build source in repo; owner firmware/binaries stay outside Git.
Keep -r shadows and relative offsets, IRSC init, mapper, TFTP, CNSS before modem vote.
No association claims without actual association. User is offline; continue work.

# Quest 1 (monterey) pmOS — ADSP / Wi-Fi bring-up handoff

# Breakthrough checkpoint — 2026-09-30 ~05:44

USER OFFLINE: explicitly authorized continued work overnight toward Wi-Fi.
Heartbeat automation continue-quest-wi-fi-bring-up active hourly in this task.

Early modem crash cause FOUND AND FIXED in a RAM-only diagnostic build:
RMTFS requests use shared-buffer-relative offsets (first read offset0x200), but
upstream rmtfs treats them as absolute physical addresses. Crash-persistent logs
show failed sector1 read immediately before fatal. New explicit opt-in
RMTFS_MSM8998_RELATIVE_OFFSETS=1 translates bounded offsets into the UIO buffer
0xfca00000/2MiB. Preserves -P -r; diagnostic binary rejects missing -r and -s.
Patches in patches/rmtfs; unit test relative/absolute bounds/overflow passes;
on-device metadata and relative sector reads pass for all 4 standard NV areas.
No persistent NV writes. Unknown OEM_1/OEM_2 requests are optional so far.

With fix + native mapper + fixed TFTP, MODEM STAYS ONLINE (uptime120..311),
TFTP served the entire 3184968-byte wlanmdsp.mbn, WLAN PD up, ICNSS QMI connected.
ICNSS indication/MSA/cap exchanges succeed but no FW_READY event and no wlan0 yet.
Watchdog remains unchanged; latest boot about to return to fastboot normally.
Evidence W/wifi-source/relative-modem-kmsg-live.log, relative-test-prepare.log,
relative-wlan-status.log. Previous crash evidence kmsg-storage-diagnostic-live.log
and kmsg-storage-diagnostic-pstore.tar (RMTFS failed read captured in kernel log).

Current next step: stock cnss-daemon likely supplies board/calibration data.
Stock helper requires system/lib64/vndk-29 and vndk-sp-29 in LD_LIBRARY_PATH;
outside chroot -h prints help (cnss-help-vndk.log). A restricted /run/quest-cnss-root
was constructed: stock system bind read-only, vendor bin/lib symlinks, firmware
bdwlan files copied, data/vendor/wifi RAM-only, dev only null/random/urandom,
proc read-only. NO persist/block devices exposed. Chroot linker --help works,
but cnss-daemon help/run exits with no output; diagnose with strace next.
W/wifi-source/prepare-cnss-root.sh updated vndk paths. strace downloaded from Kali.
Do not run unrestricted stock cnss-daemon against persistent data paths.

Latest safe startup archive W/wifi-source/relative-test-stage.tar contains all
firmware/helpers (NOT Git), prepare-relative-modem-test.sh (does not vote).
It stages services, stops installed rmtfs, starts RAM rmtfs-debug with relative
flag, runs local tests. Then guarded-debug-modem-vote.sh manually before uptime150.
Discover IPC ports with ANCHORED service regex /^0x... /, not matching instance/port.
Stock WLAN boot sysfs ignores input contents, ON and1 equivalent (source checked).
All source work uncommitted. Need persist/package fixes only after validation.


## Guarded TFTP modem test — 2026-09-30 ~05:28

Fresh boot: IPC policy ready; firmware SHA256 checks all passed; TFTP protocol
suite passed; mapper answered wlan/fw -> msm/modem/wlan_pd instance180; WLAN
boot trigger returned success. rmtfs was running /usr/bin/rmtfs -P -r -v.
Modem vote began at uptime108.620; MBA boot at108.692; modem out of reset108.843;
QMI domain180108.892 and SSCTL108.893; fatal108.949 (same unknown SMEM reason).
Thus TFTP compatibility is fixed but does NOT solve the early modem crash.
No wlan0 demonstrated. Recovery VERIFIED: SSH returned on normal kernel 4.4.205-perf; IPC marker
present, rmtfs service started, modem OFFLINING. Saved tftp-modem-pstore.tar
and extracted pstore; panic at109.050 confirms modem crash. No repeat vote planned
without a new concrete cause. Guarded image/recovery watchdog unchanged.

Evidence W/wifi-source/tftp-modem-final-prepare.log, tftp-final-kmsg-live.log,
tftp-final-services-live.log. Service tail used 1s polling, so absence of logged
modem requests is NOT proof none occurred in the ~0.1s before fatal. The attempted
foreground SSH-PTY rmtfs session ended immediately; nohup replacement was verified
running before vote, but its transient /run log was not continuously streamed.
A future diagnostic should flush/capture rmtfs and TFTP events synchronously.
All staging stayed in tmpfs; all backing NV reads retained -r, no firmware flash.


## TFTP compatibility checkpoint — 2026-09-30 ~05:26

On-device protocol tests now pass: fixture read, RAM-only write, read-only write
rejection, 1500-byte multi-block read and exact 1024-byte boundary (empty final
block). Evidence: W/wifi-source/tftp-protocol-fixed.log. Modem stayed OFFLINING.
Stock libqipcrtr4msmipc v0.1 translates sendto/recvfrom, but not connect. Original
server received RRQ then failed connect. patches/tqftpserv switches the server to
explicit sendto destinations, preserving sender checks on replies. Also corrects
upstream no-option WRQ to ACK0 (it attempted reading an O_WRONLY fd), no-rsize
multi-block reads/EOF, and stdout flushing. ARM compile -Wall -Werror passed.
Host root/protection test passed. Nothing installed as an autostart service.
The subsequent guarded boot/test is recorded in the latest checkpoint above.



## Live bring-up results — 2026-09-30, through ~05:14

The complete stock modem firmware now passes PIL/MBA authentication and exits
reset. This fixes the earlier missing-segment prerequisite, but the modem then
signals fatal within roughly 0.2 seconds. The kernel's existing SYSTEM restart
policy panics/reboots the headset; it recovered to USB SSH after each test.
No wlan0 yet. Firmware staging was exclusively in /run (tmpfs), rmtfs retained
-P -r, and no boot/firmware/NV partition was flashed or written.

A second, independently verified blocker was found and fixed: missing IPC router
security initialization. A local service-locator request was stuck inside
wait_for_irsc_completion in msm_ipc_router_sendmsg. Stock init starts irsc_util
with vendor/etc/sec_config as root. Running that exact stock tool/policy unblocks
IPC, and the native pd-mapper now successfully answers wlan/fw with
msm/modem/wlan_pd, instance180 through libqipcrtr4msmipc. This does not boot modem.

Repo/device now include oculus-ipc-security helper and init service; rmtfs start_pre
also enforces initialization, to handle old/future-dated OpenRC dependency caches.
The helper explicitly clears inherited LD_PRELOAD when invoking Android's linker:
otherwise rmtfs's musl IPC shim contaminates the bionic process and startup fails.
The original rmtfs unit is saved on-device as oculus-rmtfs.pre-ipc-security.
Package source revision35 updated; signed device package not built/installed yet.
Cold-boot verification PASSED: ipc-final-coldboot.log at 94s shows the policy
marker, rmtfs started, and modem still OFFLINING.

With IRSC + mapper initialized, kernel logs show connection to WLAN domain180,
then the same modem fatal. A diagnostic kernel added a private AP/modem SMEM
lookup for the failure reason; both common and private lookups failed. That
kernel was booted TEMPORARILY with fastboot boot, never flashed. Its device tree,
ramdisk/watchdogs and boot arguments exactly matched the working image. After
its crash, the headset returned to installed kernel #4. Diagnostic source is in
patches/kernel; don't enable repeated modem voting based on that patch.

Stock init also starts a TFTP firmware-file server; this is absent from the port.
A native test server has been built from linux-msm/tqftpserv pin
c2559a26098f6d2b36a946ef0b6ad02223264c3e with a bounded translator:
firmware reads from /run/quest-wifi-firmware then /lib/firmware; writes only below
/run/quest-tftp-write, no persist/NV paths. Root/path tests pass (traversal,
absolute escapes, symlinks, read-only writes rejected). On-device protocol test
PASSED after the fixes below. A TFTP-backed test was subsequently run; see latest checkpoint above.

Evidence in W=/Users/<user>/work/quest-pmos-bringup/wifi-source:
- after-modem-pstore/ and irsc-pstore/: saved first/second kernel panics.
- r4-modem-kmsg-live.log and r4-modem-pstore.tar: diagnostic fallback result.
- irsc-and-mapper-query.log: successful local domain lookup.
- ipc-preload-fix.log: corrected IPC helper/rmtfs startup.
- Native mapper patch and probe in patches/pd-mapper and tools/wifi.


## Firmware and rmtfs audit — 2026-09-30 (latest)

The wearer accepted the optimized tracked diagnostic; Wi-Fi is the next priority.
Wi-Fi is still unavailable. No modem vote or firmware load was performed in this audit.

- Live rootfs inspection found no `modem.*` or `mba.*` firmware. The installed
  firmware archive also omits them. The owner's untouched `modem_b.img` contains
  MBA and a complete split modem ELF. `tools/inspect-fat16-firmware.py
  --verify-modem IMAGE` checks FAT16 geometry, chains, hashes and ELF segment
  lengths without mounting, extracting, or changing the backup.
- All 22 loadable ELF segments pass, including `modem.b19` (434064 bytes).
  Segments 12, 16 and 23 legitimately have no file payload. The address span is
  exactly `0x8cc00000..0x8fc00000` (48 MiB), matching the LIVE device-tree modem
  reservation. The live MBA reservation is `0x8b900000`, 2 MiB. This checks
  completeness/layout only, not signature acceptance or safe startup.
- **Correction to old rmtfs notes:** installed rmtfs is v1.3. `-P` selects raw
  partitions under `/dev/disk/by-partlabel`; it does NOT mean "no PIL". Automatic
  remote-processor startup requires `-s`, which is absent. `-r` opens backing
  partitions read-only and services writes in a RAM shadow. Keep `-P -r`; dropping
  `-r` is unnecessary and would remove NV protection. Live modemst1/modemst2/fsg/fsc
  links resolve to sdf1/sdf2/sdf3/sdf4.
- Primary port documentation reports that an earlier modem vote reached MBA,
  failed loading `modem.b19` through direct firmware loading/userspace fallback,
  then wedged during vendor shutdown until the hardware watchdog fired. Missing
  files are a concrete prerequisite failure today, but not proof of the earlier
  failure's sole cause. Audit `request_firmware_into_buf` and failure shutdown
  before any modem vote; a userspace timeout cannot bound a hung kernel/SCM call.
- Exact kernel source has default PBL/MBA timeout 1000 ms and modem-auth timeout
  10000 ms (both confirmed live). A special PIL IMEM marker can disable polling
  timeouts; no disable-timeouts message appeared in captured boot logs. This
  does not prove every secure-monitor or cleanup path is bounded.
- No firmware was staged to the headset, no NV/boot-chain partitions changed,
  and the recovery watchdog remains armed.

Evidence outside Git: `/Users/<user>/work/quest-pmos-bringup/wifi-source/`, including
`stock-modem-validation.txt`, `stock-modem-firmware-index.tsv`, `live.dtb`,
`live-modem-dt.txt`, `device-preflight.log`, and exact kernel source. Stock blobs
and the live DTB are not committed.

Primary references:
- [rmtfs v1.3 flags](https://github.com/linux-msm/rmtfs/blob/v1.3/rmtfs.c)
- [rmtfs v1.3 RAM shadow](https://github.com/linux-msm/rmtfs/blob/v1.3/storage.c)
- [Port's prior modem failure report, section 5](https://github.com/Block-Flock/pmaports-oculus-monterey/blob/master/docs/bringup.md)


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

Historical ADSP-only hypothesis (disproved by MODEMUW.JSN and live domain lookup):
- The modem hosts wlan_pd instance 180; its bring-up IS relevant to Wi-Fi.
  Keep guarded, read-only tests; see current results above.
- `subsys-pil-tz cce0000.qcom,venus: Failed to locate venus.mdt(rc:-11)` ← video, separate.

Firmware all present in `/lib/firmware`: `adsp.mdt`, `adsp.b00`, `adspua.jsn`,
`wlanmdsp.mbn`, `bdwlan.*`. So ADSP PIL has its firmware locally — it likely does
**not** need rmtfs to serve it.

rmtfs runs `/usr/bin/rmtfs -P -r`: **-P selects raw partition backing; -r
keeps backing files read-only and shadows writes in RAM.** No `-s` means it does
not automatically start a remote processor. Keep `-r` for all bring-up tests.

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

### 5. rmtfs — preserve read-only backing
The old suggestion to drop `-r` is withdrawn. Preserve `-P -r`; modem firmware
is staged separately in tmpfs. Diagnose protocol requests without enabling
persistent NV writes or automatic processor startup.

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

## Owner firmware and Android runtime extraction recipe (2026-10-01)

This recipe uses **your own raw partition backups**, outside Git. Source paths were
rechecked read-only against this owner's stock images on 2026-10-01. Hashes in
[assets-inventory.md](assets-inventory.md) identify that owner's firmware revision;
other stock revisions need their own manifest. A matching hash is an identity
check, not proof that an arbitrary modem image is safe for this device/kernel.

Prerequisites: a Linux host with `e2fsprogs` (`debugfs`), `mtools` (`mcopy`), Python3,
and `sha256sum`; raw (not Android sparse) stock `system_b.img` and `modem_b.img`
from the same known-good stock installation. On macOS, the verified debugfs was
`/opt/homebrew/opt/e2fsprogs/sbin/debugfs`; use `shasum -a 256` for hashing there.
If obtaining the missing modem backup from rooted stock Android, first resolve
`/dev/block/by-name/modem_b`, then use the same read-only-source `dd`/`adb pull`
procedure in [tutorial 01](tutorial/01-back-up-stock.md). Do not flash it.

Set absolute paths; create a new private extraction directory (no spaces in these
paths, because `debugfs` has its own command parser):

```sh
system=/absolute/private-backups/system_b.img
modem=/absolute/private-backups/modem_b.img
out=/absolute/private-work/owner-wifi-extract
umask 077
mkdir "$out"
mkdir "$out/system-firmware" "$out/modem-image" "$out/linux-firmware" "$out/wifi-firmware"
python3 tools/inspect-fat16-firmware.py "$modem" --verify-modem > "$out/modem-completeness.txt"
python3 tools/inspect-fat16-firmware.py "$modem" --all > "$out/modem-source-sha256.tsv"
# debugfs is read-only unless explicitly given -w; never add -w.
debugfs -R "rdump /system/vendor/firmware $out/system-firmware" "$system"
mcopy -i "$modem" '::/image/*' "$out/modem-image/"
# FAT short-name spelling may be upper case; Linux firmware requests are lower case.
for src in "$out/modem-image/"*; do
    [ -f "$src" ] || continue
    name=$(basename "$src" | tr '[:upper:]' '[:lower:]')
    case "$name" in
        modem.*|modemr.jsn|modemuw.jsn|mba.mbn|wlanmdsp.mbn)
            cp "$src" "$out/wifi-firmware/$name" ;;
    esac
    case "$name" in
        bdwlan.*|adspr.jsn|adspua.jsn|wlanmdsp.mbn)
            cp "$src" "$out/linux-firmware/$name" ;;
    esac
done
cp "$out/system-firmware/firmware/"adsp.* "$out/linux-firmware/"
mkdir -p "$out/linux-firmware/wlan/qca_cld"
debugfs -R "dump /system/vendor/etc/wifi/WCNSS_qcom_cfg.ini $out/linux-firmware/wlan/qca_cld/WCNSS_qcom_cfg.ini" "$system"
(cd "$out/wifi-firmware" && sha256sum ./* > SHA256SUMS && sha256sum -c SHA256SUMS)
(cd "$out/linux-firmware" && find . -type f -exec sha256sum {} +) > "$out/linux-firmware.sha256"
```

`debugfs rdump` creates the basename `firmware/` under the supplied destination.
The source/destination checklist is:

| Stock source | Private/rootfs destination and purpose |
|---|---|
| `modem_b.img:/IMAGE/MBA.MBN`, `MODEM.MDT`, `MODEM.B*` | Lowercase filenames in `/usr/share/oculus-wifi/firmware/`; complete MBA and split modem ELF, including `modem.b19` |
| Same `/IMAGE/MODEMR.JSN`, `MODEMUW.JSN`, `WLANMDSP.MBN` | Same service directory; mapper/TFTP/modem WLAN domain inputs |
| Same `/IMAGE/BDWLAN.*`, `ADSPR.JSN`, `ADSPUA.JSN`, `WLANMDSP.MBN` | Lowercase filenames in `/lib/firmware/`; board variants and DSP descriptors |
| `system_b.img:/system/vendor/firmware/adsp.*` | `/lib/firmware/adsp.*`; copy the complete available split family |
| Same `/system/vendor/etc/wifi/WCNSS_qcom_cfg.ini` (firmware-tree name is an absolute symlink) | `/lib/firmware/wlan/qca_cld/WCNSS_qcom_cfg.ini` |
| Same `/system/vendor/bin/cnss-daemon`, `/system/vendor/etc`, `/system/vendor/lib64`, `/system/lib64` (including `vndk-29`, `vndk-sp-29`), `/system/apex/com.android.runtime.release` | Kept together on the intact stock system filesystem; consumed by read-only runtime bind, **not independently copied into musl `/usr/lib`** |

On this owner's revision the split modem payload checklist is `modem.b00` through
`b11`, then `b13`, `b14`, `b15`, `b17` through `b22`, plus `modem.mdt`. Missing
`b12`/`b16` is not itself a defect: the metadata validator determines which segments
have payload. Do not synthesize empty segments or substitute another revision.
The resulting Wi-Fi directory has 26 firmware inputs plus its generated SHA256SUMS.

To stage in an **offline pmOS rootfs** after reviewing its existing files, copy
`wifi-firmware/.` to `/usr/share/oculus-wifi/firmware/` and `linux-firmware/.` to
`/lib/firmware/`. Preserve original files if different; do not overwrite unrelated
firmware. Keep the Wi-Fi directory mode0700/files0600. On-device service startup
runs `sha256sum -c SHA256SUMS` before copying firmware into RAM. An updated manifest
must be generated from the exact bytes installed, not copied from this owner's table.

The Android executable dependencies require no new extraction step on the headset:
`oculus-stock-runtime start` resolves the opposite stock slot via the boot command
line, mounts it `ro,noload,nosuid,nodev,noexec`, and creates the narrow executable
bind `/run/oculus-stock-runtime/system`. Thus this method **requires an intact stock
system in the other slot**. The Wi-Fi launcher then exposes only that tree, ordinary
null/random devices, read-only proc, and RAM-only data to `cnss-daemon`. Preserve
`RMTFS_MSM8998_RELATIVE_OFFSETS=1`, `rmtfs -P -r`, and the recovery guards. Extracting
firmware does not authorize a modem vote or establish safe shutdown behavior.
