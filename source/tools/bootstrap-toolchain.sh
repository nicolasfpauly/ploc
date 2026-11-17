#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ROOT="${HUND_BUILD_ROOT:-$ROOT}"
SRC="$ROOT/.toolchain-src"
PREFIX="$ROOT/.toolchain"
mkdir -p "$SRC" "$PREFIX"
cd "$SRC"
for item in binutils:2.38 gcc:13.3.0; do
  name=${item%:*}; version=${item#*:}; file="$name-$version.tar.xz"
  if [ ! -f "$file" ]; then
    if [ "$name" = gcc ]; then url="https://ftp.gnu.org/gnu/gcc/gcc-$version/$file"; else url="https://ftp.gnu.org/gnu/binutils/$file"; fi
    curl --fail --location --retry 2 "$url" -o "$file"
  fi
  if [ ! -d "$name-$version" ]; then tar -xf "$file"; fi
done
export PATH="$PREFIX/bin:$PATH"
export CFLAGS="-O2 -Wno-error=implicit-function-declaration"
export CXXFLAGS="-O2"
if [ ! -x "$PREFIX/bin/bfin-elf-as" ]; then
 mkdir -p build-binutils
 cd build-binutils
 ../binutils-2.38/configure --target=bfin-elf --prefix="$PREFIX" --with-system-zlib --disable-nls --disable-werror --disable-gdb --disable-sim --disable-gprofng
 make -j8 MAKEINFO=true
 make install MAKEINFO=true
 cd ..
fi
if [ ! -f "$PREFIX/bfin-elf/lib/libgcc.a" ]; then
 mkdir -p build-gcc
 cd build-gcc
 ../gcc-13.3.0/configure --target=bfin-elf --prefix="$PREFIX" --with-system-zlib --enable-languages=c --without-headers --with-newlib --disable-nls --disable-shared --disable-threads --disable-libssp --disable-libquadmath --disable-libgomp --disable-libatomic --disable-multilib --disable-bootstrap --with-gmp=/opt/homebrew/opt/gmp --with-mpfr=/opt/homebrew/opt/mpfr --with-mpc=/opt/homebrew/opt/libmpc
 make -j8 all-gcc MAKEINFO=true
 make install-gcc MAKEINFO=true
 make -j8 all-target-libgcc MAKEINFO=true
 make install-target-libgcc MAKEINFO=true
fi
