#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p output
PLOC_SDK="${PLOC_SDK_SOURCE:-$PWD/sdk/hundekraft-3.1.4-source}/ext/kennel/synth_sdk"
for test in dsp ploc3 ploc4 ploc_surface ploc_adapter plocctrl ploc_screen ploc_legacy_screen; do
 clang -std=c11 -O2 -Wall -Wextra -Werror -Wno-unknown-pragmas -fsanitize=address,undefined -fno-sanitize-recover=all -I"$PLOC_SDK" "tests/test_$test.c" -lm -o "output/test-$test"
 "output/test-$test"
done
clang --target=armv7-none-eabi -std=c11 -ffreestanding -fsyntax-only -Wno-unknown-pragmas -I"$PLOC_SDK" plugins/ploc/ploc.c plugins/ploc/screen.c plugins/ploc/screen_legacy.c plugins/plocctrl/plocctrl.c
python3 vendor/op1-design-kit/skills/op1-design/scripts/check_svg.py plugins/ploc/ploc.svg
