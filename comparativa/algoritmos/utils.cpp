#include "utils.hpp"
#include <fstream>
#include <random>
#include <cmath>
#include <stdexcept>

namespace {
inline int id_from_rc(int r, int c, int cols){ return r*cols + c; }
}

// -------------------- E/S binaria --------------------
void save_csr_bin(const CSR& g, const std::string& path){
    std::ofstream f(path, std::ios::binary);
    if(!f) throw std::runtime_error("No se puede abrir para escribir: " + path);

    f.write((char*)&g.N, sizeof(g.N));
    f.write((char*)&g.M, sizeof(g.M));
    f.write((char*)g.row_ptr.data(), sizeof(long long)*(g.N+1));
    f.write((char*)g.col_ind.data(), sizeof(int)*g.M);
    f.write((char*)g.w.data(), sizeof(float)*g.M);

    // Metadatos opcionales
    f.write((char*)&g.rows, sizeof(g.rows));
    f.write((char*)&g.cols, sizeof(g.cols));
    f.write((char*)&g.diag8, sizeof(g.diag8));
    f.write((char*)&g.has_coords, sizeof(g.has_coords));
    if(g.has_coords){
        f.write((char*)g.x.data(), sizeof(float)*g.N);
        f.write((char*)g.y.data(), sizeof(float)*g.N);
    }
}

CSR load_csr_bin(const std::string& path){
    std::ifstream f(path, std::ios::binary);
    if(!f) throw std::runtime_error("No se puede abrir para leer: " + path);

    CSR g;
    f.read((char*)&g.N, sizeof(g.N));
    f.read((char*)&g.M, sizeof(g.M));
    g.row_ptr.resize(g.N+1);
    g.col_ind.resize(g.M);
    g.w.resize(g.M);
    f.read((char*)g.row_ptr.data(), sizeof(long long)*(g.N+1));
    f.read((char*)g.col_ind.data(), sizeof(int)*g.M);
    f.read((char*)g.w.data(), sizeof(float)*g.M);

    // Metadatos opcionales (si existen)
    if(f.peek()!=EOF){
        f.read((char*)&g.rows, sizeof(g.rows));
        f.read((char*)&g.cols, sizeof(g.cols));
        f.read((char*)&g.diag8, sizeof(g.diag8));
        f.read((char*)&g.has_coords, sizeof(g.has_coords));
        if(g.has_coords){
            g.x.resize(g.N); g.y.resize(g.N);
            f.read((char*)g.x.data(), sizeof(float)*g.N);
            f.read((char*)g.y.data(), sizeof(float)*g.N);
        }
    }
    return g;
}

// -------------------- Generador Grid --------------------
CSR gen_grid(int rows, int cols, bool diag8,
             float wmin, float wmax, unsigned seed){
    const long long N = 1LL*rows*cols;

    std::vector<std::pair<int,int>> nbrs = {{ 1, 0},{-1, 0},{ 0, 1},{ 0,-1}};
    if(diag8){
        nbrs.push_back({ 1, 1}); nbrs.push_back({ 1,-1});
        nbrs.push_back({-1, 1}); nbrs.push_back({-1,-1});
    }

    std::vector<long long> deg(N, 0);
    for(int r=0;r<rows;++r) for(int c=0;c<cols;++c){
        for(auto [dr,dc] : nbrs){
            int rr=r+dr, cc=c+dc;
            if(rr>=0 && rr<rows && cc>=0 && cc<cols)
                deg[id_from_rc(r,c,cols)]++;
        }
    }

    CSR g; g.N = (int)N;
    g.row_ptr.assign(N+1, 0);
    for(long long i=0;i<N;++i) g.row_ptr[i+1] = g.row_ptr[i] + deg[i];

    g.M = g.row_ptr.back();
    g.col_ind.resize(g.M);
    g.w.resize(g.M);

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> U(wmin, wmax);

    for(int r=0;r<rows;++r) for(int c=0;c<cols;++c){
        const int u = id_from_rc(r,c,cols);
        long long off = g.row_ptr[u];
        for(auto [dr,dc] : nbrs){
            int rr=r+dr, cc=c+dc;
            if(rr>=0 && rr<rows && cc>=0 && cc<cols){
                int v = id_from_rc(rr,cc,cols);
                g.col_ind[off] = v;
                g.w[off]       = U(rng);
                ++off;
            }
        }
    }

    // Coordenadas para heurística (A*)
    g.has_coords = true;
    g.x.resize(N); g.y.resize(N);
    for(int r=0;r<rows;++r) for(int c=0;c<cols;++c){
        int u = id_from_rc(r,c,cols);
        g.x[u] = static_cast<float>(c);
        g.y[u] = static_cast<float>(r);
    }
    g.rows = rows; g.cols = cols; g.diag8 = diag8;
    return g;
}

// -------------------- Generador Erdős–Rényi --------------------
CSR gen_er(int N, long long M, float wmin, float wmax, unsigned seed, bool directed){
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> U(0, N-1);
    std::uniform_real_distribution<float> W(wmin, wmax);

    std::vector<std::vector<std::pair<int,float>>> adj(N);
    long long added = 0;
    while(added < M){
        int u = U(rng), v = U(rng);
        if(u==v) continue;
        float w = W(rng);
        adj[u].push_back({v, w}); added++;
        if(!directed){
            adj[v].push_back({u, W(rng)}); added++;
        }
    }

    CSR g; g.N = N;
    g.row_ptr.assign(N+1, 0);
    for(int u=0; u<N; ++u) g.row_ptr[u+1] = g.row_ptr[u] + (long long)adj[u].size();

    g.M = g.row_ptr.back();
    g.col_ind.resize(g.M);
    g.w.resize(g.M);

    for(int u=0; u<N; ++u){
        long long off = g.row_ptr[u];
        for(auto &e : adj[u]){
            g.col_ind[off] = e.first;
            g.w[off]       = e.second;
            ++off;
        }
    }
    g.has_coords = false;
    g.rows = 0; g.cols = 0; g.diag8 = false;
    return g;
}

// -------------------- Ruta --------------------
int path_length(int s, int t, const std::vector<int>& parent){
    if(s==t) return 1;
    if(t<0 || t>= (int)parent.size()) return 0;
    int len = 0, guard = 0, N = (int)parent.size();
    for(int v=t; v!=-1; v=parent[v]){
        ++len;
        if(v==s) return len;          // incluye s y t
        if(++guard > N) break;        // guardia contra ciclos/malas asignaciones
    }
    return 0; // no hay ruta válida hasta s
}

float path_cost(int s, int t, const CSR& g, const std::vector<int>& parent){
    if(s==t) return 0.0f;
    if(t<0 || t>= (int)parent.size()) return INFINITY;
    float cost = 0.0f;
    int v = t, guard = 0, N = (int)parent.size();
    while(v != -1 && v != s){
        int p = parent[v];
        if(p == -1) return INFINITY;
        // buscar arista p->v en CSR
        bool found = false;
        for(long long e=g.row_ptr[p]; e<g.row_ptr[p+1]; ++e){
            if(g.col_ind[e] == v){ cost += g.w[e]; found = true; break; }
        }
        if(!found) return INFINITY;
        v = p;
        if(++guard > N) return INFINITY;
    }
    return (v==s) ? cost : INFINITY;
}

// -------------------- Heurística --------------------
float heuristic_grid(const CSR& g, int u, int t){
    if(!g.has_coords) return 0.0f;
    const float dx = std::fabs(g.x[u] - g.x[t]);
    const float dy = std::fabs(g.y[u] - g.y[t]);
    if(g.diag8){
        // Octile (aprox euclídea para 8-direcciones)
        const float D = 1.0f, D2 = 1.41421356f;
        return D*(dx+dy) + (D2 - 2*D)*std::min(dx,dy);
    }
    // Manhattan (4-direcciones)
    return dx + dy;
}
