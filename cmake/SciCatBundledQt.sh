#!/usr/bin/env bash
set -euo pipefail

SCICAT_ROOT=/opt/scicat
export LD_LIBRARY_PATH="${SCICAT_ROOT}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export QT_PLUGIN_PATH="${SCICAT_ROOT}/plugins${QT_PLUGIN_PATH:+:${QT_PLUGIN_PATH}}"

exec "${SCICAT_ROOT}/bin/SciCat" "$@"
