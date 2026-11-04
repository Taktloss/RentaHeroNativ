#!/bin/sh
# Baut die Wine-Messprogramme (i686, ohne CRT) mit clang/lld-link nach $1 (Standard: .)
# Aufruf danach: WINEPREFIX=$PWD/wineprefix wine <ordner>/ddprobe.exe
set -e
here=$(dirname "$0"); out=${1:-.}
llvm-dlltool -m i386 -d "$here/../iv5ref/kernel32.def" -l "$out/kernel32.lib" -k
for p in ddprobe; do
  clang --target=i686-pc-windows-msvc -O1 -ffreestanding -fno-stack-protector -fno-builtin -c "$here/$p.c" -o "$out/$p.obj"
  lld-link /nologo /subsystem:console /entry:start /nodefaultlib /out:"$out/$p.exe" "$out/$p.obj" "$out/kernel32.lib"
done
