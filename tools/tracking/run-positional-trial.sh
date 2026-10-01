#!/bin/sh
# Bounded, opt-in trial. Existing boot defaults and installed runtime stay intact.
# RUNTIME contains usr/bin/monado-service, usr/lib/libopenxr_monado.so and
# monterey-head-mesh-fb. BUNDLE contains the private VIT dependencies/config/map.
set -eu
[ "$#" -eq 3 ] || { echo 'usage: run-positional-trial.sh RUNTIME BUNDLE SECONDS(10..180)' >&2; exit 2; }
runtime=$1
bundle=$2
seconds=$3
case "$runtime:$bundle" in /*:/*) ;; *) echo 'absolute paths required' >&2; exit 2;; esac
case "$seconds" in ''|*[!0-9]*) exit 2;; esac
[ "$seconds" -ge 10 ] && [ "$seconds" -le 180 ]
[ -x "$runtime/usr/bin/monado-service" ]
[ -x "$runtime/monterey-head-mesh-fb" ]
[ -x "$bundle/live-vit-quality" ]
[ -f "$bundle/head-offset.txt" ]
if [ -n "${MONTEREY_START_FILE:-}" ] && [ -e "$MONTEREY_START_FILE" ]; then
 echo 'Readiness gate already exists; choose a fresh path' >&2; exit 2
fi
/usr/sbin/oculus-recovery-guard --check
umask 077
trial=$(mktemp -d /run/quest-positional-trial.XXXXXX)
printf '%s\n' "$trial" # Logs and diagnostics are kept until reboot.
# Preserve enough provenance to distinguish capture changes from runtime changes.
# Do not dump the environment, Wi-Fi configuration, or private calibration data.
{
 printf 'manifest_version=1\nseconds=%s\n' "$seconds"
 printf 'prediction_requested=%s\ncamera_hz_requested=%s\n' "${MONTEREY_VIO_PREDICT:-unset}" "${QUEST_VIO_CAMERA_HZ:-default}"
 printf 'worker_cpu_affinity=4-7\nworker_nice=10\n'
 printf 'camera_settings_source=configured-not-confirmed-running\n'
 if [ -r /etc/conf.d/oculus-camera ]; then
  awk '/^export CAMERA_(AUTO_EXPOSURE|DENOISE|DIAGNOSTICS|EXPOSURE_US|GAIN_Q4)=[0-9]+$/ { print }' /etc/conf.d/oculus-camera
 else
  printf 'camera_config=unavailable\n'
 fi
 for artifact in "$0" "$runtime/usr/bin/monado-service" \
  "$runtime/usr/lib/libopenxr_monado.so" "$runtime/monterey-head-mesh-fb" \
  "$bundle/live-vit-quality" "$bundle/lib/libbasalt.so.2" \
  "$bundle/check.conf" "$bundle/live-map.bin" "$bundle/head-offset.txt" \
  /usr/libexec/oculus-camera/stock-camera.so; do
  if [ -r "$artifact" ]; then sha256sum "$artifact"; else printf 'missing=%s\n' "$artifact"; fi
 done
} > "$trial/manifest.txt"
service_pid=
worker_pid=
renderer_pid=
cleanup() {
 trap - EXIT HUP INT TERM
 for pid in "$renderer_pid" "$worker_pid" "$service_pid"; do
  [ -z "$pid" ] || { kill -TERM "$pid" 2>/dev/null || :; wait "$pid" 2>/dev/null || :; }
 done
 unset MONTEREY_VIO_POSE_FILE MONTEREY_VIO_PREDICT MONTEREY_TRACKING_RESET_REQUEST XR_RUNTIME_JSON
 rc-service oculus-test-scene start
}
trap cleanup EXIT
trap 'exit 130' HUP INT TERM
rc-service oculus-test-scene stop
for i in 1 2 3 4 5; do pgrep -x monado-service >/dev/null || break; sleep 1; done
if pgrep -x monado-service >/dev/null; then echo 'Existing Monado still running' >&2; exit 2; fi
export XDG_RUNTIME_DIR=/run/user/10000
export XRT_COMPOSITOR_NULL=1 XRT_NO_STDIN=1 MONTEREY_LOG=info
export MONTEREY_DISTORTION_MESH=/var/lib/monado/monterey/distortion-mesh.bin
export MONTEREY_VIO_POSE_FILE="$trial/pose.bin"
export MONTEREY_TRACKING_RESET_REQUEST="$trial/reset-request"
export XR_RUNTIME_JSON="$trial/runtime.json"
# Paths in this developer harness must contain no JSON metacharacters.
case "$runtime" in *'"'*|*'\'*) echo 'unsupported runtime path' >&2; exit 2;; esac
printf '{"file_format_version":"1.0.0","runtime":{"name":"Monado positional trial","library_path":"%s/usr/lib/libopenxr_monado.so"}}\n' "$runtime" > "$XR_RUNTIME_JSON"
: > "$trial/runtime.log"
timeout "$((seconds+45))" "$runtime/usr/bin/monado-service" > "$trial/runtime.log" 2>&1 </dev/null &
service_pid=$!
attempt=0
while ! grep -q 'Gravity initialized' "$trial/runtime.log"; do
 kill -0 "$service_pid"; attempt=$((attempt+1)); [ "$attempt" -lt 120 ]; sleep .25
done
QUEST_VIO_HEAD_OFFSET_FILE="$bundle/head-offset.txt" QUEST_VIO_MAILBOX="$trial/pose.bin"
export QUEST_VIO_HEAD_OFFSET_FILE QUEST_VIO_MAILBOX
deadline=$(( $(cut -d. -f1 /proc/uptime) + seconds ))
segment=0
start_worker() {
 remaining=$(( deadline - $(cut -d. -f1 /proc/uptime) ))
 [ "$remaining" -gt 0 ] || return 1
 suffix=
 [ "$segment" -eq 0 ] || suffix="-$segment"
 timeout "$((remaining+5))" nice -n 10 taskset -c 4-7 \
  "$bundle/lib/libc.musl-aarch64.so.1" --library-path "$bundle/lib" \
  "$bundle/live-vit-quality" "$bundle/check.conf" "$bundle/live-map.bin" \
  /run/quest-camera-root/tmp/camera-feed "$trial/poses$suffix.csv" "$remaining" > "$trial/worker$suffix.log" 2>&1 &
 worker_pid=$!
}
start_worker
# Caller can request a fresh external wearer-readiness gate. This wait is bounded
# by the worker lifetime and never extends either recovery mechanism.
if [ -n "${MONTEREY_START_FILE:-}" ]; then
 while [ ! -e "$MONTEREY_START_FILE" ]; do kill -0 "$service_pid"; kill -0 "$worker_pid"; sleep .2; done
fi
MONTEREY_FRAMEBUFFER=1 MONTEREY_RECOVERY_SESSION=1 \
 timeout "$((seconds+10))" "$runtime/monterey-head-mesh-fb" > "$trial/renderer.log" 2>&1 &
renderer_pid=$!
# Restart only the positional worker on explicit wearer request. Runtime,
# orientation, display, trial deadline and both watchdogs remain unchanged.
resetting=0
while kill -0 "$worker_pid" 2>/dev/null; do
 kill -0 "$service_pid"; kill -0 "$renderer_pid"
 [ "$(cut -d. -f1 /proc/uptime)" -lt "$deadline" ] || break
 if [ -e "$MONTEREY_TRACKING_RESET_REQUEST" ] && [ "$resetting" -eq 0 ]; then
  kill -TERM "$worker_pid" 2>/dev/null || :
  wait "$worker_pid" 2>/dev/null || :
  worker_pid=
  segment=$((segment+1))
  if [ -e "$MONTEREY_VIO_POSE_FILE" ]; then mv "$MONTEREY_VIO_POSE_FILE" "$trial/pose-before-reset-$segment.bin"; fi
  start_worker || break
  resetting=1
  printf 'Positional estimator restart %s; original trial deadline retained\n' "$segment"
 fi
 # Keep renderer's origin reset pending until the new worker has published;
 # its fresh first packet is unready and will pass the normal startup gate.
 if [ "$resetting" -eq 1 ] && [ -s "$MONTEREY_VIO_POSE_FILE" ]; then
  mv "$MONTEREY_TRACKING_RESET_REQUEST" "$trial/reset-request-$segment"
  resetting=0
 fi
 sleep .2
done
if [ -n "$worker_pid" ] && [ "$(cut -d. -f1 /proc/uptime)" -lt "$deadline" ]; then
 wait "$worker_pid"
 worker_pid=
fi
# EXIT cleanup stops any remaining worker and restores the accepted scene.
