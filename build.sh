#!/usr/bin/env bash
# Build do GamingFlix Ubisoft Offline Tool (cross-compile Linux -> Windows x64)
# Requer: mingw-w64 (x86_64-w64-mingw32-gcc, x86_64-w64-mingw32-windres)
set -euo pipefail

CC=x86_64-w64-mingw32-gcc
WINDRES=x86_64-w64-mingw32-windres
OUT="GamingFlix-Ubisoft-Offline-Tool.exe"

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

echo ">> compilando resource (manifest + versao)..."
"$WINDRES" -I res res/res.rc -O coff -o build_res.o

echo ">> compilando app..."
mkdir -p dist
"$CC" src/main.c build_res.o -o "dist/$OUT" \
  -mwindows -O2 -s \
  -ladvapi32 -lshell32 -lgdi32 -luser32 -lkernel32 -lmsimg32 -static

rm -f build_res.o
echo ">> pronto: dist/$OUT"
ls -lh "dist/$OUT"
