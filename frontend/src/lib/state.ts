// Tipos y utilidades compartidas

export type Cell = 0 | 1;
export type Pt = { r: number; c: number };
export type AlgoKey = "dijkstra" | "astar" | "dstar" | "bmssp";
export type EditMode = "toggleObstacle" | "moveStart" | "moveGoal";

export type Layers = {
  visited: boolean[];
  future: boolean[];
  done: boolean[];
};

export const CELL_SIZE = 20;

// idx línea → id en arreglo 1D
export const idx = (r: number, c: number, cols: number) => r * cols + c;

// Colores (solo referencia; el render usa valores directos)
export const COLORS = {
  bg: "#f5f5f5",
  grid: "#e0e0e0",
  obstacle: "#000000",
  visited: "#ffd54f",
  future: "#2196f3",
  done: "#0d47a1",
  agent: "#ff6f00",
  start: "#2e7d32",
  goal: "#c62828",
};

// Base de API
export const API_BASE = import.meta.env.VITE_API_BASE || "http://localhost:4000";
