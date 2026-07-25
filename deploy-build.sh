#!/usr/bin/env bash

set -euo pipefail

# Usage: ./copy_build.sh <destination_dir>
DEST="${1:-}"

if [[ -z "$DEST" ]]; then
  echo "Usage: $0 <destination_dir>"
  exit 1
fi

# Create destination directories
mkdir -p "$DEST/kmod"

# Copy kernel module
if [[ -f build/kmod/mtdpartikr.ko ]]; then
  cp -v build/kmod/mtdpartikr.ko "$DEST/kmod/"
else
  echo "Warning: build/kmod/mtdpartikr.ko not found"
fi

echo "Done copying build artifacts to $DEST"
