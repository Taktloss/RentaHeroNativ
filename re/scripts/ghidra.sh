#!/usr/bin/env bash
# Führt ein Ghidra-Skript headless auf dem vorhandenen Projekt aus (ohne erneute Analyse).
# Aufruf: re/scripts/ghidra.sh <Skript.java> [argumente...]
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
script=$1; shift
JAVA_HOME=${JAVA_HOME:-$HOME/.local/opt/jdk-25.0.5+8} \
  "${GHIDRA_HOME:-$HOME/.local/opt/ghidra_12.2_PUBLIC}/support/analyzeHeadless" \
  "$root/re/private/ghidra" RahGame -process Rent-A-Hero.exe -noanalysis \
  -scriptPath "$root/re/scripts" -postScript "$script" "$@" 2>&1 \
  | grep -E '> |ERROR' | sed 's/ (GhidraScript)//'
