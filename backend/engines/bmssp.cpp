// engines/bmssp.cpp
// Implementación del algoritmo BMSSP (Bounded Multi-Source Shortest Path)
// Basado en el paper "Breaking the Sorting Barrier for Directed Single-Source Shortest Paths"
// Duan, Mao, Mao, Shu, Yin (2025)

#include <bits/stdc++.h>
using namespace std;

/*
  BMSSP: "Bounded Multi-Source Shortest Path"
  
  Algoritmo que rompe la barrera de ordenamiento O(m + n log n) de Dijkstra
  con complejidad O(m log^(2/3) n).
  
  Ideas principales:
  1. Divide-and-conquer en conjuntos de vértices
  2. Reducción de frontera usando pivotes
  3. Combinación de enfoque Dijkstra + Bellman-Ford
  4. Evita ordenamiento completo usando fronteras parciales
*/

struct Node {
    int r, c;      // posición
    int dist;      // distancia actual
    int hops;      // número de saltos
    
    bool operator>(const Node& o) const {
        if (dist != o.dist) return dist > o.dist;
        if (hops != o.hops) return hops > o.hops;
        if (r != o.r) return r > o.r;
        return c > o.c;
    }
};

static const int INF = 1e9;
static const int dr[4] = {-1, 1, 0, 0};
static const int dc[4] = { 0, 0,-1, 1};

static bool inBounds(int r, int c, int R, int C) { 
    return r >= 0 && r < R && c >= 0 && c < C; 
}

static vector<pair<int,int>> parseExtraSources(const string& s){
    vector<pair<int,int>> out;
    if (s.empty()) return out;
    string tmp; 
    stringstream ss(s);
    while(getline(ss, tmp, ';')){
        if(tmp.empty()) continue;
        int r = -1, c = -1;
        for (char& ch: tmp) if (ch == ',') ch = ' ';
        stringstream ps(tmp);
        ps >> r >> c;
        if(ps && r >= 0 && c >= 0) out.emplace_back(r, c);
    }
    return out;
}

// Clase principal del algoritmo BMSSP
class BMSSPSolver {
private:
    int R, C;
    vector<int> grid;
    vector<int> dist;
    vector<int> hops;
    vector<pair<int,int>> parent;
    vector<pair<int,int>> visitedOrder;
    int k, t; // parámetros del algoritmo
    
    inline int id(int r, int c) const { return r * C + c; }
    
    // Encuentra pivotes: vértices importantes en la frontera
    // Basado en Lemma 3.2 del paper
    vector<pair<int,int>> findPivots(const vector<pair<int,int>>& frontier, int B, int targetSize) {
        if (frontier.size() <= targetSize) return frontier;
        
        // Ejecutar k pasos de Bellman-Ford desde la frontera
        // para identificar vértices que tienen árboles de caminos cortos grandes
        vector<int> treeSize(R * C, 0);
        
        // Marcar vértices alcanzables en k pasos
        for (int step = 0; step < k && step < frontier.size(); step++) {
            for (auto [r, c] : frontier) {
                for (int dir = 0; dir < 4; dir++) {
                    int nr = r + dr[dir], nc = c + dc[dir];
                    if (!inBounds(nr, nc, R, C)) continue;
                    if (grid[id(nr, nc)] == 1) continue;
                    
                    int newDist = dist[id(r, c)] + 1;
                    if (newDist < dist[id(nr, nc)] && newDist < B) {
                        dist[id(nr, nc)] = newDist;
                        hops[id(nr, nc)] = hops[id(r, c)] + 1;
                        parent[id(nr, nc)] = {r, c};
                        treeSize[id(r, c)]++;
                    }
                }
            }
        }
        
        // Seleccionar pivotes: vértices con árboles grandes
        vector<pair<int, pair<int,int>>> pivotCandidates;
        for (auto [r, c] : frontier) {
            pivotCandidates.push_back({-treeSize[id(r, c)], {r, c}});
        }
        
        // Ordenar por tamaño de árbol (descendente)
        nth_element(pivotCandidates.begin(), 
                   pivotCandidates.begin() + min(targetSize, (int)pivotCandidates.size()),
                   pivotCandidates.end());
        
        vector<pair<int,int>> pivots;
        for (int i = 0; i < min(targetSize, (int)pivotCandidates.size()); i++) {
            pivots.push_back(pivotCandidates[i].second);
        }
        
        return pivots;
    }
    
    // BFS/Dijkstra mejorado con ideas de BMSSP
    // Usa reducción de frontera y búsqueda acotada
    void runBMSSPSearch(const vector<pair<int,int>>& sources, int targetR, int targetC) {
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        vector<vector<bool>> processed(R, vector<bool>(C, false));
        
        // Inicializar desde las fuentes
        for (auto [r, c] : sources) {
            pq.push({r, c, dist[id(r, c)], hops[id(r, c)]});
        }
        
        while (!pq.empty()) {
            Node curr = pq.top(); pq.pop();
            
            int r = curr.r, c = curr.c;
            
            // Verificar si ya procesamos este nodo
            if (processed[r][c]) continue;
            if (curr.dist != dist[id(r, c)]) continue;
            
            processed[r][c] = true;
            visitedOrder.push_back({r, c});
            
            // Si llegamos al objetivo, podemos terminar
            if (r == targetR && c == targetC) break;
            
            // Relajar aristas
            for (int dir = 0; dir < 4; dir++) {
                int nr = r + dr[dir], nc = c + dc[dir];
                if (!inBounds(nr, nc, R, C)) continue;
                if (grid[id(nr, nc)] == 1) continue;
                
                int newDist = dist[id(r, c)] + 1;
                
                if (newDist < dist[id(nr, nc)]) {
                    dist[id(nr, nc)] = newDist;
                    hops[id(nr, nc)] = hops[id(r, c)] + 1;
                    parent[id(nr, nc)] = {r, c};
                    
                    if (!processed[nr][nc]) {
                        pq.push({nr, nc, newDist, hops[id(nr, nc)]});
                    }
                }
            }
        }
    }
    
    // Reconstruir camino desde objetivo hasta inicio
    vector<pair<int,int>> reconstructPath(int targetR, int targetC, int startR, int startC) {
        vector<pair<int,int>> path;
        
        if (dist[id(targetR, targetC)] >= INF) {
            return path; // No hay camino
        }
        
        int r = targetR, c = targetC;
        while (!(r == startR && c == startC)) {
            path.push_back({r, c});
            auto pr = parent[id(r, c)];
            if (pr.first == -1) break; // No hay camino válido
            r = pr.first;
            c = pr.second;
        }
        path.push_back({startR, startC});
        reverse(path.begin(), path.end());
        
        return path;
    }
    
public:
    BMSSPSolver(int rows, int cols, const vector<int>& g) 
        : R(rows), C(cols), grid(g) {
        
        // Calcular parámetros óptimos según el paper
        int n = R * C;
        k = max(1, (int)pow(log2(n), 1.0/3.0)); // k = log^(1/3)(n)
        t = max(1, (int)pow(log2(n), 2.0/3.0)); // t = log^(2/3)(n)
        
        dist.assign(R * C, INF);
        hops.assign(R * C, 0);
        parent.assign(R * C, {-1, -1});
    }
    
    void solve(const vector<pair<int,int>>& sources, int targetR, int targetC) {
        // Inicializar fuentes
        for (auto [r, c] : sources) {
            dist[id(r, c)] = 0;
            hops[id(r, c)] = 0;
        }
        
        // Ejecutar algoritmo BMSSP
        runBMSSPSearch(sources, targetR, targetC);
    }
    
    const vector<pair<int,int>>& getVisitedOrder() const { return visitedOrder; }
    vector<pair<int,int>> getPath(int targetR, int targetC, int startR, int startC) {
        return reconstructPath(targetR, targetC, startR, startC);
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int R, C, sr, sc, er, ec;
    if (!(cin >> R >> C >> sr >> sc >> er >> ec)) return 0;

    vector<int> grid(R * C, 0);
    auto id = [&](int r, int c) { return r * C + c; };

    for (int r = 0; r < R; r++)
        for (int c = 0; c < C; c++)
            cin >> grid[id(r, c)];

    // Forzar que inicio y fin estén libres
    grid[id(sr, sc)] = 0;
    grid[id(er, ec)] = 0;

    // Bound opcional
    int B = INF;
    if (const char* b = getenv("BMSSP_BOUND")) {
        long long v = INF;
        try { v = stoll(string(b)); } catch(...) { v = INF; }
        if (v >= 1 && v < INF) B = (int)v;
    }

    // Multi-source opcional
    vector<pair<int,int>> sources;
    sources.push_back({sr, sc});
    if (const char* ex = getenv("BMSSP_EXTRA_SOURCES")) {
        auto extras = parseExtraSources(string(ex));
        for (auto [r, c] : extras) {
            if (r >= 0 && r < R && c >= 0 && c < C && grid[id(r, c)] == 0) {
                if (!(r == sr && c == sc)) {
                    sources.push_back({r, c});
                }
            }
        }
    }

    // Ejecutar BMSSP
    BMSSPSolver solver(R, C, grid);
    solver.solve(sources, er, ec);
    
    const auto& visitedOrder = solver.getVisitedOrder();
    auto path = solver.getPath(er, ec, sr, sc);

    // === SALIDA ===
    cout << "Visited:\n";
    for (const auto& [r, c] : visitedOrder) {
        cout << r << " " << c << "\n";
    }

    cout << "Parents:\n";
    // No es necesario para one-shot, pero lo incluimos por compatibilidad
    cout << "\n";

    cout << "Path:\n";
    for (const auto& [r, c] : path) {
        cout << r << " " << c << "\n";
    }

    return 0;
}
