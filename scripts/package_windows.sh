#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

rm -rf dist/windows
mkdir -p dist/windows

make clean
make win WIN_CXX="${WIN_CXX:-g++}"

cp world.exe dist/windows/
cp README.md dist/windows/

while IFS= read -r dll; do
  if [[ -f "$dll" ]]; then
    cp -n "$dll" dist/windows/
  fi
done < <(ldd world.exe | awk '/ucrt64|mingw64|mingw32/ {print $3}' | sort -u)

cd dist
rm -f TheWorld-Windows.zip
zip -r TheWorld-Windows.zip windows
