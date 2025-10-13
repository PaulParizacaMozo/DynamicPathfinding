import type { AlgoKey, EditMode } from "../lib/state";

type Props = {
  rows: number; cols: number; density: number; algo: AlgoKey; mode: EditMode; speedMs: number;
  isPlaying: boolean; isBusy: boolean;
  compareEnabled: boolean;
  algo2: AlgoKey;
  onChangeRows: (v: number) => void;
  onChangeCols: (v: number) => void;
  onChangeDensity: (v: number) => void;
  onChangeAlgo: (a: AlgoKey) => void;
  onChangeMode: (m: EditMode) => void;
  onChangeSpeed: (ms: number) => void;
  onToggleCompare: (enabled: boolean) => void;
  onChangeAlgo2: (a: AlgoKey) => void;
  onEmpty: () => void;
  onRandom: () => void;
  onPlay: () => void;
  onPause: () => void;
  onClear: () => void;
};

export default function Controls({
  rows, cols, density, algo, mode, speedMs, isPlaying, isBusy,
  compareEnabled, algo2,
  onChangeRows, onChangeCols, onChangeDensity, onChangeAlgo, onChangeMode, onChangeSpeed,
  onToggleCompare, onChangeAlgo2,
  onEmpty, onRandom, onPlay, onPause, onClear,
}: Props) {
  return (
    <div style={{ minWidth: 380, display: "flex", flexDirection: "column", gap: 8 }}>
      <h2 style={{ margin: 0 }}>DynamicPathfinding</h2>

      <label>
        <input 
          type="checkbox" 
          checked={compareEnabled} 
          onChange={(e) => onToggleCompare(e.target.checked)}
          disabled={isBusy || isPlaying}
        />
        &nbsp;Comparar algoritmos
      </label>

      <label>
        Algoritmo 1:&nbsp;
        <select value={algo} onChange={(e) => onChangeAlgo(e.target.value as AlgoKey)} disabled={isBusy || isPlaying}>
          <option value="dstar">D* Lite</option>
          <option value="astar">A*</option>
          <option value="dijkstra">Dijkstra</option>
          <option value="bmssp">BMSSP</option>
        </select>
      </label>

      {compareEnabled && (
        <label>
          Algoritmo 2:&nbsp;
          <select value={algo2} onChange={(e) => onChangeAlgo2(e.target.value as AlgoKey)} disabled={isBusy || isPlaying}>
            <option value="dstar">D* Lite</option>
            <option value="astar">A*</option>
            <option value="dijkstra">Dijkstra</option>
            <option value="bmssp">BMSSP</option>
          </select>
        </label>
      )}

      <div style={{ display: "flex", gap: 8 }}>
        <label style={{ flex: 1 }}>
          Filas:
          <input type="number" min={2} value={rows} onChange={(e) => onChangeRows(Math.max(2, parseInt(e.target.value || "2", 10)))} disabled={isBusy || isPlaying} />
        </label>
        <label style={{ flex: 1 }}>
          Columnas:
          <input type="number" min={2} value={cols} onChange={(e) => onChangeCols(Math.max(2, parseInt(e.target.value || "2", 10)))} disabled={isBusy || isPlaying} />
        </label>
      </div>

      <div style={{ display: "flex", gap: 8 }}>
        <label style={{ flex: 1 }}>
          Densidad:
          <input type="number" step={0.05} min={0} max={1} value={density} onChange={(e) => onChangeDensity(Math.min(1, Math.max(0, parseFloat(e.target.value || "0"))))} disabled={isBusy || isPlaying} />
        </label>
        <label style={{ flex: 1 }}>
          Velocidad (ms):
          <input type="number" min={0} step={10} value={speedMs} onChange={(e) => onChangeSpeed(Math.max(0, parseInt(e.target.value || "0", 10)))} disabled={isBusy} />
        </label>
      </div>

      <div style={{ display: "flex", gap: 8, marginTop: 4, flexWrap: "wrap" }}>
        <button onClick={onEmpty} disabled={isBusy || isPlaying}>Mapa vacío</button>
        <button onClick={onRandom} disabled={isBusy || isPlaying}>Generar aleatorio</button>
        <button onClick={onPlay} disabled={isBusy || isPlaying}>Play</button>
        <button onClick={onPause} disabled={!isPlaying}>Pause</button>
        <button onClick={onClear} disabled={isBusy || isPlaying}>Limpiar</button>
      </div>

      <fieldset style={{ marginTop: 8 }}>
        <legend>Modo de edición (click izquierdo)</legend>
        <label><input type="radio" name="mode" value="toggleObstacle" checked={mode === "toggleObstacle"} onChange={() => onChangeMode("toggleObstacle")} /> Obstáculos</label> &nbsp;
        <label><input type="radio" name="mode" value="moveStart" checked={mode === "moveStart"} onChange={() => onChangeMode("moveStart")} /> Mover inicio</label> &nbsp;
        <label><input type="radio" name="mode" value="moveGoal" checked={mode === "moveGoal"} onChange={() => onChangeMode("moveGoal")} /> Mover fin</label>
      </fieldset>
    </div>
  );
}
