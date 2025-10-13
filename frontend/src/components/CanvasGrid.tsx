import { useEffect, useRef, useState } from "react";
import { CELL_SIZE, COLORS } from "../lib/state";
import type { Cell, Layers, Pt } from "../lib/state";
import { idx } from "../lib/state";

type Props = {
  rows: number;
  cols: number;
  grid: Cell[];
  layers: Layers;
  start: Pt;
  goal: Pt;
  agent?: Pt;
  onClickCell: (r: number, c: number) => void;
  onRightClickCell: (r: number, c: number) => void;

  // Drag paint con información del botón del puntero
  onStartPaint?: (r: number, c: number, button: number) => void;
  onPaintOver?: (r: number, c: number, button: number) => void;
  onEndPaint?: (button: number) => void;
};

export default function CanvasGrid({
  rows, cols, grid, layers, start, goal, agent,
  onClickCell, onRightClickCell,
  onStartPaint, onPaintOver, onEndPaint,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const [painting, setPainting] = useState(false);
  const lastButtonRef = useRef<number>(0);
  const lastCellRef = useRef<{ r: number; c: number } | null>(null);

  // Dibujo
  useEffect(() => {
    const cvs = canvasRef.current;
    if (!cvs) return;
    const ctx = cvs.getContext("2d")!;
    cvs.width = cols * CELL_SIZE;
    cvs.height = rows * CELL_SIZE;

    ctx.clearRect(0, 0, cvs.width, cvs.height);
    ctx.fillStyle = COLORS.bg;
    ctx.fillRect(0, 0, cvs.width, cvs.height);

    for (let r = 0; r < rows; r++) {
      for (let c = 0; c < cols; c++) {
        const x = c * CELL_SIZE, y = r * CELL_SIZE;
        const id = idx(r, c, cols);
        if (grid[id] === 1) {
          ctx.fillStyle = COLORS.obstacle;
          ctx.fillRect(x, y, CELL_SIZE, CELL_SIZE);
        } else {
          if (layers.visited[id]) { ctx.fillStyle = COLORS.visited; ctx.fillRect(x, y, CELL_SIZE, CELL_SIZE); }
          if (layers.done[id]) { ctx.fillStyle = COLORS.done; ctx.fillRect(x, y, CELL_SIZE, CELL_SIZE); }
          if (layers.future[id]) { ctx.fillStyle = COLORS.future; ctx.fillRect(x, y, CELL_SIZE, CELL_SIZE); }
        }
        ctx.strokeStyle = COLORS.grid;
        ctx.strokeRect(x, y, CELL_SIZE, CELL_SIZE);
      }
    }

    const drawCell = (p: Pt, color: string) => {
      ctx.fillStyle = color;
      ctx.fillRect(p.c * CELL_SIZE, p.r * CELL_SIZE, CELL_SIZE, CELL_SIZE);
    };

    drawCell(start, COLORS.start);
    drawCell(goal, COLORS.goal);

    if (agent && !(agent.r === start.r && agent.c === start.c) && !(agent.r === goal.r && agent.c === goal.c)) {
      drawCell(agent, COLORS.agent);
    }
  }, [rows, cols, grid, layers, start.r, start.c, goal.r, goal.c, agent?.r, agent?.c]);

  // Acepta Mouse o Pointer event
  const getCellFromEvent = (
    ev: React.MouseEvent<HTMLCanvasElement> | React.PointerEvent<HTMLCanvasElement>,
    rows: number,
    cols: number
  ) => {
    const rect = ev.currentTarget.getBoundingClientRect();
    const x = Math.max(0, Math.min(rect.width - 1, ev.clientX - rect.left));
    const y = Math.max(0, Math.min(rect.height - 1, ev.clientY - rect.top));
    const r = Math.floor(y / CELL_SIZE);
    const c = Math.floor(x / CELL_SIZE);
    return { r: Math.max(0, Math.min(rows - 1, r)), c: Math.max(0, Math.min(cols - 1, c)) };
  };

  return (
    <canvas
      ref={canvasRef}
      style={{ border: "1px solid #ccc", cursor: "crosshair", touchAction: "none" }}
      onClick={(e) => {
        const { r, c } = getCellFromEvent(e, rows, cols);
        onClickCell(r, c);
      }}
      onContextMenu={(e) => {
        if (!painting) {
          e.preventDefault();
          const { r, c } = getCellFromEvent(e, rows, cols);
          onRightClickCell(r, c);
        } else {
          e.preventDefault();
        }
      }}
      onPointerDown={(e) => {
        (e.currentTarget as HTMLCanvasElement).setPointerCapture(e.pointerId);
        if (e.button === 2) e.preventDefault(); // evitar menú al arrastrar con derecho

        setPainting(true);
        lastButtonRef.current = e.button;

        const { r, c } = getCellFromEvent(e, rows, cols);
        lastCellRef.current = { r, c }; // guarda la primera celda
        onStartPaint?.(r, c, e.button);
      }}
      onPointerMove={(e) => {
        if (!painting) return;
        const { r, c } = getCellFromEvent(e, rows, cols);
        lastCellRef.current = { r, c }; // actualiza última celda válida
        onPaintOver?.(r, c, lastButtonRef.current);
      }}
      onPointerUp={(e) => {
        if (!painting) return;

        // Asegura pintar la última celda válida aunque sueltes fuera o se “salte” el move
        const last = lastCellRef.current;
        if (last) onPaintOver?.(last.r, last.c, lastButtonRef.current);

        setPainting(false);
        (e.currentTarget as HTMLCanvasElement).releasePointerCapture(e.pointerId);
        onEndPaint?.(lastButtonRef.current);
        lastCellRef.current = null;
      }}
      onPointerLeave={() => {
        if (!painting) return;
        // Si sales del canvas pintando, aplica también la última celda antes de cerrar
        const last = lastCellRef.current;
        if (last) onPaintOver?.(last.r, last.c, lastButtonRef.current);

        setPainting(false);
        onEndPaint?.(lastButtonRef.current);
        lastCellRef.current = null;
      }}
    />
  );
}

