#!/bin/sh
set -eu
exec 3>/dev/syncboss0
enabled=false
cleanup() {
    if "$enabled"; then printf '\157\000\000' >&3; enabled=false; fi
    exec 3>&-
}
trap cleanup EXIT
trap 'exit 0' HUP INT TERM
printf '\156\000\000' >&3
enabled=true
oculus-syncboss-dump -n 1024 -t 3000
