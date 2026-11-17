#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p output
PACK="${PLOC_SDK_SOURCE:-$PWD/sdk/hundekraft-3.1.4-source}/ext/kennel/tools/kennel/pack.py"
if [ -d "$PWD/.toolchain/bin" ]; then export PATH="$PWD/.toolchain/bin:$PATH"; fi
for name in ploc plocctrl; do
 python3 "$PACK" "plugins/$name" -o "output/$name.knl" --keep "output/$name-objects"
done
