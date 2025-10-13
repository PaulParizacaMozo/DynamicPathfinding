#!/usr/bin/env bash
set -euo pipefail

# === Paths ===
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR"     # fuentes .cpp están directamente en engines/
OUT_DIR="$SCRIPT_DIR/bin" # binarios saldrán en engines/bin/
mkdir -p "$OUT_DIR"

# === Toolchain ===
CXX="${CXX:-g++}"
CXXFLAGS="${CXXFLAGS:--O3 -std=c++17}"

# Extensión .exe si estás en MSYS/MinGW/Cygwin
EXE_EXT=""
uname_s="$(uname -s 2>/dev/null | tr '[:upper:]' '[:lower:]' || echo '')"
case "$uname_s" in
msys* | mingw* | cygwin*) EXE_EXT=".exe" ;;
esac

echo "[info] CXX=$CXX"
echo "[info] CXXFLAGS=$CXXFLAGS"
echo "[info] SRC_DIR=$SRC_DIR"
echo "[info] OUT_DIR=$OUT_DIR"
echo

build() {
  local name="$1" # nombre del binario (p.ej., dijkstra_)
  local src="$2"  # ruta del .cpp (p.ej., engines/dijkstra.cpp)

  if [[ ! -f "$src" ]]; then
    echo "[skip] $name -> no existe $src"
    return 0
  fi

  local out="$OUT_DIR/$name$EXE_EXT"
  echo "[build] $name  <=  $(realpath --relative-to="$SCRIPT_DIR" "$src" 2>/dev/null || echo "$src")"
  "$CXX" $CXXFLAGS "$src" -o "$out"
  chmod +x "$out" || true
  echo "[ok]    $out"
}

# === Compilar los que existan ===
build "dijkstra" "$SRC_DIR/dijkstra.cpp"
build "astar" "$SRC_DIR/astar.cpp"
build "d_star_lite" "$SRC_DIR/dstar_lite.cpp"
build "bmssp" "$SRC_DIR/bmssp.cpp"

echo
echo "[done] Binarios listos en: $OUT_DIR"
