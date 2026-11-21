#!/usr/bin/env bash
# Plays a UI walk against the fake SciCat and prints the driver's log. Nothing appears on the screen.
#   run_walk.sh <build dir> <walk.json> <output dir>
# Exit code: the number of failed steps. Nothing outside 127.0.0.1 is contacted; settings live in the output folder.
#
# WALK_MODE=ghost (default): a real window of this desktop with its real render loop, but fully transparent, never
#   active and click-through. What a user runs, minus the picture on the screen; the screenshots come from here.
# WALK_MODE=offscreen: Qt's offscreen platform, drawn in software. For machines without a desktop session; the
#   screenshots are not the reference for looks.
# WALK_DATA=<folder>: archive the files of this folder instead of the generated sample data.
# WALK_REAL_SERVERS=1: do not point the application at the fake SciCat. Only for a walk that looks at the sign-in
#   page and never signs in (walk-servers.json): nothing is sent as long as no token is submitted.
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
build="${1:?usage: run_walk.sh <build dir> <walk.json> <output dir>}"; script="${2:?walk.json}"; out="${3:?output dir}"
# The fake SciCat and the stand-ins for scicat-cli and klist are the ones of the scenario tests (tests/parity).
# The fake picks a free port itself and says which, so another program on a fixed port can never answer in its place.
parity="$(cd "$here/../../../tests/parity" && pwd)"
mkdir -p "$out"; rm -f "$out"/*.png "$out/log.txt" "$out/backend.log" "$out/backend.port"; rm -rf "$out/scicat-log"
FAKE_SCICAT_LOG="$out/backend.log" python3 -I "$parity/fake_scicat.py" "${FAKE_SCICAT_PORT:-0}" "${FAKE_SCICAT_LATENCY:-20}" \
  >"$out/backend.port" 2>"$out/backend.stderr" &
backend=$!
trap 'kill $backend 2>/dev/null' EXIT
port=""
for _ in $(seq 1 100); do
  kill -0 $backend 2>/dev/null || break
  port="$(head -n 1 "$out/backend.port" 2>/dev/null)"; [ -n "$port" ] && break
  sleep 0.1
done
if [ -z "$port" ] || ! kill -0 $backend 2>/dev/null; then
  echo "the fake SciCat did not start:"; cat "$out/backend.stderr"; exit 101
fi
printf '[General]\nversion=1.0\nserverID=1\nlogPath=%s\n' "$out/scicat-log" > "$out/settings.ini"
# a small dataset to archive; @DATA@ and @OUT@ in the walk stand for these folders
rm -rf "$out/copied"
if [ -n "${WALK_DATA:-}" ]; then data="$WALK_DATA"; else
  data="$out/sample-data"; mkdir -p "$data/scan_0001" "$data/scan_0002"
  for f in scan_0001/frame_0001.h5 scan_0001/frame_0002.h5 scan_0002/frame_0001.h5 notes.txt; do [ -f "$data/$f" ] || echo "demo" > "$data/$f"; done
fi
# scientific metadata to import: one good file, one that is not JSON
printf '{"beamline": {"name": "X12SA", "energy": 6.2}, "cooled": false}\n' > "$out/import-good.json"; printf 'not json\n' > "$out/import-bad.json"
sed -e "s|@DATA@|$data|g" -e "s|@OUT@|$out|g" "$script" > "$out/walk.resolved.json"; script="$out/walk.resolved.json"
# FAKE_NO_TICKET: the stand-in klist reports no Kerberos ticket, so the walk meets the "ticket needed" dialog
case "${WALK_MODE:-ghost}" in
  offscreen) export QT_QPA_PLATFORM=offscreen ;;
  *) export SCICAT_QML_HIDDEN=1 SCICAT_E2E_GHOST=1 ;;
esac
[ -n "${WALK_REAL_SERVERS:-}" ] || export SCICAT_QML_API="http://127.0.0.1:$port/api/"
PATH="$parity/bin:$PATH" FAKE_CLI_STEP="${FAKE_CLI_STEP:-0.15}" FAKE_CLI_INGEST_PAUSE="${FAKE_CLI_INGEST_PAUSE:-3}" FAKE_NO_TICKET=1 \
  SCICAT_QML_SETTINGS="$out/settings.ini" \
  SCICAT_WINDOW_TAG="${SCICAT_WINDOW_TAG:-e2e}" SCICAT_E2E_SCRIPT="$script" SCICAT_E2E_OUT="$out" \
  QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1 "$build/tests/e2e/scicat_qml_e2e" >"$out/stdout.txt" 2>&1
status=$?
cat "$out/log.txt" 2>/dev/null
[ -d "$out/copied" ] && echo "files written by the copy: $(find "$out/copied" -type f | wc -l | tr -d ' ')"
grep -v "^$" "$out/stdout.txt" | head -40
exit $status
