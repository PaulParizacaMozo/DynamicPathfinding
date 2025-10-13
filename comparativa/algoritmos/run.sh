#!/usr/bin/env bash
set -euo pipefail

# -------- Parámetros simples (con defaults) --------
ROWS=${ROWS:-2000}    # o pasa --rows=...
COLS=${COLS:-1000}    # o pasa --cols=...
NODES=""              # si usas --nodes, se ignoran --rows/--cols
ASPECT=${ASPECT:-2:1} # usado solo con --nodes (rows:cols)
DIAG8=${DIAG8:-0}     # 0 = 4-dir, 1 = 8-dir
ALGS=${ALGS:-bmssp,dijkstra,astar,dstar}
SEED=${SEED:-42}
WMIN=${WMIN:-1}
WMAX=${WMAX:-1}
MARGIN_PCT=${MARGIN_PCT:-1.0} # B = OPT*(1+MARGIN_PCT/100)

# Admite flags tipo --rows=..., --cols=..., --nodes=..., etc. (opcional)
for a in "$@"; do
  case "$a" in
  --rows=*) ROWS="${a#*=}" ;;
  --cols=*) COLS="${a#*=}" ;;
  --nodes=*) NODES="${a#*=}" ;;
  --aspect=*) ASPECT="${a#*=}" ;;
  --diag8=*) DIAG8="${a#*=}" ;;
  --algs=*) ALGS="${a#*=}" ;;
  --seed=*) SEED="${a#*=}" ;;
  --wmin=*) WMIN="${a#*=}" ;;
  --wmax=*) WMAX="${a#*=}" ;;
  --margin-pct=*) MARGIN_PCT="${a#*=}" ;;
  *)
    echo "Opción desconocida: $a"
    exit 2
    ;;
  esac
done

# Si viene --nodes, derivar ROWS/COLS con el aspecto dado (muy simple)
if [[ -n "$NODES" ]]; then
  read ROWS COLS < <(
    python3 - <<PY
import math
N=int("$NODES"); a="$ASPECT"; ar,ac=map(float,a.split(":"))
r=int(math.sqrt(N*ar/ac)); c=max(1, N//max(1,r))
print(r, c)
PY
  )
fi

GRAPH="grid_${ROWS}x${COLS}${DIAG8:+_8}.bin"
BIN=bench
CXX=${CXX:-g++}
CXXFLAGS="-O3 -march=native -DNDEBUG -std=c++17"

# -------- Compilar --------
echo "[1/3] Compilando..."
$CXX $CXXFLAGS -o "$BIN" \
  main.cpp utils.cpp dijkstra.cpp astar.cpp bmssp.cpp dstar_lite.cpp

# -------- Generar grafo si no existe --------
if [[ ! -f "$GRAPH" ]]; then
  echo "[2/3] Generando grid ${ROWS}x${COLS} -> $GRAPH"
  ./"$BIN" --mode=gen_grid --rows="$ROWS" --cols="$COLS" \
    --out="$GRAPH" --wmin="$WMIN" --wmax="$WMAX" --seed="$SEED" \
    $([[ "$DIAG8" == "1" ]] && echo --diag8=1 || echo --diag8=0)
else
  echo "[2/3] Usando grafo existente: $GRAPH"
fi

# -------- Calcular B automáticamente --------
# Para grid 4-dir con pesos 1: OPT = (rows-1)+(cols-1).
# Si pesos != 1, usamos wmin como cota inferior conservadora.
B=$(
  python3 - <<PY
r=${ROWS}; c=${COLS}; pct=float("${MARGIN_PCT}")
wmin=float("${WMIN}"); wmax=float("${WMAX}")
base=(r-1)+(c-1)
if abs(wmin-1.0)>1e-9 or abs(wmax-1.0)>1e-9:
    base*=wmin
print(f"{base*(1.0+pct/100.0):.6f}")
PY
)

# -------- Ejecutar --------
S=0
T=$((ROWS * COLS - 1))
echo "[3/3] Ejecutando (diag8=${DIAG8}, w=[${WMIN},${WMAX}], B=${B}, seed=${SEED})"
echo "algo,N,M,s,t,time_ms,path_len"
./"$BIN" --mode=run --in="$GRAPH" --s="$S" --t="$T" --algos="$ALGS" --B="$B" | tail -n +2
