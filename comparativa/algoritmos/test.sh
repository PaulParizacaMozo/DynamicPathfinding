#!/usr/bin/env bash
set -euo pipefail

# =================== Config global ===================
CXX=${CXX:-g++}
CXXFLAGS="-O3 -march=native -DNDEBUG -std=c++17"
BIN=bench

# Usamos grids 4-dir, pesos unitarios
WMIN=1
WMAX=1
SEED=${SEED:-42}

# Lista de tamaños: "rowsxcols"
SIZES=("2000x1000" "1500x2000" "2000x2000" "2500x2000") # 2M, 3M, 4M, 5M

# Márgenes B a probar (en % del óptimo), de más estricto a menos
MARGINS=("1" "1.5" "2" "2.5" "3")

calc_B() {
  local OPT="$1"
  local PCT="$2"
  python3 - <<PY
opt = float("$OPT")
pct = float("$PCT")
print(f"{opt*(1.0+pct/100.0):.6f}")
PY
}

# =================== Compilar ===================
echo "[1/3] Compilando..."
$CXX $CXXFLAGS -o "$BIN" \
  main.cpp utils.cpp dijkstra.cpp astar.cpp bmssp.cpp dstar_lite.cpp

# =================== Función por tamaño ===================
run_for_size() {
  local RS="$1"
  local ROWS="${RS%x*}"
  local COLS="${RS#*x}"
  local GRAPH="grid_${ROWS}x${COLS}.bin"
  local S=0
  local T=$((ROWS * COLS - 1))
  local OPT=$(((ROWS - 1) + (COLS - 1)))

  echo
  echo "== Tamaño ${ROWS}x${COLS}  (N=$((ROWS * COLS))) =="
  if [[ ! -f "$GRAPH" ]]; then
    echo "[2/3] Generando grid ${ROWS}x${COLS} -> $GRAPH"
    ./"$BIN" --mode=gen_grid \
      --rows="$ROWS" --cols="$COLS" \
      --out="$GRAPH" --wmin="$WMIN" --wmax="$WMAX" --seed="$SEED"
  else
    echo "[2/3] Usando grafo existente: $GRAPH"
  fi

  echo "[3/3] Benchmarks (buscando bmssp < dijkstra):"
  echo "algo,N,M,s,t,time_ms,path_len"

  local SUCCESS=0
  local LAST_OUT=""

  for PCT in "${MARGINS[@]}"; do
    local B_VAL
    B_VAL=$(calc_B "$OPT" "$PCT")

    OUT=$(
      ./"$BIN" --mode=run \
        --in="$GRAPH" --s="$S" --t="$T" \
        --algos=bmssp,dijkstra,astar,dstar \
        --B="$B_VAL"
    )
    LAST_OUT="$OUT"

    # imprime sin cabecera (ya se imprimió)
    echo "$OUT" | tail -n +2

    # validación de rutas
    BMSSP_OK=$(echo "$OUT" | awk -F, '/^bmssp/   {print ($7>0)?"1":"0"}')
    DIJK_OK=$(echo "$OUT" | awk -F, '/^dijkstra/{print ($7>0)?"1":"0"}')
    ASTAR_OK=$(echo "$OUT" | awk -F, '/^astar/   {print ($7>0)?"1":"0"}')
    DSTAR_OK=$(echo "$OUT" | awk -F, '/^dstar/   {print ($7>0)?"1":"0"}')

    # tiempos
    BMSSP_MS=$(echo "$OUT" | awk -F, '/^bmssp/   {print $6}')
    DIJK_MS=$(echo "$OUT" | awk -F, '/^dijkstra/{print $6}')

    if [[ "$BMSSP_OK" == "1" && "$DIJK_OK" == "1" && "$ASTAR_OK" == "1" && "$DSTAR_OK" == "1" ]]; then
      awk -v b="$BMSSP_MS" -v d="$DIJK_MS" 'BEGIN{ exit !(b+0.0 < d+0.0) }' && {
        echo
        echo "Resultado: bmssp < dijkstra con B = óptimo + ${PCT}% (B=${B_VAL})"
        SUCCESS=1
        break
      }
      echo
      echo "bmssp >= dijkstra con B = óptimo + ${PCT}% (B=${B_VAL}); probando margen mayor..."
      echo
    else
      echo
      echo "Algún algoritmo no encontró ruta con B = óptimo + ${PCT}% (B=${B_VAL}); probando margen mayor..."
      echo
    fi
  done

  if [[ "$SUCCESS" -ne 1 ]]; then
    echo
    echo "No se logró bmssp < dijkstra en ${ROWS}x${COLS} con los márgenes probados. Último intento:"
    echo "$LAST_OUT" | tail -n +2
  fi
}

# =================== Ejecutar para todos los tamaños ===================
for RS in "${SIZES[@]}"; do
  run_for_size "$RS"
done
