# Build PLOC

## 0.7.0-rc1 / Hundekraft 3.3

Use the [Hundekraft 3.3 source SDK](https://hundekraft.github.io/op1-workshop/source/hundekraft-3.3-source.tar.gz) and a Blackfin ELF toolchain (GCC 13.3.0) on your PATH. From this directory:

```sh
PLOC_SDK_SOURCE=/absolute/path/to/hundekraft-3.3-source bash tools/build-standalone.sh
```

This builds only `output/hundekraft-3.3/ploc.knl`. The standalone manifest enables the engine input hook and reuses the original synth sources. The 3.1.4 build below still uses its original manifest and companion controller.

## 0.6.2 / Hundekraft 3.1.4

Start here to build the two plugin binaries for the original OP-1 with Hundekraft 3.1.4 (R246). Use the matching Hundekraft source SDK, including `ext/kennel`, and put a Blackfin ELF toolchain with `bfin-elf-gcc` on your PATH. The SDK's own README explains the packer requirements. Python 3 and Clang are used for the build and host checks. Neither the SDK, compiler nor OP-1 firmware is bundled here.

From this directory:

```sh
export PLOC_SDK_SOURCE=/absolute/path/to/hundekraft-3.1.4-source
bash tools/check.sh
bash tools/build.sh
```

The binaries are written to `output/ploc.knl` and `output/plocctrl.knl`. To regenerate the SVG and build a full release with validation and screen previews:

```sh
python3 tools/build_ploc6_release.py \
  --sdk-source "$PLOC_SDK_SOURCE" \
  --toolchain /absolute/path/to/blackfin-toolchain \
  --out ../local-release
```

The toolchain path must contain `bin/bfin-elf-gcc`; use an empty output directory. The preview uses a fallback font unless you pass your own `--font` and `--kern` inputs. No device connection is needed for either build.

The generated display SVG and its source vectors/layout are included. See [PLOC-GUIDE.md](PLOC-GUIDE.md) for the French control reference and [PROVENANCE.md](PROVENANCE.md) for attribution.
