#!/bin/sh
set -eu
export DISPLAY=:0 XDG_RUNTIME_DIR=/run/user/10000
export XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json
export XRT_COMPOSITOR_NULL=1 XRT_NO_STDIN=1 MONTEREY_LOG=info
if pgrep -x monado-service >/dev/null; then echo 'Existing Monado service; refusing second instance' >&2; exit 1; fi
monado-service > /tmp/monado-fov-service.log 2>&1 </dev/null &
service_pid=$!
cleanup() { kill -TERM "$service_pid" 2>/dev/null || :; wait "$service_pid" 2>/dev/null || :; }
trap cleanup EXIT
trap 'exit 130' HUP INT TERM
sleep 12
kill -0 "$service_pid"
"${0%/*}/monterey-head-fov" "$@"
