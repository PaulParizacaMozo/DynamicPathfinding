import { useEffect, useRef, useState } from "react";
import type { AlgoKey, Cell, Layers, Pt } from "../lib/state";
import { idx } from "../lib/state";
import { buildBody, dstarInit, dstarMove, dstarUpdate, runOneShot } from "../lib/pathApi";

type UseAgentParams = {
  grid: Cell[];
  rows: number;
  cols: number;
  start: Pt;
  goal: Pt;
  algo: AlgoKey;
  speedMs: number;
};

type UseAgentReturn = {
  layers: Layers;
  agent: Pt;
  isPlaying: boolean;
  isBusy: boolean;
  status: string;
  play: () => Promise<void>;
  pause: () => void;
  clearLayers: () => void;
  onObstacleChanged: (r: number, c: number, blocked: boolean) => Promise<void>;
  onObstacleBatchChanged: (changes: { r: number; c: number; blocked: boolean }[]) => Promise<void>;
};

export function useAgent({
  grid, rows, cols, start, goal, algo, speedMs,
}: UseAgentParams): UseAgentReturn {

  const [layers, setLayers] = useState<Layers>({
    visited: new Array(rows * cols).fill(false),
    future: new Array(rows * cols).fill(false),
    done: new Array(rows * cols).fill(false),
  });

  const [isPlaying, setIsPlaying] = useState(false);
  const [isBusy, setIsBusy] = useState(false);
  const [status, setStatus] = useState("Listo");
  const timerRef = useRef<number | null>(null);

  const agentRef = useRef<Pt>({ ...start });
  const planRef = useRef<Pt[]>([]);
  const planIdxRef = useRef<number>(0);

  useEffect(() => {
    agentRef.current = { ...start };
    planRef.current = [];
    planIdxRef.current = 0;
    setLayers({
      visited: new Array(rows * cols).fill(false),
      future: new Array(rows * cols).fill(false),
      done: new Array(rows * cols).fill(false),
    });
    setIsPlaying(false);
  }, [rows, cols, start.r, start.c, goal.r, goal.c]);

  function clearTimer() {
    if (timerRef.current !== null) { window.clearTimeout(timerRef.current); timerRef.current = null; }
  }

  function paintFuture(path: Pt[]) {
    const next = new Array(rows * cols).fill(false);
    for (let i = 1; i < path.length; i++) next[idx(path[i].r, path[i].c, cols)] = true;
    setLayers(l => ({ ...l, future: next }));
  }

  function markAgentCellAsDone(p: Pt) {
    setLayers(l => {
      const done = l.done.slice();
      done[idx(p.r, p.c, cols)] = true;
      return { ...l, done };
    });
  }

  // ===== Tick del agente =====
  useEffect(() => {
    if (!isPlaying) { clearTimer(); return; }
    clearTimer();

    const tick = async () => {
      const plan = planRef.current;
      const k = planIdxRef.current;

      if (!plan.length || k >= plan.length - 1) {
        setIsPlaying(false);
        setStatus("Destino alcanzado o plan vacío");
        return;
      }

      const next = plan[k + 1];
      agentRef.current = { ...next };
      markAgentCellAsDone(next);

      paintFuture(plan.slice(k + 1));

      if (algo === "dstar") { try { await dstarMove(next.r, next.c); } catch { } }

      planIdxRef.current = k + 1;
      timerRef.current = window.setTimeout(tick, Math.max(0, speedMs));
    };

    timerRef.current = window.setTimeout(tick, Math.max(0, speedMs));
    return clearTimer;
  }, [isPlaying, speedMs, algo, rows, cols]);

  // ===== Play =====
  async function play() {
    if (isBusy) return;
    setIsBusy(true);
    setIsPlaying(false);
    setStatus(algo === "dstar" ? "INIT D* Lite..." : "Calculando plan...");

    setLayers({
      visited: new Array(rows * cols).fill(false),
      future: new Array(rows * cols).fill(false),
      done: new Array(rows * cols).fill(false),
    });

    agentRef.current = { ...start };
    markAgentCellAsDone(start);

    const body = buildBody(rows, cols, start.r, start.c, goal.r, goal.c, grid);

    try {
      if (algo === "dstar") {
        const out = await dstarInit(body, cols);
        const visited = new Array(rows * cols).fill(false);
        out.visited.forEach(p => visited[idx(p.r, p.c, cols)] = true);
        setLayers(l => ({ ...l, visited }));

        planRef.current = out.path;
        planIdxRef.current = 0;
        paintFuture(out.path);

        setStatus(`D*Lite listo: ${out.path.length} nodos`);
      } else {
        const out = await runOneShot(algo, body, cols);
        const visited = new Array(rows * cols).fill(false);
        out.visited.forEach(p => visited[idx(p.r, p.c, cols)] = true);
        setLayers(l => ({ ...l, visited }));

        planRef.current = out.path;
        planIdxRef.current = 0;
        paintFuture(out.path);

        setStatus(`Plan listo: ${out.path.length} nodos`);
      }

      setIsPlaying(true);
    } catch (e: any) {
      setStatus(`Error: ${e?.message || "falló ejecución"}`);
    } finally {
      setIsBusy(false);
    }
  }

  function pause() { setIsPlaying(false); }

  function clearLayers() {
    setLayers({
      visited: new Array(rows * cols).fill(false),
      future: new Array(rows * cols).fill(false),
      done: new Array(rows * cols).fill(false),
    });
  }

  // ===== Replan por obstáculo (una celda) =====
  async function onObstacleChanged(r: number, c: number, blocked: boolean) {
    return onObstacleBatchChanged([{ r, c, blocked }]);
  }

  // ===== Replan por obstáculo (batch) =====
  async function onObstacleBatchChanged(changes: { r: number; c: number; blocked: boolean }[]) {
    if (!isPlaying || changes.length === 0) return;

    if (algo === "dstar") {
      // D* Lite: mover start interno al agente y mandar un solo batch UPDATE
      setIsBusy(true);
      setIsPlaying(false);
      setStatus("D*Lite UPDATE+PLAN...");

      try { await dstarMove(agentRef.current.r, agentRef.current.c); } catch { }

      const batch = changes.map(ch => `${ch.r} ${ch.c} ${ch.blocked ? 1e9 : 1}`).join("\n") + "\n";
      try {
        const out = await dstarUpdate(batch, cols);

        const visited = new Array(rows * cols).fill(false);
        out.visited.forEach(p => visited[idx(p.r, p.c, cols)] = true);
        setLayers(l => ({ ...l, visited, future: new Array(rows * cols).fill(false) }));

        planRef.current = out.path;
        planIdxRef.current = 0;
        paintFuture(out.path);

        setStatus(`Plan actualizado: ${out.path.length} nodos`);
        setIsPlaying(true);
      } catch (e: any) {
        setStatus(`Error UPDATE: ${e.message || ""}`);
      } finally { setIsBusy(false); }

      return;
    }

    // One-shot: recalcular SOLO agente→goal una vez
    setIsBusy(true);
    setIsPlaying(false);
    setStatus("Replanificando (agente→goal)...");
    const a = agentRef.current;
    const body = buildBody(rows, cols, a.r, a.c, goal.r, goal.c, grid);
    try {
      const out = await runOneShot(algo as Exclude<AlgoKey, "dstar">, body, cols);
      const visited = new Array(rows * cols).fill(false);
      out.visited.forEach(p => visited[idx(p.r, p.c, cols)] = true);
      setLayers(l => ({ ...l, visited }));

      planRef.current = out.path;
      planIdxRef.current = 0;
      paintFuture(out.path);

      setStatus(`Replan listo: ${out.path.length} nodos`);
      setIsPlaying(true);
    } catch (e: any) {
      setStatus(`Error replan: ${e?.message || ""}`);
    } finally {
      setIsBusy(false);
    }
  }

  return {
    layers,
    agent: agentRef.current,
    isPlaying,
    isBusy,
    status,
    play,
    pause,
    clearLayers,
    onObstacleChanged,
    onObstacleBatchChanged,
  };
}

