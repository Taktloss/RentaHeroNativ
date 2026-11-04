# SPDX-License-Identifier: GPL-3.0-or-later
# Cross-Build für Windows x86_64 mit llvm-mingw (UCRT). Pfad zur Toolchain über LLVM_MINGW
# (Umgebung oder -D), Standard ~/.local/opt/llvm-mingw-*. docs/WINDOWS.md
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(NOT LLVM_MINGW)
  set(LLVM_MINGW "$ENV{LLVM_MINGW}")
endif()
if(NOT LLVM_MINGW)
  file(GLOB _cands "$ENV{HOME}/.local/opt/llvm-mingw-*")
  list(SORT _cands)
  list(REVERSE _cands)
  list(GET _cands 0 LLVM_MINGW)
endif()
set(_triple x86_64-w64-mingw32)
set(CMAKE_C_COMPILER "${LLVM_MINGW}/bin/${_triple}-clang")
set(CMAKE_CXX_COMPILER "${LLVM_MINGW}/bin/${_triple}-clang++")
set(CMAKE_RC_COMPILER "${LLVM_MINGW}/bin/${_triple}-windres")
set(CMAKE_AR "${LLVM_MINGW}/bin/llvm-ar")
set(CMAKE_RANLIB "${LLVM_MINGW}/bin/llvm-ranlib")

set(CMAKE_FIND_ROOT_PATH "${LLVM_MINGW}/${_triple}" ${RAH_WIN_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
