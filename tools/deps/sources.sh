# SPDX-License-Identifier: GPL-3.0-or-later
# Gemeinsame Quellen der Abhängigkeiten für tools/windows/build.sh und tools/appimage/build.sh
# (gleiche Versionen wie der Linux-Build). Vor dem Einbinden SRC setzen.
# zlib entspricht der offiziellen Prüfsumme, die übrigen sind beim ersten Herunterladen festgelegt.

# name url sha256
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
fetch_sources() {
    mkdir -p "$SRC"
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
}
# Gemeinsamer Teil der FFmpeg-Konfiguration: nur AVI, Cinepak, Indeo 3/5, swscale (FILTERS/VIDEO.md)
FFMPEG_COMMON="--disable-programs --disable-doc --disable-debug --disable-everything --disable-network \
--disable-avdevice --disable-avfilter --disable-swresample --disable-x86asm --disable-autodetect \
--enable-demuxer=avi --enable-decoder=cinepak,indeo3,indeo5 --enable-swscale --enable-small"

# Lizenztexte der eingebundenen Bibliotheken und Shader nach $1 (root und SRC müssen gesetzt sein)
collect_licenses() {
    L=$1
    mkdir -p "$L"
    cp "$SRC/ffmpeg/COPYING.LGPLv2.1" "$L/FFmpeg-LGPL-2.1.txt"
    cp "$SRC/sdl3/LICENSE.txt" "$L/SDL3.txt"
    cp "$SRC/freetype/docs/FTL.TXT" "$L/FreeType-FTL.txt"
    cp "$SRC/unshield/LICENSE" "$L/unshield-MIT.txt"
    cp "$SRC/zlib/LICENSE" "$L/zlib.txt"
    cp "$root/third_party/imgui/LICENSE.txt" "$L/DearImGui-MIT.txt"
    # Eingebettete libretro-Shader: Lizenzköpfe der Dateien (MIT/GPL/LGPL/Public Domain) und stb_image
    {
        cat "$root/third_party/libretro-shaders/VERSION.txt"
        for f in $(cd "$root/third_party/libretro-shaders" && find . -name '*.glsl' | sort); do
            printf '\n===== %s =====\n' "${f#./}"
            # Kopf bis zur ersten Codezeile (#if/#define/uniform …)
            awk '/^[ \t]*(#if|#ifdef|#define|#pragma|uniform|varying|attribute|precision)/{exit} {print}' \
                "$root/third_party/libretro-shaders/$f" | head -90
        done
    } > "$L/libretro-shaders.txt"
    sed -n '/^ALTERNATIVE A/,$p' "$root/third_party/stb/stb_image.h" > "$L/stb_image.txt"
}
