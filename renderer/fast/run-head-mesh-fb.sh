#!/bin/sh
# VR-only diagnostic: own the display without launching a desktop.
# Preserve the guarded boot's recovery watchdog.
set -eu
export XDG_RUNTIME_DIR=/run/user/10000
export XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json
export XRT_COMPOSITOR_NULL=1 XRT_NO_STDIN=1 MONTEREY_LOG=info
export MONTEREY_FRAMEBUFFER=1
install -d -m 0700 -o 10000 -g 10000 "$XDG_RUNTIME_DIR"
if pgrep -x monado-service >/dev/null; then echo 'Existing Monado service; refusing second instance' >&2; exit 1; fi
service_pid=
demo_pid=
cleanup() {
 if [ -n "$demo_pid" ]; then kill -TERM "$demo_pid" 2>/dev/null || :; wait "$demo_pid" 2>/dev/null || :; fi
 if [ -n "$service_pid" ]; then kill -TERM "$service_pid" 2>/dev/null || :; wait "$service_pid" 2>/dev/null || :; fi
}
trap cleanup EXIT
trap 'exit 130' HUP INT TERM
if rc-service oculus-desktop-trial status >/dev/null 2>&1; then
 rc-service oculus-desktop-trial stop
 # OpenRC's launcher can exit before its Xorg child. End that display cleanly.
 for pid in $(pidof Xorg || :); do kill -TERM "$pid"; done
 sleep 2
fi
if pgrep -x Xorg >/dev/null; then echo 'Display still owned by Xorg; refusing framebuffer access' >&2; exit 1; fi
if [ "${1:-}" != --static ]; then
 : > /tmp/monado-mesh-fb-service.log
 monado-service >/tmp/monado-mesh-fb-service.log 2>&1 </dev/null &
 service_pid=$!
 # Wait for real gravity initialization instead of assuming twelve seconds.
 attempt=0
 while ! grep -q 'Gravity initialized' /tmp/monado-mesh-fb-service.log; do
  kill -0 "$service_pid"
  attempt=$((attempt+1))
  [ "$attempt" -le 240 ] || { echo 'Tracking did not settle within 60 seconds' >&2; exit 1; }
  sleep 0.25
 done
 kill -0 "$service_pid"
fi
# Optional external readiness gate: calibrate at rest before wearer puts it on.
# Caller must choose a fresh path and create it only after the wearer is ready.
if [ -n "${MONTEREY_START_FILE:-}" ]; then
 while [ ! -e "$MONTEREY_START_FILE" ]; do
  if [ -n "$service_pid" ]; then kill -0 "$service_pid"; fi
  sleep 0.2
 done
fi
"${0%/*}/monterey-head-mesh-fb" "$@" &
demo_pid=$!
wait "$demo_pid"
