#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Windows-Build (x86_64, Windows 10/11) unter Linux mit llvm-mingw. Lädt die Quellen der
# Abhängigkeiten (Prüfsummen fest), baut sie nach build-win/deps und dann den Port nach
# build-win/port; Ergebnis mit DLLs in build-win/dist. docs/WINDOWS.md
# Aufruf: tools/windows/build.sh [--jobs N]
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
jobs=$(nproc)
[ "${1:-}" = "--jobs" ] && jobs=$2

: "${LLVM_MINGW:=$(ls -d "$HOME"/.local/opt/llvm-mingw-* 2>/dev/null | sort | tail -1)}"
[ -x "$LLVM_MINGW/bin/x86_64-w64-mingw32-clang" ] || { echo "llvm-mingw fehlt (LLVM_MINGW setzen)"; exit 1; }
export LLVM_MINGW
T=x86_64-w64-mingw32
export PATH="$LLVM_MINGW/bin:$PATH"

W="$root/build-win"
SRC="$W/src"; P="$W/deps"
mkdir -p "$SRC" "$P"
TC="$root/cmake/llvm-mingw.cmake"
CM="cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=$TC -DRAH_WIN_PREFIX=$P -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DCMAKE_PREFIX_PATH=$P"

. "$root/tools/deps/sources.sh"
fetch_sources

step() { [ -f "$P/.done-$1" ]; }
done_() { touch "$P/.done-$1"; }

if ! step zlib; then
    unpack zlib-1.3.1.tar.gz zlib
    $CM -S "$SRC/zlib" -B "$W/b-zlib" -DZLIB_BUILD_EXAMPLES=OFF
    cmake --build "$W/b-zlib" -j "$jobs" && cmake --install "$W/b-zlib"
    rm -f "$P/bin/zlib1.dll" "$P/lib/libzlib.dll.a"  # nur statisch
    cp "$P/lib/libzlibstatic.a" "$P/lib/libz.a"
    done_ zlib
fi
if ! step freetype; then
    unpack freetype-2.14.3.tar.xz freetype
    $CM -S "$SRC/freetype" -B "$W/b-freetype" -DBUILD_SHARED_LIBS=OFF -DFT_DISABLE_ZLIB=ON \
        -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON
    cmake --build "$W/b-freetype" -j "$jobs" && cmake --install "$W/b-freetype"
    done_ freetype
fi
if ! step sdl3; then
    unpack SDL3-3.4.18.tar.gz sdl3
    $CM -S "$SRC/sdl3" -B "$W/b-sdl3" -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
    cmake --build "$W/b-sdl3" -j "$jobs" && cmake --install "$W/b-sdl3"
    done_ sdl3
fi
if ! step unshield; then
    unpack unshield-1.6.2.tar.gz unshield
    $CM -S "$SRC/unshield" -B "$W/b-unshield" -DBUILD_STATIC=ON -DUSE_OUR_OWN_MD5=ON \
        -DZLIB_LIBRARY="$P/lib/libz.a" -DZLIB_INCLUDE_DIR="$P/include"
    cmake --build "$W/b-unshield" -j "$jobs" && cmake --install "$W/b-unshield"
    done_ unshield
fi
if ! step ffmpeg; then
    unpack ffmpeg-8.1.3.tar.gz ffmpeg
    mkdir -p "$W/b-ffmpeg"
    (cd "$W/b-ffmpeg" && "$SRC/ffmpeg/configure" --prefix="$P" --enable-cross-compile --target-os=mingw32 \
        --arch=x86_64 --cross-prefix=$T- --cc=$T-clang --cxx=$T-clang++ --ar=llvm-ar --ranlib=llvm-ranlib \
        --nm=llvm-nm --strip=llvm-strip --windres=$T-windres --pkg-config=pkg-config \
        --enable-shared --disable-static $FFMPEG_COMMON)
    make -C "$W/b-ffmpeg" -j "$jobs" && make -C "$W/b-ffmpeg" install
    done_ ffmpeg
fi

# Port
PKG_CONFIG_LIBDIR="$P/lib/pkgconfig" PKG_CONFIG_SYSROOT_DIR="" \
    $CM -S "$root" -B "$W/port" -DCMAKE_INSTALL_PREFIX="$W/dist" \
    -DZLIB_LIBRARY="$P/lib/libz.a" -DZLIB_INCLUDE_DIR="$P/include"
cmake --build "$W/port" -j "$jobs"

# Verteilung: EXE + FFmpeg-DLLs + C++-Laufzeit
D="$W/dist"
mkdir -p "$D"
cp "$W/port/rentahero.exe" "$D/"
cp "$P"/bin/av*.dll "$P"/bin/sw*.dll "$D/"
cp "$LLVM_MINGW/$T/bin/libc++.dll" "$LLVM_MINGW/$T/bin/libunwind.dll" "$D/"
cp "$root/LICENSE" "$D/LICENSE.txt"
# Lizenzen der mitgelieferten bzw. eingebundenen Bibliotheken
L="$D/licenses"
collect_licenses "$L"
cp "$LLVM_MINGW/LICENSE.TXT" "$L/llvm-mingw-libc++-libunwind.txt"
cat > "$D/THIRD-PARTY.txt" <<EOT
Rent-A-Hero (Port) – mitgelieferte Bibliotheken (Lizenztexte in licenses/)

FFmpeg 8.1.3 (LGPL-2.1-or-later) als DLLs: avcodec, avformat, avutil, swscale.
  Quelle: https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n8.1.3.tar.gz
  Unverändert gebaut mit: $(sed -n 's/^FFMPEG_CONFIGURATION=//p' "$W/b-ffmpeg/ffbuild/config.mak" 2>/dev/null | sed "s|$W|<build-win>|g")
  Die DLLs dürfen durch eigene Builds gleicher Version ersetzt werden.
SDL 3.4.18 (zlib-Lizenz), statisch.
FreeType 2.14.3 (FreeType License): Portions of this software are copyright © The FreeType
  Project (www.freetype.org). All rights reserved. Statisch.
unshield 1.6.2 (MIT), statisch.  zlib 1.3.1 (zlib-Lizenz), statisch.
Dear ImGui 1.92.9b (MIT), statisch (Startmenü).
stb_image 2.30 (MIT/Public Domain), statisch. libretro-Shader (Bildfilter, eingebettet):
  Lizenzköpfe in licenses/libretro-shaders.txt (MIT, GPL-2.0+, LGPL-2.1+, GPL-3.0, Public Domain).
libc++/libunwind aus llvm-mingw (Apache-2.0 mit LLVM-Ausnahme) als DLLs.
EOT
ver=$(git -C "$root" describe --always --dirty 2>/dev/null || echo unbekannt)
zip="$W/rentahero-win64-$ver.zip"
rm -f "$zip"
(cd "$D" && cmake -E tar cf "$zip" --format=zip .)
echo "fertig: $D"
echo "Archiv: $zip ($(du -h "$zip" | cut -f1))"
