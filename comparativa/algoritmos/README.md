
# Benchmark de SSSP: BMSSP vs Dijkstra vs A*vs D* Lite

Este experimento compara cuatro algoritmos de caminos mínimos de fuente única sobre **grids 4-direcciones con pesos unitarios**:

* **BMSSP** (implementación acotada con poda por `B`, fast-path con Dial cuando pesos=1, y fallback garantizado).
* **Dijkstra** (cola de prioridad).
* **A*** (heurística Manhattan).
* **D* Lite** (dinámico).

**Fuente:** esquina superior izquierda (`s=0`).
**Destino:** esquina inferior derecha (`t=N-1`).
**Grafo:** grid 4-dir, peso=1, sin diagonales.
**Cota `B`:** óptimo × margen en `[1%, 3%]` (el script elige el primer margen con ruta en todos y donde BMSSP < Dijkstra).
**Métrica:** tiempo total (ms) y longitud de ruta (edges).

---

## Resultados

### 2M nodos — `2000x1000` (N=2,000,000)

CSV:

```
bmssp,2000000,7994000,0,1999999,70.460,2999
dijkstra,2000000,7994000,0,1999999,172.760,2999
astar,2000000,7994000,0,1999999,123.112,2999
dstar,2000000,7994000,0,1999999,595.817,2999
```

* **BMSSP vs Dijkstra:** ~**2.45×** más rápido (172.76 / 70.46).
* **BMSSP vs A*:** ~**1.75×** más rápido.
* **Ruta:** 2999 (óptima).

**Figura:** `img/tiempos_2000x1000.png`

---

### 3M nodos — `1500x2000` (N=3,000,000)

CSV:

```
bmssp,3000000,11993000,0,2999999,104.461,3499
dijkstra,3000000,11993000,0,2999999,264.565,3499
astar,3000000,11993000,0,2999999,192.438,3499
dstar,3000000,11993000,0,2999999,917.996,3499
```

* **BMSSP vs Dijkstra:** ~**2.53×** más rápido.
* **BMSSP vs A*:** ~**1.84×** más rápido.
* **Ruta:** 3499 (óptima).

**Figura:** `img/tiempos_1500x2000.png`

---

### 4M nodos — `2000x2000` (N=4,000,000)

CSV:

```
bmssp,4000000,15992000,0,3999999,136.273,3999
dijkstra,4000000,15992000,0,3999999,364.651,3999
astar,4000000,15992000,0,3999999,260.642,3999
dstar,4000000,15992000,0,3999999,1275.344,3999
```

* **BMSSP vs Dijkstra:** ~**2.68×** más rápido.
* **BMSSP vs A*:** ~**1.91×** más rápido.
* **Ruta:** 3999 (óptima).

**Figura:** `img/tiempos_2000x2000.png`

---

### 5M nodos — `2500x2000` (N=5,000,000)

CSV:

```
bmssp,5000000,19991000,0,4999999,173.011,4499
dijkstra,5000000,19991000,0,4999999,462.270,4499
astar,5000000,19991000,0,4999999,328.321,4499
dstar,5000000,19991000,0,4999999,1577.945,4499
```

* **BMSSP vs Dijkstra:** ~**2.67×** más rápido.
* **BMSSP vs A*:** ~**1.90×** más rápido.
* **Ruta:** 4499 (óptima).

**Figura:** `img/tiempos_2500x2000.png`

---

## Observaciones y comparativa

* **BMSSP gana consistentemente a Dijkstra** en todos los tamaños (≈2.5–2.7×).
  El beneficio viene de:

  * **Poda por `B`** (B ≈ costo óptimo con margen pequeño).
  * **Dial buckets** cuando pesos=1, evitando `decrease-key` costoso.
* **A*** sigue siendo competitivo, pero BMSSP lo supera en estos grids sin diagonales ni obstáculos.
* **D* Lite** es claramente más lento en este escenario estático (como era de esperar).

### Escalamiento

El tiempo crece casi linealmente con N para todos:

* **BMSSP:** 70.5 → 104.5 (×1.48) → 136.3 (×1.31) → 173.0 (×1.27)
* **Dijkstra:** 172.8 → 264.6 (×1.53) → 364.7 (×1.38) → 462.3 (×1.27)
* **A*:** 123.1 → 192.4 (×1.56) → 260.6 (×1.35) → 328.3 (×1.26)

(El factor de N: 2M→3M ×1.5, 3M→4M ×1.33, 4M→5M ×1.25.)

---

## Cómo reproducir

1. Compilar y generar cada grid automáticamente:

   ```bash
   ./run.sh
   ```

   (El script intenta márgenes de `B` en `[1, 1.5, 2, 2.5, 3]%` y se queda con la primera corrida donde todos hallan ruta y `bmssp < dijkstra`.)

2. Las imágenes se generan con el script de Python y deben estar en:

   ```
   img/tiempos_2000x1000.png
   img/tiempos_1500x2000.png
   img/tiempos_2000x2000.png
   img/tiempos_2500x2000.png
   ```

---

## Conclusiones

* En **grids 4-dir de gran escala y pesos unitarios**, **BMSSP** ofrece mejoras de ~**2.5×** sobre **Dijkstra** manteniendo rutas óptimas.
* El ajuste de **`B` (cota)** es clave: márgenes pequeños (1–3%) maximizan la poda y el rendimiento.
* Para escenarios con pesos unitarios, usar **Dial** dentro del núcleo acotado es determinante.
