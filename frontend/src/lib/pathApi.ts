import { API_BASE } from "./state";
import type { Cell, Pt } from "./state";

// Construye el cuerpo de entrada para los binarios
export function buildBody(
  rows: number,
  cols: number,
  sr: number,
  sc: number,
  er: number,
  ec: number,
  cells: Cell[]
) {
  const header = `${rows} ${cols} ${sr} ${sc} ${er} ${ec}`;
  const lines = [header];
  for (let r = 0; r < rows; r++) {
    const row: string[] = [];
    for (let c = 0; c < cols; c++) row.push(String(cells[r * cols + c] ? 1 : 0));
    lines.push(row.join(" "));
  }
  return lines.join("\n");
}

// Parsea salida "Visited/Parents/Path"
export function parseOutput(
  text: string,
  cols: number
): { visited: Pt[]; path: Pt[]; parents: Map<number, number> } {
  const lines = text.trim().split(/\r?\n/);
  const visited: Pt[] = [];
  const path: Pt[] = [];
  const parents = new Map<number, number>();

  let mode: "none" | "visited" | "parents" | "path" = "none";
  for (const ln of lines) {
    if (ln.startsWith("Visited:")) { mode = "visited"; continue; }
    if (ln.startsWith("Parents:")) { mode = "parents"; continue; }
    if (ln.startsWith("Path:")) { mode = "path"; continue; }
    const parts = ln.trim().split(/\s+/);
    if (!parts[0]) continue;

    if (mode === "visited" && parts.length >= 2) {
      const r = parseInt(parts[0], 10), c = parseInt(parts[1], 10);
      if (!Number.isNaN(r) && !Number.isNaN(c)) visited.push({ r, c });
    } else if (mode === "parents" && parts.length >= 4) {
      const r = parseInt(parts[0], 10), c = parseInt(parts[1], 10);
      const pr = parseInt(parts[2], 10), pc = parseInt(parts[3], 10);
      if (![r, c, pr, pc].some(Number.isNaN)) {
        const id = r * cols + c;
        const pid = pr * cols + pc;
        parents.set(id, pid);
      }
    } else if (mode === "path" && parts.length >= 2) {
      const r = parseInt(parts[0], 10), c = parseInt(parts[1], 10);
      if (!Number.isNaN(r) && !Number.isNaN(c)) path.push({ r, c });
    }
  }
  return { visited, path, parents };
}

// === Fetchers ===

// Dijkstra / A* / BMSSP
export async function runOneShot(algo: "dijkstra" | "astar" | "bmssp", body: string, cols: number) {
  const resp = await fetch(`${API_BASE}/api/${algo}`, {
    method: "POST",
    headers: { "Content-Type": "text/plain" },
    body,
  });
  if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
  return parseOutput(await resp.text(), cols);
}

// D* Lite
export async function dstarInit(body: string, cols: number) {
  const resp = await fetch(`${API_BASE}/api/dstar/init`, {
    method: "POST",
    headers: { "Content-Type": "text/plain" },
    body,
  });
  if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
  return parseOutput(await resp.text(), cols);
}

export async function dstarMove(r: number, c: number) {
  await fetch(`${API_BASE}/api/dstar/move`, {
    method: "POST",
    headers: { "Content-Type": "text/plain" },
    body: `${r} ${c}`,
  });
}

export async function dstarUpdate(batch: string, cols: number) {
  const resp = await fetch(`${API_BASE}/api/dstar/update`, {
    method: "POST",
    headers: { "Content-Type": "text/plain" },
    body: batch,
  });
  if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
  return parseOutput(await resp.text(), cols);
}
