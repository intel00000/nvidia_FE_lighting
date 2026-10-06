#!/bin/sh
# Login-time launcher for the NVIDIA FE lighting "apply on startup" feature.
#
# Usage: fe-lighting-startup.sh [--no-delay] SETTINGS_FILE
set -u

self=$0
if command -v readlink >/dev/null 2>&1; then
    resolved=$(readlink -f "$self" 2>/dev/null) && [ -n "$resolved" ] && self=$resolved
fi
here=$(cd "$(dirname "$self")" && pwd)

if [ -n "${FELIGHT_BIN:-}" ] && [ -x "${FELIGHT_BIN}" ]; then
    bin=$FELIGHT_BIN
elif [ -x "$here/felight" ]; then
    bin="$here/felight"
elif command -v felight >/dev/null 2>&1; then
    bin=$(command -v felight)
else
    echo "fe-lighting-startup: felight binary not found (\$FELIGHT_BIN, $here/felight, PATH)" >&2
    exit 2
fi

exec "$bin" startup "$@"
