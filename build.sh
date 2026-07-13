#!/bin/sh
# Build GBScribe.gba via the devkitPro Docker image.
# Usage: ./build.sh [make-args...]   e.g. ./build.sh clean
set -e
cd "$(dirname "$0")"
HOSTDIR="$(pwd -W 2>/dev/null || pwd)"
MSYS_NO_PATHCONV=1 docker run --rm -v "$HOSTDIR:/work" -w /work \
    devkitpro/devkitarm:latest make "$@"

# keep a copy of every successful build in dist/ (gitignored)
if [ -f GBScribe.gba ]; then
    mkdir -p dist
    cp GBScribe.gba dist/GBScribe.gba
fi
