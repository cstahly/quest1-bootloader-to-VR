# Default test scene and renewable recovery timer

Installed and boot-verified, 2026-09-30. Native diagnostic scene, not a desktop or VR home.

For source/build/install destinations and dependency order, use the
[ordered runtime assembly](../runtime/README.md). That newly consolidated sequence
has not been validated as a clean install; the evidence below is the historical
owner-device test. Current installed fallback is orientation-only with the single
composite camera wall. r5 positional tracking was isolated; r8 and CPU cache
invalidation remain uninstalled experiments. Earlier counts and slot retries below
are historical observations, not a current device-state query.

The owner requested boot-to-test-scene, live data visualization, and a controller
trigger to restart the watchdog countdown. The old rule to never alter the fixed
software timer is superseded only for this explicit change. The hardware watchdog
remains enabled. No boot image, bootloader, splash, modem, NV, or factory calibration
partition is written by this work.

## Runtime

- `/usr/bin/oculus-test-scene` invokes the direct framebuffer diagnostic with the
  owner's stock lens mesh and Monado head tracking. Waits for actual gravity
  initialization (maximum 60 seconds) rather than a blind 12-second delay.
- `/usr/sbin/oculus-recovery-guard --run` adopts BOTH original software timers:
  the real-root BusyBox shell and the initramfs `/hooks/20-oculus-recovery.sh` shell.
  The legacy shells and their `sleep 300` children are identified by exact command
  lines, process start times, parent IDs, and the recorded initramfs PID. The new
  countdown is armed before terminating either old timer, preserving the earlier
  original deadline. Unexpected/missing processes fail preflight without takeover.
- Holding either physical index trigger for two seconds renews the deadline to
  five minutes from that hold. Release is required before another renewal. A held
  trigger at attachment, stuck trigger, dropped input sync, or disconnected
  controller cannot renew indefinitely. A/X buttons do not count.
- The bridge now exposes a separate `BTN_TRIGGER` in addition to existing pointer
  keys; this does not start any desktop. The guard reads only the matching Touch
  uinput devices without grabbing them.
- Recovery owns its timer independently of the renderer. Expiration, guard TERM,
  or input-worker failure requests bootloader recovery. Hardware watchdog unchanged.
- Guard and scene OpenRC services are enabled in `default` on the test device.
  Guard startup waits for Wi-Fi's initial legacy-guard validation to finish.
- `--poweroff` is an explicit root-only orderly power-off request. Never use this
  against an older already-running guard that lacks its SIGUSR2 handler. Normal
  service stop is deliberately refused; arbitrary TERM means recovery, not poweroff.
- `--renew` is an explicit root-only maintenance request that renews the countdown
  to five minutes. It is not an automatic keepalive. Expired deadlines cannot be
  revived. Both control commands check that the running guard has the same binary
  inode as the caller; after replacing the binary, reboot before using them.

## HUD

Head-locked stereo panel uses the same stock optical projection as the scene.
Shows measured render-loop fps, CPU drawing time, submission wait, angular speed,
Wi-Fi IPv4/RSSI, recovery countdown, renewal count, and last 90 frame intervals.
Graph reference is 13.9ms; orange intervals exceed25ms. Loop rate is not a claim
that every frame reached scanout. Cached sparse pixels refresh twice per second.
No subprocesses, DNS, or network requests are made by the HUD.

## Evidence and remaining checks

- ARM builds with warnings as errors. Recovery-policy tests cover release/hold,
  stuck input and expiry precedence. Native Linux process tests cover unattended
  expiry, renewal, TERM, input-worker failure, overdue input and poweroff routing;
  test builds replace reboot operations with fixed exit codes.
- ARM HUD raster test checks both eyes, pixel bounds and capacity using the owner's
  private mesh. PNG inspected locally. A status-read race initially made the scene
  exit after35seconds; corrected timestamp tolerance and added regression coverage.
- First corrected on-device scene has stayed running beyond the old315s deadline.
  Controller renewals observed in the live status file (six at latest checkpoint),
  both evdev inputs open, Wi-Fi associated. Frame submission measured~72fps initially
  and~84–90fps after panel wake, with11.14/22.29ms intervals; do not hardcode refresh.
- Wearer confirmed corrected scene/HUD visible. Six real controller-trigger renewals
  observed; scene remained active beyond both old timer deadlines. Full framebuffer
  capture saved privately as `hud-build/live-composite.png` and shown to the owner.
- Cold-boot automatic startup PASSED: at uptime83s scene submitting72fps; by99s Wi-Fi
  associated with DHCP<HEADSET_WIFI_IP>. At125s, default services running, scene still
  submitting71.9fps, hardware watchdog enabled, internet ping2/2. No manual service
  start or tracking-readiness intervention on that boot. Evidence `hud-build/autostart-verified.log`.
- Explicit `oculus-recovery-guard --poweroff` issued after sync at end of validation;
  final USB/shutdown observation is recorded in the main handoff.
- Driver sometimes reports a placeholder0dBm. Final installed HUD shows RSSI N/A
  for invalid/nonnegative levels instead; the earlier screenshot shows the old0dBm label.
  This label-only fix was installed after the successful cold-boot launch and will
  be picked up next launch. It builds with warnings as errors; no renderer restart
  was needed during the user's viewing.
- For the next boot: press Power, leave the headset still briefly for gravity
  calibration, then wear it. Hold either index trigger2seconds and release to reset
  the remaining time to5minutes. Expiry still returns to fastboot. USB-unplugged
  startup has not been physically verified. Slot B had6 retries before the last
  validation boot, so approximately5 remain; boot-success marking is still future
  work. Do not assume unlimited reboot cycles without maintaining slot metadata.
- Private binaries and images: `/Users/<user>/work/quest-pmos-bringup/hud-build`.
  Build machine: `/home/<user>/quest-hud-build`, ARM staging inside pmOS rootfs
  `/tmp/quest-hud-build`. Original device bridge saved as `.pre-hud`.

Logo investigation is paused at owner request. Read-only backup inspection did not
locate the blue Meta M; both ABL and XBL remain untouched. Do not infer the graphic
must be in ABL solely because splash contains older grey Oculus logos.

## Charging-mode limitation

Poweroff with USB attached led to a new kernel boot with `androidboot.mode=charger`;
pstore was empty. pmOS currently starts its regular services in that mode. Unplug
USB and issue `oculus-recovery-guard --poweroff` over Wi-Fi to leave it off. A proper
charger-only mode is not implemented. Do not repeatedly reboot to try to fix this.
