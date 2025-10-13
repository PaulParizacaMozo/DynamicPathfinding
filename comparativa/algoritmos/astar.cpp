#include "utils.hpp"
#include <queue>
#include <limits>
#include <cmath>

/*
  A*:
  - Grafo CSR (utils.hpp)
  - f(u) = g(u) + h(u), donde:
      g(u): costo acumulado desde s
      h(u): heurística estimada de u a t
  - Min-heap por f(u)
  - "Closed set" implícito con vector<bool>/char para no reexpandir un nodo ya fijado

  Notas:
    - Si g.has_coords=false, heuristic_grid(...) devuelve 0 => A* == Dijkstra
    - Con grid 4-dir usa Manhattan; con 8-dir, octile (definidos en utils.cpp)
    - Requiere pesos no negativos.
*/

bool astar_run(const CSR& g, int s, int t, std::vector<int>& parent){
    const float INF = std::numeric_limits<float>::infinity();
    parent.assign(g.N, -1);

    // Casos borde
    if(s < 0 || s >= g.N || t < 0 || t >= g.N) return false;
    if(s == t){ parent[t] = -1; return true; } // ruta trivial

    // gscore: costo exacto desde s
    // fscore: estimación total f = g + h
    std::vector<float> gscore(g.N, INF), fscore(g.N, INF);

    // Heurística (0 si no hay coords)
    auto h = [&](int u){ return heuristic_grid(g, u, t); };

    using P = std::pair<float,int>; // (fscore, nodo)
    std::priority_queue<P, std::vector<P>, std::greater<P>> open;

    std::vector<char> closed(g.N, 0); // 0: no cerrado, 1: cerrado

    gscore[s] = 0.0f;
    fscore[s] = h(s);
    open.push({fscore[s], s});

    while(!open.empty()){
        auto [fu, u] = open.top(); open.pop();

        // Si este nodo ya fue cerrado, ignora (lazy)
        if(closed[u]) continue;

        // Si la clave f extraída no coincide (por obsolescencia), no pasa nada.
        if(fu != fscore[u]) continue;

        // Meta alcanzada: como h >= 0 y consistente, esto es óptimo.
        if(u == t) break;

        closed[u] = 1;

        // Expandir
        for(long long e = g.row_ptr[u]; e < g.row_ptr[u+1]; ++e){
            int   v   = g.col_ind[e];
            float wuv = g.w[e];
            if(wuv < 0.0f) continue; // A* requiere pesos no negativos

            if(closed[v]) continue;

            float tentative = gscore[u] + wuv;
            if(tentative < gscore[v]){
                gscore[v] = tentative;
                parent[v] = u;
                fscore[v] = tentative + h(v);
                open.push({fscore[v], v});
            }
        }
    }

    // Return true si la distancia a t es finita
    return std::isfinite(gscore[t]);
}
