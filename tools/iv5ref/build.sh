#!/bin/sh
# Baut iv5ref.exe (i686, ohne CRT) mit clang/lld-link nach $1 (Standard: Arbeitsordner)
set -e
here=$(dirname "$0"); out=${1:-.}
llvm-dlltool -m i386 -d "$here/kernel32.def" -l "$out/kernel32.lib" -k
clang --target=i686-pc-windows-msvc -O1 -ffreestanding -fno-stack-protector -fno-builtin -c "$here/iv5ref.c" -o "$out/iv5ref.obj"
lld-link /nologo /subsystem:console /entry:start /nodefaultlib /out:"$out/iv5ref.exe" "$out/iv5ref.obj" "$out/kernel32.lib"
