#!/usr/bin/env bash
# Keep personal data (PII) out of this repo.
#   tools/check-pii.sh --staged   lines being committed (the pre-commit hook runs this)
#   tools/check-pii.sh --tree     every tracked file (CI runs this)
#
# Generic patterns catch the usual shapes: private LAN IPs, home-directory paths,
# personal email addresses, private keys. Strings with no generic shape (device
# serial, real name, ...) go one per line in the gitignored .pii-patterns file
# locally, or in the PII_PATTERNS secret in CI. Write the placeholders listed in
# local.env.example instead (<SERIAL>, <BUILD_HOST_IP>, /home/<user>, ...).
#
# Only file:line is printed, never the matched text, so the check itself does not
# copy personal data into terminals, CI logs or agent transcripts.
set -u
mode=${1:---staged}
cd "$(git rev-parse --show-toplevel)" || exit 2

generic='192\.168\.[0-9]+\.[0-9]+|(^|[^0-9.])10\.[0-9]+\.[0-9]+\.[0-9]+|/Users/[A-Za-z0-9._-]+|/home/[A-Za-z0-9._-]+|[A-Za-z0-9._%+-]+@(gmail|googlemail|yahoo|outlook|hotmail|live|icloud|me|proton|protonmail)\.(com|me|ch)|BEGIN [A-Z ]*PRIVATE KEY'
allow='maintainer="'   # upstream Alpine package maintainers: public attribution

exact=$(mktemp); trap 'rm -f "$exact"' EXIT
[ -f .pii-patterns ] && grep -v -e '^[[:space:]]*#' -e '^[[:space:]]*$' .pii-patterns >>"$exact"
[ -n "${PII_PATTERNS:-}" ] && printf '%s\n' "$PII_PATTERNS" | grep -v '^[[:space:]]*$' >>"$exact"

# Both modes produce "path:line:content".
case $mode in
  --staged)
    text=$(git diff --cached -U0 --no-color --diff-filter=ACMR -- . ':!tools/check-pii.sh' |
      awk '/^\+\+\+ /{ f = substr($0, 7); next }
           /^@@/    { if (match($0, /\+[0-9]+/)) n = substr($0, RSTART + 1, RLENGTH - 1); next }
           /^\+/    { print f ":" n ":" substr($0, 2); n++ }') ;;
  --tree)
    text=$(git grep -n -I -e '' -- . ':!tools/check-pii.sh') ;;
  *) echo "usage: $0 [--staged|--tree]" >&2; exit 2 ;;
esac

hits=$(printf '%s\n' "$text" | grep -E -- "$generic" | grep -v -- "$allow")
if [ -s "$exact" ]; then
  hits=$(printf '%s\n%s\n' "$hits" "$(printf '%s\n' "$text" | grep -F -f "$exact")")
fi
hits=$(printf '%s\n' "$hits" | sed '/^$/d' | cut -d: -f1,2 | sort -u)

if [ -n "$hits" ]; then
  echo "check-pii: personal data found. Replace it with a placeholder from local.env.example:" >&2
  printf '%s\n' "$hits" | sed 's/^/  /' >&2
  exit 1
fi
