# Camera roadmap authorized — 2026-09-30

User is reconnecting the headset and authorizes work on ALL five stages in
**docs/CAMERA-ROADMAP.md**: camera panel, hold-to-peek, calibrated stereo,
visual-inertial positional tracking, and camera-driven playground effects.
Start with camera capture. Preserve all stages, verify actual results, screenshot,
leave known-good boot and shut down at stopping point. Prior “do not boot again”
notes below are superseded for this new authorized work. Still no bootloader/NV/
factory changes or desktop. Full shutdown with charger attached still needs USB
unplug then Wi-Fi poweroff; do not repeat charging-mode reboot loops.

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

# Default scene/HUD + trigger renewal VERIFIED — 2026-09-30

Supersedes earlier in-progress checkpoint immediately below. User confirmed corrected
scene and HUD visible, requested actual composite screenshot; captured and shown at
W/hud-build/live-composite.png. Six physical trigger renewals confirmed. Scene ran
past original recovery deadline and >229seconds renderer time without premature exit.
Fresh boot then automatically started guard+scene (~72fps) and Wi-Fi; verified through
uptime125s with internetping2/2. Default services enabled; no desktop. Updated guard
with explicit --poweroff handler was running on that boot (executable hash matches).
After sync, invoked /usr/sbin/oculus-recovery-guard --poweroff as user requested.
Final shutdown observation follows when verified. Do NOT automatically boot it again.
User expects pressing Power tomorrow to start the scene; stillness is needed briefly
for gravity initialization. See docs/TEST-SCENE-AUTOSTART.md for installation, safety,
remaining slot-retry limitation, tests, artifacts, and RSSI label-only final update.
No splash/bootloader/modem/NV changes. All source remains uncommitted for later sorting.

# Test-scene autostart + HUD + renewable recovery — active work

See docs/TEST-SCENE-AUTOSTART.md for current implementation and verification.
Owner authorized replacing the fixed software recovery timer with a trigger-renewable
five-minute countdown; hardware watchdog must remain enabled. Device use is authorized
again; splash/logo investigation paused. At stopping point, owner wants POWER OFF,
not fastboot. Corrected scene currently running; WAIT for wearer response before changing
it or rebooting. First HUD version exited from a timestamp race; fixed version running
past old watchdog deadline and controller renewals confirmed. Autostart enabled but
fresh-boot verification and explicit shutdown still pending. Updated guard --poweroff
handler built but not yet loaded into the current running guard. Do not send SIGUSR2
to the current older guard. All changes remain uncommitted.

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

# Latest checkpoint — 2026-09-30 ~05:29

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

# Latest checkpoint — 2026-09-30 ~05:27

See docs/adsp-wifi.md for current Wi-Fi results; older checkpoints below are history.
modem.b19/full firmware verified and authenticated, but modem still fatal shortly
after reset. Diagnostic temporary kernel r4 tried; both SMEM crash-reason lookups
failed. Recovered automatically to installed #4; no boot image was flashed.
IPC policy helper now clears LD_PRELOAD and enforces initialization in rmtfs
start_pre; final coldboot PASSED. Device package source checksum verification PASS.

TFTP server native local IPC protocol test PASSED after patching connect/send to
sendto, ACK0 for WRQ, and multiblock EOF. Read-only firmware and RAM-only writable
roots verified by host and device tests. No modem test WITH TFTP yet.
New fresh guarded boot started; host W/wifi-source/tftp-test-stage.tar contains
all binaries, firmware and prepare-tftp-modem-test.sh (prepares only, DOES NOT vote).
W/quest-ssh, guards unchanged, desktop never enabled. After preparation, start
host log streams then run guarded-modem-vote.sh explicitly before uptime150.
No autostart mapper/TFTP/modem installed. Repos retain uncommitted current work.

# Latest checkpoint — 2026-09-30 ~05:04

IRSC-enabled modem test also crashed; live kernel log proves QMI WLAN domain180
connection succeeded before modemfatal. Boot recovered automatically to pmOS.
Saved second pstore W/wifi-source/irsc-pstore/ + .tar. No usable modemreason yet.
Live logs irsc-boot-kmsg-live.log and irsc-boot-rmtfs-live.log (rmtfs had no output).
No NV or firmwarepartition writes, no bootflash.

Installed permanent *IPC policy initialization* only: /usr/sbin/oculus-ipc-security,
/etc/init.d/oculus-ipc-security, rmtfs unit now needs it; existingunit backup
/etc/init.d/oculus-rmtfs.pre-ipc-security. Helper runs readonly stockirsc_util with
stockpolicy and marks /run/oculus-ipc-security.ready after success, avoiding duplicates.
Repo devicepackage updated pkgrel35 + checksums. Package notrebuilt/syncedKaliyet.
First coldboot didNOTstartit becauseOpenRC dependencycache had futuremtime. Fixed
with rc-update add oculus-ipc-security default and rc-update -u, start passed.
Coldboot recheck afterthisfix stillpending. No desktop/modemautostart changes.

Diagnostic kernel r4 built successfully (exec75247 done), exactexistingkernelconfig
andGCC4, onlypatch adds private AP/modem SMEM crashreason fallback whencommonlookup
fails. Repo patches/kernel/APKBUILD+0001-modem-partition-crash-reason.patch; original
APKBUILD saved W/wifi-source/linux-APKBUILD-r3. Kali nativebuildroot nowkernelsource,
NOT ARMMonado buildroot. SignedAPK Kaliwork/packages/edge/aarch64/linux-oculus-monterey-4.4.205-r4.apk.
Notinstalled. Extracted W/wifi-source/vmlinuz-r4. Built temporary Androidbootimage
usingnewgzipkernel plusEXACToriginalDTBtail, EXACTguardedramdisk, samecmdline/addresses,
recomputedSHA1imageID andlegacysignaturecontainer. OriginalimageIDvalidatedfirst.
W/wifi-source/diagnostic-r4-final.img SHA256
06abe1a5e489eb81e0a8f789442f9f9e298c51f031f33720df7b907ffea5ac6a.
Ramdisksha256063488d1f5f73c3d55cdbd2843d995f68aab9d5eca2bd88cebd05ec20941ed37;
DTBtail2073ab72a0c6330e06fa6a27802faee6a9f7b6ed8f254c813a8a861e0f77519c.

Justissued fastboot BOOT (NOT FLASH) diagnosticimage~05:04, toolsession96753.
Sending25MiBOK; awaitingboot result/SSH. Installedboot_b remainsworkingr3guarded.
Next: verify unamebuild#5 andIPCservicerunningautomatically; stage RAMfirmware,
mapper+queryclient andguardedvote. Onlyone diagnosticmodemretry toobtainactualreason;
retain allwatchdogs/NVshadow. After crash/reboot runtime revertsinstalledkernelr3;
capturepstore promptly. No physicalquestionpending.

# Latest checkpoint — 2026-09-30 ~04:55

Headset recovered automatically from modem panic. Full pstore archived under
W/wifi-source/after-modem-pstore/ (and .tar). It proves MBA boot and all modem
segments authenticated/loaded successfully: MBA147.311, modem out ofreset147.450,
SSCTL connected147.603, modem fatal147.673, SYSTEM restart kernelpanic147.781.
No b19 or shutdown-loader failure in this attempt. Unknown failure reason because
SMEM lookup failed. Boot returned to pmOS, then normalwatchdog/explicit reboot to
fastboot. No persistent firmware or NV writes. All /run test files vanished.

IMPORTANT NEW VERIFIED BLOCKER/FIX: IPC router security initialization was absent.
Read-only local pd-mapper query hung in D state at wait_for_irsc_completion from
msm_ipc_router_sendmsg. Stock init.monterey.rc starts vendor.irsc_util as root,
classcore. Executed stock irsc_util with stocksec_config viaexistingreadonly
Android linker/runtime. It exited0 and unblockedIPC. Local service-locator query
then PASSED: wlan/fw -> msm/modem/wlan_pd instance180, exit0. No modem started in
that boot. This is a concrete prerequisite fix, not proofWi-Fi works.
Command stockroot /run/oculus-stock-runtime/system, linker
apex/com.android.runtime.release/bin/linker64, LD_LIBRARY_PATH release/lib64/bionic,
release/lib64, stockroot/lib64, stockroot/vendor/lib64; executable vendor/bin/irsc_util
argument vendor/etc/sec_config. All alreadyreadonly stockmount. Kernel security
ioctl adds stockrules; closingIRSCsocket signalscompletion. Preserve policy,
never globallydisablesecurity.

pd-mapper built pin5ecd2fe926aca7abfe40724177f63b942cff3947 with explicit--maps patch
repo patches/pd-mapper. ARMbinary W/wifi-source/pd-mapper-quest; queryclient source
repo tools/wifi/query-pd-mapper.c, binary W/wifi-source/query-pd-mapper. Run both with
LD_PRELOAD=/usr/lib/preload/libqipcrtr4msmipc.so. Stockmodemr/modemuw JSON stagedonly
inRAM. Kernel dump_servers service0x40 instance0x101 yields node/port; observed1/0x1a.
Probe now uses literal valid QMI request because upstream reqdescriptor is decode-only
(VAR_LEN_ARRAY without DATA_LEN). Do not reuse oldfailedprobe. Successfullog
W/wifi-source/irsc-and-mapper-query.log. Upstream daemon noautostartenabled.

Next test beingprepared: freshlyrebooted existingguardedslotB~04:54:45, retrycount4
beforeboot. Wait~65–75sSSH. Stagecompletefirmware+pdmapper+client in/run, initialize
stockIRSC, runmapper andprovequery BEFORE modemvote. Use rmtfs -P -r -v manual
under samepreload withlogs (stop existingservice, rerunoculus-rmtfs start torestore
alias beforemanuallaunch). Capturelivekernel/rmtfslogstohostbeforeguardedvote.
Keepwatchdogs, noNVwrites, noflash. No pendingphysicalquestion. UserreportedMeta
logo duringpreviouscrashrecovery, thenSSHreturnedconfirmed.

Stock B modemversion01099 (2024), matchesinstalledwlanmdsp SHA256. SlotAbackup01091
is different; do notmix versionsor swap onguess. No more firmwareversionhypothesis
asprovenrootcause.

# Active recovery watch — 2026-09-30 ~04:43 host time

User explicitly said continue Wi-Fi work, no stopping at plans. This section
supersedes earlier "no modem vote performed" states. Do not repeat modem vote.

Boot B retry count reset from 0, rebooted around 04:39. At uptime 61s verified
hardware watchdog disable=0 and both 300s recovery sleeps. rmtfs running -P -r
with libqipcrtr4msmipc preload. No desktop changes, no NV/boot/firmware partition
writes. Validated complete stock firmware staged ONLY into /run/quest-wifi-firmware
(tmpfs), all SHA256 checks passed. Source audit checked direct firmware loader,
metadata/segments, bounded MBA/auth/AXI waits, direct nonsecure restart register
in actual DT; secure memory assignment remains opaque. Prepared manual guarded
script W/wifi-source/guarded-modem-vote.sh, intended to retain successful vote
until recovery reboot rather than exercise vendor close/shutdown.

First attempt at uptime109 failed modem.mdt -11 without starting MBA. Found
BusyBox printf with format plus newline had left firmware_class.path empty;
single-format printf with no newline and exact readback corrects it. First
attempt fully logged W/wifi-source/first-complete-modem-vote.log.
Second attempt at uptime147 used corrected path, verified all firmware, then
USB disappeared and SSH/debug-shell/fastboot not reachable. No successful vote
or WLAN proof. Need watchdog recovery and pstore capture before further tests.
No persistent firmware staged; tmpfs changes vanish at reboot. Normal watchdog
expected around uptime317..330. At04:43 host still off USB; physical display
question pending (logo/USB/black; observe only, no button instruction yet).

Parallel local work (same agent): preparing upstream pd-mapper on Kali, because
none installed and upstream default enumeration needs /sys/class/remoteproc
which downstream lacks. Clone /home/<user>/pmos/pd-mapper-quest; build dependency
install into ROOTFS CHROOT only running exec99131, W/wifi-source/pd-mapper-prep.log.
No device daemon installed or run. No other builds running.

# Latest checkpoint — 2026-09-30, firmware audit

This section supersedes historical active-session/build/gate status below.
No wearer question is pending. Accepted tracked diagnostic is unchanged; the
owner prioritized Wi-Fi. See docs/adsp-wifi.md for new firmware and rmtfs findings.
No modem vote, firmware staging, NV write or recovery-watchdog change occurred.

Monado r3 is BUILT and INSTALLED. All 22 tests passed serially; parallel QEMU
worker test crashed, so APKBUILD check now uses ctest -j1. r3 disables SDL peek
and debug GUI, includes fixed-origin POSITION_VALID with POSITION_TRACKED false,
stock optics and the real framebuffer compositor target. Session wrapper updated
on device, including XRT_LOG=debug readiness logging. The OpenXR hello_xr sample
creates the native Vulkan compositor and both eye swapchains successfully, but
no rendered-frame proof was captured: even holding stdin open, the client exited
0 without a visible session/frame sequence. Logs: W/runtime-r3-held-stdin.log and
W/runtime-r3-service-full.log. Do NOT claim real OpenXR display acceptance yet.
No VR home or autostart enabled. Desktop stays disabled.

Isolated Mesa KGSL EGL still SIGSEGVs during initialization. No acceleration
validated; do not replace global Mesa. GPU debugging deferred for Wi-Fi priority.

Firmware audit: stock modem has all 22 loadable segments, exact 48 MiB live-DT
memory match; installed rootfs lacks modem/MBA files. Prior modem failure report
specifically identifies modem.b19 fallback and vendor shutdown wedge. Full PIL
failure-path audit remains necessary before a vote. Current rmtfs -P -r already
provides RAM-shadow writes over read-only partitions; never remove -r based on
historical notes. Firmware inspector now supports --verify-modem.

Current boot will return to fastboot via unchanged watchdog. Last boot B retry
count was 1 before reboot; check count before next boot, set_active b only if
needed. No package builds or renderer processes left intentionally running.

# Latest checkpoint — 2026-09-30 ~04:27

Wearer accepted optimized TRACKED scene: “looks great. there's no concept of around but the test scene looks good for what it is. wifi, or is there more to do here?” Explained3DoF rotation vs position, agreed finishbuiltGPUprobe thenWi-Fi. No physicalquestionspending. Acceptedrenderer+logsarchived /Volumes/vela/Backups/quest1-recovery/pmos/head-tracking/display-paced-20260930 withSHA256SUMS; historicalhandoffmirrorupdated. Installedoptimizedrendererunchanged.

Mesaisolatedpackage successfullybuilt, signed, installed/opt/mesa-kgsl pluslz4-libs. NativecodegenerationworkarounddocumentedinKGSL-ACCELERATION.md. EGLprobeSIGSEGV139duringinitialization, onlysurfacelessdebuglog; nohardwareaccelvalidated. StopGPUdebugfornow tohonorWi-Fipriority. No globalMesachanges. DriverprobeandpropertieslogsW/kgsl-*.

Monador3buildnowactiveexec33596, W/monado-r3-build.log; sourcechecksumsynced. r3notyetinstalled. SharedbuildrootnowMonado, Mesaunstrippedbuilddeletedbynewbuild; packagedAPKsafelocal. Headset freshguardedboot~04:25,SSHavailable~04:26, currentr2installed. NativeVRcompositorstillnotworking; r3willdisableSDLpeekandaddPOSITION_VALIDfixedorigin/tests. No VRhome/autostartenabled,desktopdisabled.

Wi-Fi investigationstartedoffline: readcorrectedADSP-WIFI-HANDOFFandrepo docs/adsp-wifi.md; AUDIO ADSPONLY theorydisproved. MODEMUW.JSN saysmodem/wlan_pd. DO NOT bootmodemblindly orenablewritablermtfs. ExactkernelsourcecachedKali/cache_distfiles/linux-oculus-monterey-6929f734ce0e602018790ff3a52dc7bad646af60.tar.gz. ExtractedW/wifi-source/pil-q6v5-mss.candmsm8998.dtsi. BaseDTSmodemfirmwarename modem,self-auth,mem-protect0xF,compatibleq6v55. NoPIL/NV/firmwarewritesperformed. Need inspectpil-q6v5.c,stockinitfirmwareandknownwedgebeforeanynewexperiment. Maintainwatchdog.

# Wearer gate pending — 2026-09-30 ~04:19

Optimized border-raster diagnostic installed as /root/quest-mesh-fast/monterey-head-mesh-fb, old paced version saved as *-paced-backup. Updated wrapper installed with NO forced refresh override, ensures /run/user/10000 exists. Fresh boot calibrated successfully, servicePID12221, Started90Hz, bias .007564,-.012803,-.003260. Running gated wrapper nohup with MONTEREY_START_FILE=/tmp/quest-border-wearer-ready, output/tmp/quest-border-tracked.log. ASKED wearer Ready; MUST WAIT FOR ACTUAL REPLY before touch gate. Atasktimebootuptime~1min40s. Never disable300s watchdog. Ifreplylateandbootgone, freshlyboot/recalibratefirst. Afterreply touchgate and askwhetherheadtrackingreturnedandmotionsmoother,leaveunchangeduntilobservations. Runtime stillr2 headless for this diagnostic, notactualnativecompositor.

Mesa resume71019 stillworking giantNIRC, localmesa-kgsl-native-resume.log. NoAPKyet. Newruntime session wrapper XRT_LOG=debug needed to expose Listening socket log (globaldefaultWARN); repo updated,notdeviceyet. R3APKbuildstillpendingMesa.

# Latest checkpoint — 2026-09-30 04:17 host time

- Mesa build reached1330/1338 but nir_opt_algebraic.py under qemu took >14min. Ran identical source generator natively on Kali into /tmp/quest-nir-native.c (48MB), verified NIR source directories identical excluding __pycache__. Terminated only stuck generator PID2994957; original package build consequently exited failure. Saved build.ninja.before-native-nir, changed only that generated command to copy the verified native output into build output. Resumed meson compile then abuild rootpkg in existing buildroot, exec71019, local mesa-kgsl-native-resume.log. Currently compiling giant generated C. Canonical APKBUILD unchanged (normal generator still used for reproducible fresh builds). Do not start r3 package build until this finishes.
- r3 patch regenerated + synced with fixed-origin POSITION_VALID (POSITION_TRACKED remains false), plus tests. Needed for normal OpenXR samples to draw on a3DoF device. r3 still NOT built. WINDOW_PEEK=OFF/DEBUG_GUI=OFF.
- Direct diagnostic now driver-paced. Static first measured72fps uniform13.89ms (0/1200 intervals>25ms), user said “much smoother but doesn't rotate in space anymore”; explained --static intentionally no tracking. Tracked test later observed~90Hz actual cadence and78–84fps, skipping11.14→22.28ms. Thus72Hz inference not stable across panel wakeups. Originalcontrol90 confirmed; temporary helper72 test yielded57.6fps and helper exits1 despite effective write. Restore90 performed while active; readback panel-control90 confirmed, static~86fps. Do not rely on helper return code or claim supported72 setting validated. No permanent rate setting. Removed draft launcher forced72 env locally (device wrapper still72 until recopied).
- New CPU line optimization draws only newly exposed3x3 stamp border. Equivalent pixel coverage/OR blending verified against original rasterizer for both eyes/all octants/clipping. Source renderer/fast/framebuffer.h + test-framebuffer.c; compiled rootfs/tmp/quest-fast; local executable W/monterey-head-mesh-fb-border fetched. NOT on headset yet.
- Current installed diagnostic is paced version /root/quest-mesh-fast/monterey-head-mesh-fb,60fps backup alongside. Runtime r2 remains installed, real compositor blocked by SDL peek; native VR home/autostart not implemented/enabled. Desktop remainsdisabled.
- Latest headset state: fastboot retry-count:b3, reboot issued~04:17. Wait~70sSSH; then transfer optimized binary+updatedwrapper and calibrate tracked scene with fresh readiness gate. No physical question currently pending. User confirmed continue. Leave test unchanged while asking observation. Keep300s watchdog.
- Kali connectivity intermittently drops then returns; user says “needs poked”, has not gone anywhere. Don’t restart machine or unrelated op25.

# Latest checkpoint — 2026-09-30, ~04:03 host time

- r2 built and all 22 tests passed serially (parallel QEMU worker test once crashed; no test skipped). Signed r2 APK installed on headset, with four signed cached Qt dependencies transferred over USB. Stock mesh provisioned at /var/lib/monado/monterey/distortion-mesh.bin. Khronos hello_xr built and staged /root/openxr-sample/src/tests/hello_xr/.
- First real compositor test failed: upstream XRT_FEATURE_WINDOW_PEEK unconditionally requests SDL/Vulkan surface extensions despite no desktop. Tracking and stock mesh loaded successfully. Service cleanup also hit an uninitialized mutex assertion after this early Vulkan failure. No frame rendered. Log local runtime-r2-first-run.log.
- APKBUILD now pkgrel3, WINDOW_PEEK=OFF and DEBUG_GUI=OFF; NOT built/installed yet. Source framebuffer metrics added (240 driver-completion intervals: fps/p50/p95/p99/max/>25ms count). Patch regenerated/checksum updated and synced to Kali port. Build Monado AFTER Mesa finishes; shared buildroot must not run concurrent package builds.
- Session wrapper now waits for tracking AND IPC listening log (IPC_LOG=debug), avoiding launching app before compositor ready. Wrapper changed in repo, device still old wrapper. Runtime autostart NOT enabled; no VR home implemented.
- Isolated Mesa Freedreno KGSL build running, log mesa-kgsl-build-retry.log, exec session94354. Configure finished; 1338 compilation steps. APK deps py3-yaml correct. No KGSL libraries installed on headset yet. Mesa prefix /opt/mesa-kgsl; does NOT replace system Mesa. Turnip cannot support Adreno540, OpenGL-over-KGSL remains experiment and will not automatically accelerate Vulkan Monado.
- tools/kgsl/probe-properties.c queries device info and UCHE_GMEM_VADDR only. Compiled local W/quest-kgsl-probe. tools/kgsl/probe-egl.c offscreen clear/readback validates reported renderer and color; compiled W/quest-egl-probe. Neither run on device yet. Use isolated LD_LIBRARY_PATH=/opt/mesa-kgsl/lib EGL_PLATFORM=surfaceless MESA_LOADER_DRIVER_OVERRIDE=kgsl for GL probe once isolated libs staged.
- Updated diagnostic framebuffer interval metrics building on Kali rootfs /tmp/quest-fast, execsession90256; fetch executable after done. Static run can measure jitter without wearer/tracking. It is still single-buffer scanout and may tear. No panel-rate changes made.
- Watchdog returned to fastboot normally after r2 test. Issued fastboot reboot ~04:01; SSH not ready at latest poll, retry normally after ~70s. Boot B tries reset earlier, should remain several. Desktop disabled throughout. No flash/NV/factory writes.

Earlier detail below (build/device statuses superseded by above):

# Active runtime integration — 2026-09-30

Owner explicitly requested completing runtime optics, frame-pacing work, and VR-only startup, then added another agent's suggestion: investigate Mesa/Turnip KGSL instead of trying to get a DRM render node. Do not stop at a plan. No subagents authorized. Preserve recovery watchdog and all boot-chain/NV/firmware safety constraints in existing handoff. Wait for actual wearer responses to physical questions.

Current completed device state: Monado r1 tracking installed; direct diagnostic renderer uses stock RGB lens warp and a two-CPU-buffer threaded framebuffer submission path. Driver and renderer both measured ~59.8fps. Owner confirmed visible and definitely better, but not fully smooth. Desktop boot service removed from default runlevel and stopped; wrapper no longer restarts desktop. No VR shell autostart yet. Current runtime integration r2 NOT installed. Headset returned to fastboot; last verified retry-count:b=0, so next boot needs set_active b before reboot. No flash required.

Canonical repo: /Volumes/vela/src/quest1-nura-port (initially clean before our changes). All current modified/new renderer/fast, README/docs, patches/monado changes are this agent's uncommitted work. Do not commit binaries/proprietary stock mesh/private SSH key. Existing artifacts/local work: /Users/<user>/work/quest-pmos-bringup. SSH wrapper quest-ssh; serial<SERIAL>; fastboot /Users/<user>/Library/Application Support/QuestStack/platform-tools/37.0.1/osx-universal/fastboot. USB root172.16.42.1. Boot SSH ~70s; watchdog300s realroot/~330s total. Device only has /dev/kgsl-3d0, no /dev/dri. Framebuffer2880x1600 stride11520, RGBA byte order (red0 green8 blue16),180degree rotation. CPU memcpy alone yields black; MUST unblank current mode and FBIOPAN_DISPLAY per submitted frame. See framebuffer.h and prior notes for exact implementation.

## Work drafted now

Local full source /Users/<user>/work/quest-pmos-bringup/monado-source (pinned e160863ad52c0e1c29c8bb7df3e47facdb02288d, already r1-patched).
Original r1 files copied to /Users/<user>/work/quest-pmos-bringup/monado-r1-base for diffs. New changes generated into repo patches/monado/0002-monterey-runtime-optics-framebuffer.patch. APKBUILD pkgrel2 includes patch1 and patch2 and checksums. To regenerate patch2/checksum after modifying full source, run local regenerate-runtime-patch.py. DO NOT rerun old patch-tracking-startup/add-tracking scripts (non-idempotent).

Changes in draft r2:
- Driver monterey_lens.c/.h copied from already-tested direct decoder, parses mesh header FOV angles with range/finite checks. Stock blob NOT in repo. Runtime defaults /var/lib/monado/monterey/distortion-mesh.bin; MONTEREY_DISTORTION_MESH override. Missing/invalid file fails device creation (needs provisioning before install/use).
- monterey_device.c loads mesh, overrides asymmetric FOVs with header angles, sets compute_distortion callback and builds distortion mesh. Converts Monado screen top-down UV to stock grid bottom-up, subtracts calibrated per-eye27.61/-27.61px shifts (retains wearer820/620 centers), forward grid ray to textureUV based on actual FOV. Out-of-grid returns -2 UV for border. This runtime optical orientation still needs real compositor validation.
- tests/tests_monterey_protocol.cpp synthetic monotonic grid fixture file (no proprietary data), sets env during lifecycle test and verifies center/up/right UV orientation plus45deg fixture FOV. Other original22test targets remain.
- New main/comp_window_monterey_fb.c based on offscreen debug_image target. Selected explicitly XRT_COMPOSITOR_MONTEREY_FB=1; normal Monado Vulkan compositor, NOT null/headless output. Uses scratch target2880x1600 RGBA, finalTRANSFER_SRC layout, copy to coherent mapped stagingbuffer after queue-ordered image barrier, fence wait, rotate180 and forward to shared framebuffer submission worker. Still software Vulkan/Lavapipe; no acceleration claim. Single Vulkan readback blocks compositor per frame for initial correctness; optimize after first success.
- main/monterey_framebuffer.h derived from verified diagnostic backend (raster line functions removed), handles nativecolor,unblank,FBIOPAN_DISPLAY, threaded bounded two CPU buffers,cleanup.
- comp_window.h includes xrt_config_drivers.h and factory declaration, comp_compositor.c conditionalfactory entry, compositorCMake conditional newsource, comp_settings.c explicit target envoption.
- First compile failed only because vk_bundle has no vkResetCommandBuffer; fixed to vkResetCommandPool(device,pool,0). Retry build currently running; check logs.

## Active builds and hosts

Kali <user>@<BUILD_HOST_IP> keyauth sudo-n.
pmbootstrap /home/<user>/pmos/pmbootstrap/pmbootstrap.py; pmaports port /home/<user>/pmos/pmaports/device/testing/monado-oculus-monterey.
Rootfs chroot /home/<user>/pmos/work/chroot_rootfs_oculus-monterey. Buildroot /home/<user>/pmos/work/chroot_buildroot_aarch64. Persistent log /home/<user>/pmos/work/log.txt. pmbootstrap build --lax preserves dirs; full build ~2-4min. Builds crossdirect x86host→aarch64 and tests viaqemu.

1. r2 retry: unifiedexec session97394, local monado-r2-build-retry.log. Build command python3 pmbootstrap.py build --lax monado-oculus-monterey. Original failed log monado-r2-build.log. At last check running configure/build. Need all22tests pass, inspect warnings, then fetch APK. Do not claim installed.
2. Khronos official OpenXR-SDK-Source cloned shallow /home/<user>/pmos/OpenXR-SDK-Source at3ed64d0f9bb680f24b80a085091e5c8fab38f7b7. Copied into rootfschroot/tmp/OpenXR-SDK-Source. hello_xr build session52454, local hello-xr-build.log. rootfs installed cmake samurai vulkan-loader-dev glslang-dev jsoncpp-dev pkgconf python3 libxrandr-dev libxxf86vm-dev. Command cmake -B build -G Ninja -DBUILD_TESTS=ON -DBUILD_API_LAYERS=OFF -DBUILD_CONFORMANCE_TESTS=OFF -DCMAKE_BUILD_TYPE=Release && cmake --build build --target hello_xr -j4. CMake compile feature tests underqemu slow. Posix platform plugin itself needs noXdisplay; Vulkan plugin expectedusablewithoutdesktop. Need confirm actualrun. This sample is for real OpenXR pipeline validation, NOT a finished VR home shell.
3. KGSL investigation: shallow sparse clone of primary fork https://github.com/lfdevs/mesa-for-android-container.git into Kali /home/<user>/pmos/mesa-kgsl, session19254. Requested sparse dirs src/freedreno/drm src/freedreno/common src/gallium/drivers/freedreno .github. Inspect actualKGSL capabilities/a5xx restrictions before build. No GPU driver installed/changed yet.

## GPU findings

User suggestion: KGSL backend can avoidneed for DRMnode. Accept and investigate, don't assumeDRMrequired. However official https://docs.mesa3d.org/drivers/freedreno.html explicitly says Turnip Vulkan supportsAdreno6xx and no plans fora5xx or earlier. Freedreno GL supports2xx-6xx, making GL-over-KGSL candidate; Monado Vulkan compositor acceleration still separate problem. PrimaryXDC2023LucasFryzek slides https://indico.freedesktop.org/event/4/contributions/198/attachments/128/188/XDC_2023_Lucas_Fryzek_Freedreno_on_Android.pdf documentFreedrenoKGSL work. Fork authorreferencesMesaMR21570. Need verify540 support/oldKGSLioctlcompatibility, don't install arbitrary prebuiltdebian libraries onmusl. May needlibhybris/stockVulkan alternative ifMonadoaccelerationcritical, no researchcompletedonthat yet.

## Next concrete steps

- Finish r2 build/test, fixcompile/testfailures. Regeneratepatch/checksum and syncAPKBUILD+patch toKali beforebuildretry. Preserve signed r1 rollbackAPK.
- Finishhello_xr build. Stage its binaryandrequired libs usingrootfs apkpackages ifneeded; no SFTP onheadset, use cat/tar pipes overSSH.
- Provision extractedlocal stockmesh (/Users/<user>/work/quest-pmos-bringup/stock-optics/distortion-mesh.bin) into device /var/lib/monado/monterey/ without factorywrites. Install signedr2APK onlyaftertests.
- Freshguardedboot (resetBtriesifneeded). NEVERenable desktopagain. StandaloneMonado service env XRT_COMPOSITOR_MONTEREY_FB=1 and XRT_COMPOSITOR_NULL unset/0. RootXDG_RUNTIME_DIR=/run/user/10000 may need mkdir/chown since desktopnoautostart. Inspectcompositoroutput and realOpenXRsample. All testingstillwatchdogguarded.
- ImplementusefulVRhome/sessionautostart onlyafter realcompositorverified; don't substituteheadlessdemoandclaimOSintegration.
- Investigateframestutter usingactualsubmitinterval distributions andvsync. Suspected60fps producer vs90Hzpanel uneven cadence; notyetmeasured. Panel sysfsreports dynamicfps enabled60..90 in earlierinfo. No refreshchangesmade. Could explore supported60HzsettingONLYafterreadingactualdriver interface/state, withreversible restore. GPU acceleration likelylongtermlimiter.
- Recordprogressfrequently; physicalreadiness questions waitforactualreply; prepareruntimebeforeaskwearer, avoidwatchdogendduringtest byfreshboot.
