#!/usr/bin/env bash
# Führt ein Ghidra-Skript headless auf dem vorhandenen Projekt aus (ohne erneute Analyse).
# Aufruf: re/scripts/ghidra.sh <Skript.java> [argumente...]
# Anderes Programm: RAH_GHIDRA_DIR (unter re/private), RAH_GHIDRA_PROJ, RAH_GHIDRA_PROG
# (z. B. ir50_32.dll: iv5/ghidra, Indeo5, ir50_32.dll)
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
script=$1; shift
JAVA_HOME=${JAVA_HOME:-$HOME/.local/opt/jdk-25.0.5+8} \
  "${GHIDRA_HOME:-$HOME/.local/opt/ghidra_12.2_PUBLIC}/support/analyzeHeadless" \
  "$root/re/private/${RAH_GHIDRA_DIR:-ghidra}" "${RAH_GHIDRA_PROJ:-RahGame}" \
  -process "${RAH_GHIDRA_PROG:-Rent-A-Hero.exe}" -noanalysis \
  -scriptPath "$root/re/scripts" -postScript "$script" "$@" 2>&1 \
  | grep -E '> |ERROR' | sed 's/ (GhidraScript)//'
