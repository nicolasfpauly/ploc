#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${PLOC_SDK_SOURCE:?Set PLOC_SDK_SOURCE to the extracted Hundekraft 3.3 source root}"
packer="$(cd "$PLOC_SDK_SOURCE" && pwd)/ext/kennel/tools/kennel/pack.py"
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
cp -R plugins/ploc "$stage/ploc"
cp "$stage/ploc/plugin-standalone.json" "$stage/ploc/plugin.json"
mkdir -p output/hundekraft-3.3
python3 "$packer" "$stage/ploc" -o output/hundekraft-3.3/ploc.knl
