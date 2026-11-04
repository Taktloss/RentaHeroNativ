#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Linux-AppImage (x86_64). Baut im Container (Ubuntu 22.04, glibc 2.35; podman oder docker)
# die Abhängigkeiten aus den Quellen und den Port, Ergebnis: build-appimage/RentAHero-<git>-x86_64.AppImage
# docs/APPIMAGE.md.  Aufruf: tools/appimage/build.sh [--jobs N] [--no-container]
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
jobs=$(nproc); container=1
while [ $# -gt 0 ]; do
    case $1 in
        --jobs) jobs=$2; shift ;;
        --no-container) container=0 ;;
        *) echo "unbekannt: $1"; exit 1 ;;
    esac
    shift
done
if [ $container = 1 ]; then
    eng=$(command -v podman || command -v docker) || { echo "podman/docker fehlt (oder --no-container)"; exit 1; }
    img=rentahero-appimage-build:22.04
    "$eng" build -q -t $img -f "$root/tools/appimage/Containerfile" "$root/tools/appimage" >/dev/null
    ver=$(git -C "$root" describe --always --dirty 2>/dev/null || echo unbekannt)
    exec "$eng" run --rm -v "$root:/src:z" -e RAH_VERSION="$ver" -e HOME=/tmp \
        $( [ "$(basename "$eng")" = podman ] && echo --userns=keep-id ) -u "$(id -u):$(id -g)" \
        $img sh /src/tools/appimage/build.sh --jobs "$jobs" --no-container
fi

: "${RAH_VERSION:=$(git -C "$root" describe --always --dirty 2>/dev/null || echo unbekannt)}"
B="$root/build-appimage"
SRC="$B/src"; P="$B/deps"; A="$B/AppDir"
mkdir -p "$SRC" "$P"
CM="cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DCMAKE_PREFIX_PATH=$P -DCMAKE_POSITION_INDEPENDENT_CODE=ON"
. "$root/tools/deps/sources.sh"
fetch_sources
fetch appimagetool-1.9.1-x86_64.AppImage \
    https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage \
    ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0
fetch runtime-20251108-x86_64 https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-x86_64 \
    2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d

step() { [ -f "$P/.done-$1" ]; }
done_() { touch "$P/.done-$1"; }

if ! step zlib; then
    unpack zlib-1.3.1.tar.gz zlib
    $CM -S "$SRC/zlib" -B "$B/b-zlib" -DZLIB_BUILD_EXAMPLES=OFF
    cmake --build "$B/b-zlib" -j "$jobs" && cmake --install "$B/b-zlib"
    rm -f "$P"/lib/libz.so*  # nur statisch
    done_ zlib
fi
if ! step freetype; then
    unpack freetype-2.14.3.tar.xz freetype
    $CM -S "$SRC/freetype" -B "$B/b-freetype" -DBUILD_SHARED_LIBS=OFF -DFT_DISABLE_ZLIB=ON \
        -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON
    cmake --build "$B/b-freetype" -j "$jobs" && cmake --install "$B/b-freetype"
    done_ freetype
fi
if ! step sdl3; then
    # statisch; X11/Wayland/PipeWire/Pulse/ALSA/GL lädt SDL zur Laufzeit vom System nach
    unpack SDL3-3.4.18.tar.gz sdl3
    $CM -S "$SRC/sdl3" -B "$B/b-sdl3" -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
        -DSDL_X11=ON -DSDL_WAYLAND=ON -DSDL_PIPEWIRE=ON -DSDL_PULSEAUDIO=ON -DSDL_ALSA=ON
    cmake --build "$B/b-sdl3" -j "$jobs" && cmake --install "$B/b-sdl3"
    done_ sdl3
fi
if ! step unshield; then
    unpack unshield-1.6.2.tar.gz unshield
    $CM -S "$SRC/unshield" -B "$B/b-unshield" -DBUILD_STATIC=ON -DUSE_OUR_OWN_MD5=ON \
        -DZLIB_LIBRARY="$P/lib/libz.a" -DZLIB_INCLUDE_DIR="$P/include"
    cmake --build "$B/b-unshield" -j "$jobs" && cmake --install "$B/b-unshield"
    # eigene MD5-Objekte landen nicht in der installierten libunshield.a (nur $<TARGET_OBJECTS> beim Linken)
    ar rs "$P/lib/libunshield.a" $(find "$B/b-unshield" -path '*md5*' -name '*.o')
    done_ unshield
fi
if ! step ffmpeg; then
    # LGPL: als gemeinsame Bibliotheken im AppImage (austauschbar)
    unpack ffmpeg-8.1.3.tar.gz ffmpeg
    mkdir -p "$B/b-ffmpeg"
    (cd "$B/b-ffmpeg" && "$SRC/ffmpeg/configure" --prefix="$P" --enable-shared --disable-static \
        --enable-pic $FFMPEG_COMMON)
    make -C "$B/b-ffmpeg" -j "$jobs" && make -C "$B/b-ffmpeg" install
    done_ ffmpeg
fi

# Port: C++-Laufzeit statisch, FFmpeg aus ../lib
PKG_CONFIG_PATH="$P/lib/pkgconfig" PKG_CONFIG_LIBDIR="$P/lib/pkgconfig" \
    $CM -S "$root" -B "$B/port" -DRAH_UNSHIELD_STATIC=ON \
    -DZLIB_LIBRARY="$P/lib/libz.a" -DZLIB_INCLUDE_DIR="$P/include" \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc" \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON -DCMAKE_INSTALL_RPATH='$ORIGIN/../lib'
cmake --build "$B/port" -j "$jobs"
LD_LIBRARY_PATH="$P/lib" "$B/port/rah_tests" > "$B/tests.log" 2>&1 || { tail -20 "$B/tests.log"; exit 1; }
tail -1 "$B/tests.log"

# AppDir
rm -rf "$A"
mkdir -p "$A/usr/bin" "$A/usr/lib" "$A/usr/share/applications" "$A/usr/share/icons/hicolor/scalable/apps" \
    "$A/usr/share/doc/rentahero"
cp "$B/port/rentahero" "$A/usr/bin/"
strip "$A/usr/bin/rentahero"
for l in libavcodec libavformat libavutil libswscale; do
    cp -P "$P/lib/$l".so* "$A/usr/lib/"
done
strip --strip-unneeded "$A"/usr/lib/*.so.*.* 2>/dev/null || true
cp "$root/tools/appimage/rentahero.desktop" "$A/usr/share/applications/"
cp "$root/tools/appimage/rentahero.svg" "$A/usr/share/icons/hicolor/scalable/apps/"
cp "$root/tools/appimage/rentahero.desktop" "$root/tools/appimage/rentahero.svg" "$A/"
ln -s rentahero.svg "$A/.DirIcon"
ln -s usr/bin/rentahero "$A/AppRun"
D="$A/usr/share/doc/rentahero"
cp "$root/LICENSE" "$D/LICENSE.txt"
collect_licenses "$D/licenses"
cat > "$D/THIRD-PARTY.txt" <<EOT
Rent-A-Hero (Port) – enthaltene Bibliotheken (Lizenztexte in licenses/)

FFmpeg 8.1.3 (LGPL-2.1-or-later) als gemeinsame Bibliotheken in usr/lib: avcodec, avformat, avutil, swscale.
  Quelle: https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n8.1.3.tar.gz
  Unverändert gebaut mit: $(sed -n 's/^FFMPEG_CONFIGURATION=//p' "$B/b-ffmpeg/ffbuild/config.mak" 2>/dev/null | sed "s|$B|<build-appimage>|g")
  Die Bibliotheken dürfen durch eigene Builds gleicher Version ersetzt werden
  (AppImage mit --appimage-extract entpacken).
SDL 3.4.18 (zlib-Lizenz), statisch.
FreeType 2.14.3 (FreeType License): Portions of this software are copyright © The FreeType
  Project (www.freetype.org). All rights reserved. Statisch.
unshield 1.6.2 (MIT), statisch.  zlib 1.3.1 (zlib-Lizenz), statisch.
Dear ImGui 1.92.9b (MIT), statisch (Startmenü).
stb_image 2.30 (MIT/Public Domain), statisch. libretro-Shader (Bildfilter, eingebettet):
  Lizenzköpfe in licenses/libretro-shaders.txt (MIT, GPL-2.0+, LGPL-2.1+, GPL-3.0, Public Domain).
libstdc++/libgcc (GPL-3.0 mit GCC-Laufzeitausnahme), statisch.
EOT

# Nur Basisbibliotheken des Systems dürfen fehlen (glibc, libm …)
LD_LIBRARY_PATH="$A/usr/lib" ldd "$A/usr/bin/rentahero" | awk '{print $1}' | \
    grep -v -E '^(linux-vdso|/lib64/ld-linux|libc\.so|libm\.so|libpthread|libdl|librt|libav|libsw)' && \
    { echo "unerwartete Abhängigkeit"; exit 1; } || true

out="$B/RentAHero-$RAH_VERSION-x86_64.AppImage"
rm -f "$out"
chmod +x "$SRC/appimagetool-1.9.1-x86_64.AppImage"
ARCH=x86_64 APPIMAGE_EXTRACT_AND_RUN=1 "$SRC/appimagetool-1.9.1-x86_64.AppImage" --no-appstream \
    --runtime-file "$SRC/runtime-20251108-x86_64" "$A" "$out" > "$B/appimagetool.log" 2>&1 \
    || { tail -20 "$B/appimagetool.log"; exit 1; }
echo "AppImage: $out ($(du -h "$out" | cut -f1))"

# Dasselbe als einfaches Archiv (ohne AppImage): rentahero-<ver>/bin, lib, share
T="$B/tar/rentahero-$RAH_VERSION"
rm -rf "$B/tar"; mkdir -p "$T"
cp -a "$A/usr/bin" "$A/usr/lib" "$A/usr/share" "$T/"
cp "$root/LICENSE" "$T/LICENSE.txt"
cat > "$T/README.txt" <<EOT
Rent-A-Hero (Port) $RAH_VERSION für Linux x86_64 (glibc >= 2.35)
Start: bin/rentahero  – die Originaldaten (CDs) sind nicht enthalten, Auswahl im Startmenü.
Lizenzen: LICENSE.txt, share/doc/rentahero/
EOT
tgz="$B/rentahero-linux-x86_64-$RAH_VERSION.tar.gz"
tar -C "$B/tar" --owner=0 --group=0 -czf "$tgz" "rentahero-$RAH_VERSION"
echo "Archiv: $tgz ($(du -h "$tgz" | cut -f1))"
