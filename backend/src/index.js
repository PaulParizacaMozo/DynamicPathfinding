import express from "express";
import cors from "cors";
import bodyParser from "body-parser";
import { exec, spawn } from "child_process";
import * as process from "process";

const app = express();
app.use(cors());
app.use(bodyParser.text({ type: "*/*" }));

const PORT = process.env.PORT || 4000;
const isWindows = process.platform === "win32";
const exeExtension = isWindows ? ".exe" : "";
const binBase = isWindows ? "engines/bin" : "./engines/bin";

const getExecutablePath = (baseName) => `${binBase}/${baseName}${exeExtension}`;

const runExec = (exePath, inputData, res, algoName) => {
  const cmd = isWindows ? `"${exePath}"` : exePath;
  const child = exec(cmd, (error, stdout) => {
    if (error) {
      console.error(`Error executing ${algoName}:`, error);
      return res.status(500).send(`Failed to execute ${algoName} algorithm`);
    }
    res.send(stdout);
  });
  child.stdin.write(inputData + "\n");
  child.stdin.end();
};

// === Dijkstra / A* / BMSSP (como ya tenías) ===
app.post("/api/dijkstra", (req, res) => runExec(getExecutablePath("dijkstra"), req.body.trim(), res, "Dijkstra"));
app.post("/api/astar", (req, res) => runExec(getExecutablePath("astar"), req.body.trim(), res, "A*"));
app.post("/api/bmssp", (req, res) => runExec(getExecutablePath("bmssp"), req.body.trim(), res, "BMSSP"));

// === D* Lite persistente ===
let dstarChild = null;
let dstarBuffer = "";
let dstarQueue = [];

/** Inicia el proceso persistente si no existe */
function ensureDStar() {
  if (dstarChild && !dstarChild.killed) return;

  const exePath = getExecutablePath("d_star_lite");
  dstarChild = spawn(exePath, [], { stdio: "pipe" });

  dstarChild.stdout.on("data", (chunk) => {
    dstarBuffer += chunk.toString();
    // Respuestas terminan con "END\n"
    let idx;
    while ((idx = dstarBuffer.indexOf("\nEND\n")) !== -1) {
      const packet = dstarBuffer.slice(0, idx); // sin END
      dstarBuffer = dstarBuffer.slice(idx + 5);
      const resolver = dstarQueue.shift();
      if (resolver) resolver(packet);
    }
  });

  dstarChild.stderr.on("data", (chunk) => {
    console.error("[D*Lite STDERR]", chunk.toString());
  });

  dstarChild.on("exit", (code) => {
    console.warn("[D*Lite] exited", code);
    dstarChild = null;
  });
}

/** Envía comandos y espera una respuesta (terminada en END) */
function sendDStar(cmdText) {
  ensureDStar();
  return new Promise((resolve, reject) => {
    dstarQueue.push(resolve);
    dstarChild.stdin.write(cmdText);
    // opcional: timeout
    setTimeout(() => {
      if (dstarQueue.includes(resolve)) {
        reject(new Error("D*Lite timeout"));
      }
    }, 20000);
  });
}

/** Utilidades para traducir tu body a INIT + grid */
function parseGridBody(body) {
  // body = mismo formato que ya usas: header + filas de grid
  // header: rows cols sr sc er ec
  const lines = body.trim().split(/\r?\n/);
  const header = lines[0].trim();
  const gridLines = lines.slice(1).filter(Boolean);
  return { header, gridLines };
}

/** INIT + PLAN (devolvemos el primer plan ya listo) */
app.post("/api/dstar/init", async (req, res) => {
  try {
    const { header, gridLines } = parseGridBody(req.body);
    let cmd = `INIT ${header.split(/\s+/).slice(0).join(" ")}\n`;
    for (const ln of gridLines) cmd += ln + "\n";
    const ok = await sendDStar(cmd);
    if (!ok.startsWith("OK")) console.warn("[D*Lite INIT] resp:", ok);

    const plan = await sendDStar("PLAN\n");
    res.type("text/plain").send(plan);
  } catch (e) {
    console.error(e);
    res.status(500).send("Failed to init D* Lite");
  }
});

/** UPDATE (batch) + PLAN
 * Body esperado (texto):
 *  r c val
 *  r c val
 *  ...
 * Donde val = 1000000000 para obstáculo, 1 para libre
 */
app.post("/api/dstar/update", async (req, res) => {
  try {
    const lines = req.body.trim().split(/\r?\n/).filter(Boolean);
    let cmd = "";
    for (const ln of lines) {
      const [r, c, val] = ln.trim().split(/\s+/);
      cmd += `UPDATE ${r} ${c} ${val}\n`;
    }
    if (cmd === "") cmd = "UPDATE 0 0 1\n"; // no-op para no romper protocolo
    const ok = await sendDStar(cmd);
    // Podrían haber varios OK/END concatenados; no hace falta inspeccionarlos
    const plan = await sendDStar("PLAN\n");
    res.type("text/plain").send(plan);
  } catch (e) {
    console.error(e);
    res.status(500).send("Failed to update D* Lite");
  }
});

/** MOVE + PLAN
 * Body esperado: "r c" (texto)
 */
app.post("/api/dstar/move", async (req, res) => {
  try {
    const [r, c] = req.body.trim().split(/\s+/);
    const ok = await sendDStar(`MOVE ${r} ${c}\n`);
    const plan = await sendDStar("PLAN\n");
    res.type("text/plain").send(plan);
  } catch (e) {
    console.error(e);
    res.status(500).send("Failed to move D* Lite");
  }
});

app.listen(PORT, () => {
  console.log(`Backend running at http://localhost:${PORT}`);
});
