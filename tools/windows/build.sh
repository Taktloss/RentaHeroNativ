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

# name|url|sha256
fetch() {
    name=$1 url=$2 sum=$3
    f="$SRC/$name"
    if [ ! -f "$f" ]; then
        curl -fsSL -o "$f.part" "$url"
        mv "$f.part" "$f"
    fi
    echo "$sum  $f" | sha256sum -c --quiet - || { echo "Prüfsumme falsch: $name"; exit 1; }
}
unpack() {  # archiv zielordner
    d="$SRC/$2"
    [ -d "$d" ] || { mkdir -p "$d.tmp" && tar xf "$SRC/$1" -C "$d.tmp" --strip-components=1 && mv "$d.tmp" "$d"; }
}

fetch zlib-1.3.1.tar.gz https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz \
    9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
fetch freetype-2.14.3.tar.xz https://download.savannah.gnu.org/releases/freetype/freetype-2.14.3.tar.xz \
    36bc4f1cc413335368ee656c42afca65c5a3987e8768cc28cf11ba775e785a5f
fetch SDL3-3.4.18.tar.gz https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz \
    9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
fetch unshield-1.6.2.tar.gz https://github.com/twogood/unshield/archive/refs/tags/1.6.2.tar.gz \
    a937ef596ad94d16e7ed2c8553ad7be305798dcdcfd65ae60210b1e54ab51a2f
fetch ffmpeg-8.1.3.tar.gz https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n8.1.3.tar.gz \
    09f990289327d3ebfedf4fc15cae3f884a65dab7a5aac7bbb5b01ff83900efd9

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
        --enable-shared --disable-static --disable-programs --disable-doc --disable-debug \
        --disable-everything --disable-network --disable-avdevice --disable-avfilter \
        --disable-swresample --disable-x86asm --disable-autodetect \
        --enable-demuxer=avi --enable-decoder=cinepak,indeo3,indeo5 --enable-swscale \
        --enable-small)
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
mkdir -p "$L"
cp "$SRC/ffmpeg/COPYING.LGPLv2.1" "$L/FFmpeg-LGPL-2.1.txt"
cp "$SRC/sdl3/LICENSE.txt" "$L/SDL3.txt"
cp "$SRC/freetype/docs/FTL.TXT" "$L/FreeType-FTL.txt"
cp "$SRC/unshield/LICENSE" "$L/unshield-MIT.txt"
cp "$SRC/zlib/LICENSE" "$L/zlib.txt"
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
libc++/libunwind aus llvm-mingw (Apache-2.0 mit LLVM-Ausnahme) als DLLs.
EOT
ver=$(git -C "$root" describe --always --dirty 2>/dev/null || echo unbekannt)
zip="$W/rentahero-win64-$ver.zip"
rm -f "$zip"
(cd "$D" && cmake -E tar cf "$zip" --format=zip .)
echo "fertig: $D"
echo "Archiv: $zip ($(du -h "$zip" | cut -f1))"
