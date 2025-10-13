#include "utils.hpp"
#include <queue>
#include <limits>
#include <tuple>
#include <unordered_map>
#include <cmath>

/*
  D* Lite (dinámico, dirigidos, pesos no negativos)

  Ideas clave:
  - Mantiene funciones g(.) y rhs(.) y un open-set con prioridad por claves:
      key(u) = [min(g(u), rhs(u)) + h(start,u) + km,  min(g(u), rhs(u))]
  - "goal" es la fuente del cálculo (rhs(goal)=0). La política extrae ruta
    desde start, siguiendo sucesores v que minimicen c(u,v)+g(v).
  - Para actualizaciones de aristas (u->v con nuevo peso), se recalcula rhs(u)
    y se llama updateVertex(u). Luego ComputeShortestPath() repara óptimos.
  - Este archivo implementa la versión dinámica internamente, y expone:
      - dstar_lite_run_static(...)  -> sin cambios, 1 sola planificación
      - dstar_lite_run_dynamic(...) -> con lista de cambios (opcional)

  Notas:
  - Requiere grafo inverso (predecesores) para actualizar rápido.
  - Heurística: reutiliza heuristic_grid(g, u, start_actual). Si no hay coords,
    h=0 (admisible), equivalente a LPA.
*/

namespace {
struct Key {
    float k1, k2;
    int   u;    // nodo
    // comparador para priority_queue (min-heap)
    bool operator>(const Key& o) const {
        if(k1 != o.k1) return k1 > o.k1;
        if(k2 != o.k2) return k2 > o.k2;
        return u  > o.u;
    }
};

struct ReverseCSR {
    int N=0;
    std::vector<long long> row_ptr; // N+1
    std::vector<int>       col_ind; // lista de predecesores: para cada x, lista de p con p->x
    std::vector<float>     w;       // pesos p->x
};

ReverseCSR build_reverse(const CSR& g){
    ReverseCSR r; r.N = g.N;
    r.row_ptr.assign(r.N+1, 0);

    // contar indegrees
    for(int u=0; u<g.N; ++u){
        for(long long e=g.row_ptr[u]; e<g.row_ptr[u+1]; ++e){
            int v = g.col_ind[e];
            r.row_ptr[v+1]++; // un pred más para v
        }
    }
    // prefijos
    for(int i=0;i<r.N;i++) r.row_ptr[i+1]+=r.row_ptr[i];
    long long M = r.row_ptr.back();
    r.col_ind.assign(M, 0);
    r.w.assign(M, 0.0f);

    // relleno: necesitamos offsets temporales
    std::vector<long long> off = r.row_ptr;
    for(int u=0; u<g.N; ++u){
        for(long long e=g.row_ptr[u]; e<g.row_ptr[u+1]; ++e){
            int v = g.col_ind[e];
            float ww = g.w[e];
            long long pos = off[v]++;
            r.col_ind[pos] = u; // u es pred de v
            r.w[pos]       = ww;
        }
    }
    return r;
}

struct DStarLite {
    const CSR& G;
    ReverseCSR R;
    int s, goal;    // start actual y 'goal' (destino)
    float km = 0.0f;

    const float INF = std::numeric_limits<float>::infinity();

    std::vector<float> g, rhs;
    std::vector<char>  in_open;          // marca rápida
    std::vector<std::pair<float,float>> key_of; // clave actual (para invalidación)

    // open-set con lazy deletion
    std::priority_queue<Key, std::vector<Key>, std::greater<Key>> open;

    DStarLite(const CSR& g, int start, int target)
      : G(g), R(build_reverse(g)), s(start), goal(target),
        g(g.N, INF), rhs(g.N, INF), in_open(g.N, 0), key_of(g.N, {INF, INF})
    {
        // Inicialización: rhs(goal) = 0; inserta goal en OPEN
        rhs[goal] = 0.0f;
        insert(goal);
    }

    // Heurística entre dos nodos (usada con start)
    inline float h(int a, int b) const {
        // reusamos heuristic_grid si hay coords; caso contrario 0
        // Nota: utils::heuristic_grid(g,u,t) requiere referencia no-const
        // así que hacemos un const_cast seguro (no modifica 'G')
        return heuristic_grid(const_cast<CSR&>(G), a, b);
    }

    // Calcula clave de un nodo u
    inline std::pair<float,float> calcKey(int u) const {
        float mu = std::min(g[u], rhs[u]);
        return { mu + h(u, s) + km,  mu };
    }

    void insert(int u){
        auto K = calcKey(u);
        key_of[u] = K;
        open.push(Key{K.first, K.second, u});
        in_open[u] = 1;
    }
    void remove_from_open(int u){
        // Lazy: no extraemos explícitamente; solo invalidamos marca.
        in_open[u] = 0;
    }

    // Coste c(u,v) (si no existe, INF). Aquí el grafo es CSR saliente:
    // recorrer aristas de u y buscar v.
    float cost_uv(int u, int v) const {
        for(long long e=G.row_ptr[u]; e<G.row_ptr[u+1]; ++e)
            if(G.col_ind[e]==v) return G.w[e];
        return INF;
    }

    // rhs(u) = min_{v in Succ(u)} c(u,v) + g(v)
    float min_rhs_succ(int u){
        float best = INF;
        for(long long e=G.row_ptr[u]; e<G.row_ptr[u+1]; ++e){
            int v = G.col_ind[e];
            float cand = G.w[e] + g[v];
            if(cand < best) best = cand;
        }
        return best;
    }

    void updateVertex(int u){
        if(u != goal){
            rhs[u] = min_rhs_succ(u);
        }
        // Si u ya está en open, invalídalo (lazy)
        if(in_open[u]) remove_from_open(u);
        if(g[u] != rhs[u]){
            insert(u);
        }
    }

    // Devuelve true si kA < kB (lexicográfico)
    static inline bool lessKey(const std::pair<float,float>& A, const std::pair<float,float>& B){
        if(A.first != B.first) return A.first < B.first;
        return A.second < B.second;
    }

    void computeShortestPath(){
        while(true){
            if(open.empty()) break;

            // Limpieza de entradas obsoletas
            Key top = open.top();
            auto Ktop_pair = std::make_pair(top.k1, top.k2);
            if(!in_open[top.u] || Ktop_pair != key_of[top.u]){
                open.pop();
                continue;
            }

            auto Kstart = calcKey(s);
            auto Ktop   = Ktop_pair;

            // Condición de parada:
            // while ( topKey < key(s) || rhs(s) != g(s) )
            if( !lessKey(Ktop, Kstart) && !(rhs[s] != g[s]) ){
                break;
            }

            int u = top.u;
            open.pop();
            in_open[u] = 0;

            auto Kold = Ktop;

            auto Knew = calcKey(u);
            if( lessKey(Kold, Knew) ){
                // clave cambió, reinsertar con clave actual
                insert(u);
                continue;
            }

            if( g[u] > rhs[u] ){
                // Mejora: fijamos g[u] y actualizamos predecesores
                g[u] = rhs[u];
                // para todo p en Pred(u): updateVertex(p)
                for(long long e=R.row_ptr[u]; e<R.row_ptr[u+1]; ++e){
                    int p = R.col_ind[e];
                    updateVertex(p);
                }
            } else {
                // Empeora: g[u] = INF; actualizar u y todos sus predecesores
                float gold = g[u];
                g[u] = INF;
                updateVertex(u);
                for(long long e=R.row_ptr[u]; e<R.row_ptr[u+1]; ++e){
                    int p = R.col_ind[e];
                    // Solo los que dependían de u podrían mejorar/empeorar
                    if (std::fabs(rhs[p] - (cost_uv(p,u) + gold)) < 1e-6f){
                        updateVertex(p);
                    } else {
                        // Aun si no era el argmin exacto, es seguro actualizar
                        // de todas formas; el min_rhs_succ(p) lo arregla.
                        updateVertex(p);
                    }
                }
            }
        }
    }

    // Aplica un cambio de peso en arista u->v (nuevo valor nw)
    // y repara el grafo incrementalmente.
    void applyEdgeUpdate(int u, int v, float nw){
        // 1) Actualiza coste en CSR (u->v). (Búsqueda lineal en out-aristas de u.)
        bool found=false;
        for(long long e=G.row_ptr[u]; e<G.row_ptr[u+1]; ++e){
            if(G.col_ind[e]==v){
                const_cast<CSR&>(G).w[e] = nw; // mutación controlada
                found=true; break;
            }
        }
        if(!found){
            // Si no existía, ignoramos (o podrías insertarla; omitimos por simplicidad)
            return;
        }
        // 2) Recalcular rhs(u) porque depende de Succ(u)
        updateVertex(u);
        // 3) Repara óptimos
        computeShortestPath();
    }

    // Extrae una ruta greedy desde s hasta goal usando g(.) ya calculado.
    // Llena 'parent[v] = u' (predecesor) para reconstruir de goal hacia s.
    bool extract_path(std::vector<int>& parent){
        parent.assign(G.N, -1);
        if(!std::isfinite(g[s])) return false; // no hay ruta

        int u = s;
        int guard=0;
        while(u != goal){
            float best = INF; int best_v = -1;
            for(long long e=G.row_ptr[u]; e<G.row_ptr[u+1]; ++e){
                int v = G.col_ind[e];
                float cand = G.w[e] + g[v];
                if(cand < best){
                    best = cand; best_v = v;
                }
            }
            if(best_v == -1 || !std::isfinite(best)) return false;
            parent[best_v] = u;  // para backtracking desde goal
            u = best_v;
            if(++guard > G.N) return false; // guardia
        }
        return true;
    }
};
} // namespace

// -----------------------------
// API ESTÁTICA (para tu header)
// -----------------------------
bool dstar_lite_run_static(const CSR& g, int s, int t, std::vector<int>& parent){
    // Un solo plan estático: inicializa y resuelve
    DStarLite dsl(g, s, t);
    dsl.computeShortestPath();
    return dsl.extract_path(parent);
}

// --------------------------------------------------------------
// API DINÁMICA OPCIONAL (agrega esta firma en utils.hpp si quieres)
// --------------------------------------------------------------
// Ejecuta D* Lite y aplica una lista de cambios de peso en aristas.
// 'updates' es una lista de tuplas (u, v, new_w).
bool dstar_lite_run_dynamic(const CSR& g, int s, int t,
                            const std::vector<std::tuple<int,int,float>>& updates,
                            std::vector<int>& parent){
    DStarLite dsl(g, s, t);
    // Plan inicial
    dsl.computeShortestPath();
    // Aplica cambios (p.ej., bloqueos: new_w muy grande; o ajustes locales)
    for(const auto& up: updates){
        int u,v; float nw;
        std::tie(u,v,nw) = up;
        if(u<0||u>=g.N||v<0||v>=g.N) continue;
        if(nw < 0.0f) continue; // D* Lite requiere no-negativos
        dsl.applyEdgeUpdate(u, v, nw);
    }
    return dsl.extract_path(parent);
}
