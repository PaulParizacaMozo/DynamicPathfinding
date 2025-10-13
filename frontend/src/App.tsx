import { useMemo, useRef, useState } from "react";
import CanvasGrid from "./components/CanvasGrid";
import Controls from "./components/Controls";
import Legend from "./components/Legend";
import { idx } from "./lib/state";
import type { AlgoKey, Cell, EditMode, Pt } from "./lib/state";
import { useAgent } from "./hooks/useAgent";

export default function App() {
  // Estado base del mundo
  const [rows, setRows] = useState(20);
  const [cols, setCols] = useState(30);
  const [density, setDensity] = useState(0.25);
  const [algo, setAlgo] = useState<AlgoKey>("dstar");
  const [mode, setMode] = useState<EditMode>("toggleObstacle");
  const [speedMs, setSpeedMs] = useState(50);

  // Comparación de algoritmos
  const [compareEnabled, setCompareEnabled] = useState(false);
  const [algo2, setAlgo2] = useState<AlgoKey>("astar");

  const [grid, setGrid] = useState<Cell[]>(() => new Array(20 * 30).fill(0) as Cell[]);
  const [start, setStart] = useState<Pt>({ r: 0, c: 0 });
  const [goal, setGoal] = useState<Pt>({ r: 19, c: 29 });

  // Hook del agente (animación y replan) - Algoritmo 1
  const {
    layers, agent, isPlaying, isBusy, status,
    play, pause, clearLayers, onObstacleChanged, onObstacleBatchChanged
  } = useAgent({ grid, rows, cols, start, goal, algo, speedMs });

  // Hook del agente - Algoritmo 2 (para comparación)
  const {
    layers: layers2, agent: agent2, isPlaying: isPlaying2, isBusy: isBusy2, status: status2,
    play: play2, pause: pause2, clearLayers: clearLayers2, 
    onObstacleChanged: onObstacleChanged2, onObstacleBatchChanged: onObstacleBatchChanged2
  } = useAgent({ grid, rows, cols, start, goal, algo: algo2, speedMs });

  // Rehacer grid con nuevas dimensiones
  useMemo(() => {
    const g = new Array(rows * cols).fill(0) as Cell[];
    g[idx(0, 0, cols)] = 0;
    g[idx(rows - 1, cols - 1, cols)] = 0;
    setGrid(g);
    setStart({ r: 0, c: 0 });
    setGoal({ r: rows - 1, c: cols - 1 });
  }, [rows, cols]);

  // Helpers grid
  const makeEmptyGrid = () => new Array(rows * cols).fill(0) as Cell[];
  const makeRandomGrid = () => {
    const g = new Array(rows * cols).fill(0) as Cell[];
    for (let i = 0; i < g.length; i++) g[i] = Math.random() < density ? 1 : 0;
    g[idx(start.r, start.c, cols)] = 0;
    g[idx(goal.r, goal.c, cols)] = 0;
    return g;
  };

  function handleEmpty() { 
    if ((!isBusy && !isPlaying) && (!compareEnabled || (!isBusy2 && !isPlaying2))) { 
      setGrid(makeEmptyGrid()); 
      clearLayers(); 
      if (compareEnabled) clearLayers2();
    } 
  }
  
  function handleRandom() { 
    if ((!isBusy && !isPlaying) && (!compareEnabled || (!isBusy2 && !isPlaying2))) { 
      setGrid(makeRandomGrid()); 
      clearLayers(); 
      if (compareEnabled) clearLayers2();
    } 
  }
  
  function handleClear() { 
    if ((!isBusy && !isPlaying) && (!compareEnabled || (!isBusy2 && !isPlaying2))) {
      clearLayers(); 
      if (compareEnabled) clearLayers2();
    }
  }

  // === Click simple: izquierda = colocar (1), derecha = borrar (0) ===
  async function onClickCell(r: number, c: number) {
    if (r < 0 || r >= rows || c < 0 || c >= cols) return;
    if (mode !== "toggleObstacle") return;
    if ((r === start.r && c === start.c) || (r === goal.r && c === goal.c)) return;

    setGrid(prev => {
      const next = prev.slice();
      const id = idx(r, c, cols);
      next[id] = 1; // colocar siempre
      onObstacleChanged(r, c, true).catch(() => { });
      if (compareEnabled) onObstacleChanged2(r, c, true).catch(() => { });
      return next as Cell[];
    });
  }

  async function onRightClickCell(r: number, c: number) {
    if (r < 0 || r >= rows || c < 0 || c >= cols) return;
    if (mode !== "toggleObstacle") return;
    if ((r === start.r && c === start.c) || (r === goal.r && c === goal.c)) return;

    setGrid(prev => {
      const next = prev.slice();
      const id = idx(r, c, cols);
      next[id] = 0; // borrar siempre
      onObstacleChanged(r, c, false).catch(() => { });
      if (compareEnabled) onObstacleChanged2(r, c, false).catch(() => { });
      return next as Cell[];
    });
  }

  // === Drag paint (mantener pulsado): herramienta según botón ===
  const paintingRef = useRef(false);
  const paintToStateRef = useRef<0 | 1>(1); // 1 = colocar, 0 = borrar
  const paintedSetRef = useRef<Set<number>>(new Set());

  function onStartPaint(r: number, c: number, button: number) {
    if (mode !== "toggleObstacle") return;
    if ((r === start.r && c === start.c) || (r === goal.r && c === goal.c)) return;

    paintingRef.current = true;
    paintedSetRef.current.clear();

    setGrid(prev => {
      const next = prev.slice();
      const id = idx(r, c, cols);

      // botón decide herramienta: izquierdo=colocar, derecho=borrar
      paintToStateRef.current = (button === 2) ? 0 : 1;

      next[id] = paintToStateRef.current;
      paintedSetRef.current.add(id);
      return next as Cell[];
    });
  }

  function onPaintOver(r: number, c: number, _button: number) {
    if (!paintingRef.current || mode !== "toggleObstacle") return;
    if ((r === start.r && c === start.c) || (r === goal.r && c === goal.c)) return;

    const id = idx(r, c, cols);
    if (paintedSetRef.current.has(id)) return;

    setGrid(prev => {
      const next = prev.slice();
      next[id] = paintToStateRef.current;
      paintedSetRef.current.add(id);
      return next as Cell[];
    });
  }

  async function onEndPaint(_button: number) {
    if (!paintingRef.current) return;
    paintingRef.current = false;

    if (paintedSetRef.current.size === 0) return;

    const blocked = paintToStateRef.current === 1; // true=colocar, false=borrar
    const changes = Array.from(paintedSetRef.current).map(id => {
      const r = Math.floor(id / cols);
      const c = id % cols;
      return { r, c, blocked };
    });

    await onObstacleBatchChanged(changes).catch(() => { });
    if (compareEnabled) await onObstacleBatchChanged2(changes).catch(() => { });
    paintedSetRef.current.clear();
  }

  // Mover nodos inicio/fin (fuera de toggleObstacle)
  async function onClickCellMove(r: number, c: number) {
    if (isBusy || isPlaying || (compareEnabled && (isBusy2 || isPlaying2))) return;

    if (mode === "moveStart" && !(r === goal.r && c === goal.c)) {
      setStart({ r, c });
      setGrid(prev => { const n = prev.slice(); n[idx(r, c, cols)] = 0; return n as Cell[]; });
      clearLayers();
      if (compareEnabled) clearLayers2();
    } else if (mode === "moveGoal" && !(r === start.r && c === start.c)) {
      setGoal({ r, c });
      setGrid(prev => { const n = prev.slice(); n[idx(r, c, cols)] = 0; return n as Cell[]; });
      clearLayers();
      if (compareEnabled) clearLayers2();
    }
  }

  // Router de click izquierdo según modo
  async function handleClickCell(r: number, c: number) {
    if (mode === "toggleObstacle") return onClickCell(r, c);
    return onClickCellMove(r, c);
  }

  return (
    <div style={{ display: "flex", gap: 16, padding: 16, fontFamily: "sans-serif", flexDirection: "column" }}>
      <div style={{ display: "flex", gap: 16, alignItems: "flex-start" }}>
        <div>
          <Controls
            rows={rows} cols={cols} density={density} algo={algo} mode={mode} speedMs={speedMs}
            isPlaying={isPlaying} isBusy={isBusy}
            compareEnabled={compareEnabled}
            algo2={algo2}
            onChangeRows={setRows}
            onChangeCols={setCols}
            onChangeDensity={setDensity}
            onChangeAlgo={(a) => { if (!isBusy && (!compareEnabled || !isBusy2)) setAlgo(a); }}
            onChangeMode={setMode}
            onChangeSpeed={setSpeedMs}
            onToggleCompare={(enabled) => {
              if (!isBusy && !isPlaying && !isBusy2 && !isPlaying2) {
                setCompareEnabled(enabled);
                if (!enabled) clearLayers2();
              }
            }}
            onChangeAlgo2={(a) => { if (!isBusy2 && compareEnabled) setAlgo2(a); }}
            onEmpty={handleEmpty}
            onRandom={handleRandom}
            onPlay={() => {
              play();
              if (compareEnabled) play2();
            }}
            onPause={() => {
              pause();
              if (compareEnabled) pause2();
            }}
            onClear={handleClear}
          />
          <div style={{ marginTop: 8, fontSize: 14 }}>
            <b>Estado:</b> {status}
          </div>
          {compareEnabled && (
            <div style={{ marginTop: 4, fontSize: 14 }}>
              <b>Estado (Comparación):</b> {status2}
            </div>
          )}
          <Legend />
        </div>

        <div>
          <div style={{ marginBottom: 8, fontSize: 14, fontWeight: "bold" }}>
            Algoritmo: {algo.toUpperCase()}
          </div>
          <CanvasGrid
            rows={rows}
            cols={cols}
            grid={grid}
            layers={layers}
            start={start}
            goal={goal}
            agent={isPlaying ? agent : undefined}
            onClickCell={handleClickCell}
            onRightClickCell={onRightClickCell}
            onStartPaint={onStartPaint}
            onPaintOver={onPaintOver}
            onEndPaint={onEndPaint}
          />
        </div>
      </div>

      {compareEnabled && (
        <div style={{ display: "flex", gap: 16, alignItems: "flex-start", marginLeft: 396 }}>
          <div>
            <div style={{ marginBottom: 8, fontSize: 14, fontWeight: "bold" }}>
              Algoritmo: {algo2.toUpperCase()}
            </div>
            <CanvasGrid
              rows={rows}
              cols={cols}
              grid={grid}
              layers={layers2}
              start={start}
              goal={goal}
              agent={isPlaying2 ? agent2 : undefined}
              onClickCell={handleClickCell}
              onRightClickCell={onRightClickCell}
              onStartPaint={onStartPaint}
              onPaintOver={onPaintOver}
              onEndPaint={onEndPaint}
            />
          </div>
        </div>
      )}
    </div>
  );
}

