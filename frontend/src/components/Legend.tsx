export default function Legend() {
  return (
    <div style={{ marginTop: 8, fontSize: 12, color: "#666" }}>
      <h4>Leyenda de Colores</h4>
      <div><span style={{ background: "#000", color: "#fff", padding: "0 6px" }}>■</span> Obstáculo</div>
      <div><span style={{ background: "#ffd54f", padding: "0 6px" }}>■</span> Área de Búsqueda</div>
      <div><span style={{ background: "#2196f3", color: "#fff", padding: "0 6px" }}>■</span> Camino Futuro</div>
      <div><span style={{ background: "#0d47a1", color: "#fff", padding: "0 6px" }}>■</span> Camino Recorrido</div>
      <div><span style={{ background: "#ff6f00", color: "#fff", padding: "0 6px" }}>■</span> Agente</div>
      <div><span style={{ background: "#2e7d32", color: "#fff", padding: "0 6px" }}>■</span> Inicio</div>
      <div><span style={{ background: "#c62828", color: "#fff", padding: "0 6px" }}>■</span> Fin</div>
    </div>
  );
}
