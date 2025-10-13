# DynamicPathfinding

Visualizador 2D de **pathfinding** con animación basada en agente.
Algoritmos soportados: **Dijkstra**, **A***, **D* Lite** y **BMSSP**.

## Estructura

```
.
├── backend
│   ├── engines
│   │   ├── astar.cpp
│   │   ├── bmssp.cpp
│   │   ├── dijkstra.cpp
│   │   ├── dstar_lite.cpp
│   │   ├── bin/               # ejecutables C++ (se generan con run.sh)
│   │   └── run.sh             # compila a ./bin/
│   ├── src/index.js           # servidor Node: expone /api/*
│   ├── package.json
│   └── package-lock.json
└── frontend
    ├── src/
    │   ├── App.tsx            # orquestación simple
    │   ├── components/        # Canvas, Controles, Leyenda
    │   ├── hooks/useAgent.ts  # animación agente + replan
    │   └── lib/               # buildBody, parseOutput, tipos
    ├── index.html
    ├── package.json
    └── vite.config.ts
```

## Requisitos

* Node.js 18+
* Compilador C++ (g++/clang++) para construir los binarios

## Ejecución

1. Compilar los ejecutables C++:

```bash
cd backend/engines
chmod +x run.sh
./run.sh
# Genera: ./bin/dijkstra, ./bin/astar, ./bin/d_star_lite, ./bin/bmssp
```

2. Instalar dependencias del backend y arrancar:

```bash
cd ../
npm install
npm run dev
```

3. Instalar dependencias del frontend y arrancar:

```bash
cd ../frontend
npm install
npm run dev
```

## Uso rápido

* **Mapa vacío** o **Generar aleatorio** para crear el entorno.
* **Algoritmo**: D\*Lite , A*, Dijkstra, BMSSP.
* **Editar**:

  * Click izquierdo: **colocar** obstáculo.
  * Click derecho: **borrar** obstáculo.
  * Mantener y arrastrar: pinta en bloque según el botón con el que empiezas.
  * “Mover inicio / Mover fin”: cambia los nodos verde/rojo.
* **Play**: el agente (naranja) sigue el plan; el recorrido queda **azul oscuro** y el futuro **azul claro**.
* Al agregar obstáculos durante la ejecución:

  * **D* Lite** replanifica incrementalmente (mantiene su estado interno).
  * **Dijkstra / A* / BMSSP** recalculan solo **agente → objetivo** y continúan (no retroceden).

## Endpoints (backend)

* `POST /api/dijkstra`

* `POST /api/astar`

* `POST /api/bmssp`

  * Body:

    ```
    rows cols sr sc er ec
    <rows líneas de grilla 0/1 separadas por espacios>
    ```

  * Respuesta (texto):

    ```
    Visited:
    r c
    ...
    Parents:
    r c pr pc
    ...
    Path:
    r c
    ...
    ```

* **D\* Lite**

  * `POST /api/dstar/init` → inicializa con la grilla completa (mismo body).
  * `POST /api/dstar/move` → cuerpo: `r c` (mover agente; avanza km).
  * `POST /api/dstar/update` → cuerpo: una o varias líneas `r c cost`
    (usa `1e9` como “bloqueado”, `1` como libre). Devuelve nuevo plan en mismo formato de salida.
