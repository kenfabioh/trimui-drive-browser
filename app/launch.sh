#!/bin/sh
# No firmware modifications, CPU/governor overrides or global library exports.
set -eu
APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$APP_DIR"
umask 077
if ! awk '$2 == "/mnt/SDCARD" { found=1 } END { exit !found }' /proc/mounts; then
    echo 'Drive Browser: microSD nao esta montado em /mnt/SDCARD.' >&2
    exit 1
fi
mkdir -p data/cache data/tmp
export XDG_CACHE_HOME="$APP_DIR/data/cache"
export TMPDIR="$APP_DIR/data/tmp"
# App dependencies first; retain firmware GPU libraries and CrossMix extras.
export LD_LIBRARY_PATH="$APP_DIR/lib:/usr/trimui/lib:/mnt/SDCARD/System/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
if [ -f config/rclone.conf ]; then chmod 600 config/rclone.conf 2>/dev/null || :; fi
if [ "$(uname -m)" != "aarch64" ]; then
    echo 'Drive Browser: este pacote requer Linux AArch64.' >&2
    exit 1
fi
exec ./bin/drive-browser "$@"
