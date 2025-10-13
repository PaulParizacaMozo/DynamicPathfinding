#include "utils.hpp"
#include <queue>
#include <limits>
#include <cmath>

/*
  Dijkstra (pesos no negativos) con:
  - Grafo en formato CSR (utils.hpp)
  - Cola de prioridad (min-heap) sobre (distancia, nodo)
  - "Lazy deletion": ignoramos entradas obsoletas en el heap
  - Corte temprano: al extraer 't' del heap, ya tenemos su mejor distancia

  Entrada:
    g : CSR (N, M, row_ptr, col_ind, w)
    s : origen
    t : destino
  Salida:
    parent[v] : predecesor inmediato en el camino desde s a v (o -1)
    return    : true si existe ruta s->t, false si no

  Complejidad:
    - Tiempo: O( (N + M) log N ) con binary heap
    - Memoria: O(N) para dist y parent

  Notas de implementación (escala):
    - dist usa float (mismo tipo que pesos). Si tus pesos pueden acumular
      números muy grandes, cambia a double.
    - "Lazy deletion" evita coste extra de decrease-key. Comprobamos si la
      distancia del tope coincide con dist[u]; si no, descartamos.
    - Corte temprano ahorra trabajo cuando solo importa el s->t.
*/

bool dijkstra_run(const CSR& g, int s, int t, std::vector<int>& parent){
    const float INF = std::numeric_limits<float>::infinity();
    parent.assign(g.N, -1);

    // Casos borde
    if(s < 0 || s >= g.N || t < 0 || t >= g.N) return false;
    if(s == t){ parent[t] = -1; return true; } // ruta trivial (longitud 1)

    std::vector<float> dist(g.N, INF);

    // Min-heap por distancia: (dist, nodo)
    using P = std::pair<float,int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;

    // Inicialización
    dist[s] = 0.0f;
    pq.push({0.0f, s});

    while(!pq.empty()){
        auto [du, u] = pq.top(); pq.pop();

        // Entrada obsoleta (lazy deletion)
        if(du != dist[u]) continue;

        // Corte temprano: al extraer u con su mejor distancia,
        // si u == t, ya conocemos la mejor ruta a t.
        if(u == t) break;

        // Relajación de aristas salientes u -> v
        for(long long e = g.row_ptr[u]; e < g.row_ptr[u+1]; ++e){
            int   v  = g.col_ind[e];
            float wuv = g.w[e];
            // Dijkstra requiere pesos no-negativos
            if(wuv < 0.0f) continue; // o lanzar excepción si quieres ser estricto

            float nd = du + wuv;
            if(nd < dist[v]){
                dist[v]   = nd;
                parent[v] = u;
                pq.push({nd, v});
            }
        }
    }

    // return true si la distancia finita a t
    return std::isfinite(dist[t]);
}
