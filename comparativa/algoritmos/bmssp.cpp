
#include "utils.hpp"
#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <limits>
#include <cmath>
#include <algorithm>
#include <climits>

// =================== numerics & helpers ===================
static constexpr float INF_F = std::numeric_limits<float>::infinity();
static constexpr float EPS   = 1e-5f;
static inline bool improves(float nd, float dv){ return nd + EPS < dv; }
static inline bool ties    (float nd, float dv){ return std::fabs(nd - dv) <= EPS; }

// =================== tiny set ===================
struct NodeSet {
    std::unordered_set<int> s;
    void add(int v){ s.insert(v); }
    bool has(int v) const { return s.find(v)!=s.end(); }
    int  size() const { return (int)s.size(); }
    std::vector<int> to_vec() const { return std::vector<int>(s.begin(), s.end()); }
};

// =================== BucketQueue (Δ-stepping) ===================
struct BucketQueue {
    std::vector<std::vector<int>> buckets;
    std::unordered_map<int,int> pos; // nodo -> idx bucket
    float delta;
    size_t minIdx;

    explicit BucketQueue(float d=1.0f): delta(std::max(1e-6f,d)), minIdx(0) {}

    inline int idx_of(float dist) const {
        if(!std::isfinite(dist) || dist < 0) return INT_MAX/4;
        long long idx = (long long)std::floor(dist / delta + 1e-9); // tolerante
        if(idx < 0) idx = 0;
        if(idx > (long long)INT_MAX/4) idx = INT_MAX/4;
        return (int)idx;
    }

    void ensure(int idx){
        if(idx<0) return;
        if((size_t)(idx+1) > buckets.size()) buckets.resize(idx+1);
    }

    void insert(int v, float dist){
        int idx = idx_of(dist);
        ensure(idx);
        buckets[idx].push_back(v);
        pos[v] = idx;
        if((size_t)idx < minIdx) minIdx = idx;
    }

    void decreaseKey(int v, float newd){
        auto it = pos.find(v);
        if(it != pos.end()){
            int oldIdx = it->second;
            auto &B = buckets[oldIdx];
            for(size_t i=0;i<B.size();++i){
                if(B[i]==v){ B[i]=B.back(); B.pop_back(); break; }
            }
        }
        insert(v, newd);
    }

    bool extractMin(int &out_v){
        while(minIdx < buckets.size() && buckets[minIdx].empty()) ++minIdx;
        if(minIdx >= buckets.size()) return false;
        auto &B = buckets[minIdx];
        out_v = B.back(); B.pop_back();
        pos.erase(out_v);
        return true;
    }

    bool empty() const {
        size_t i=minIdx;
        while(i<buckets.size()){
            if(!buckets[i].empty()) return false;
            ++i;
        }
        return true;
    }
};

// =================== median-of-three pivot ===================
static int medianOfThreePivot(const NodeSet& S, const std::vector<float>& db){
    auto nodes = S.to_vec();
    if(nodes.empty()) return -1;
    if(nodes.size() <= 3) return nodes[nodes.size()/2];

    std::vector<int> tmp = nodes;
    std::nth_element(tmp.begin(), tmp.begin(), tmp.end(),
        [&](int a,int b){ return db[a] < db[b]; });
    std::nth_element(tmp.begin(), tmp.begin()+tmp.size()/2, tmp.end(),
        [&](int a,int b){ return db[a] < db[b]; });
    std::nth_element(tmp.begin(), tmp.end()-1, tmp.end(),
        [&](int a,int b){ return db[a] < db[b]; });

    int first = tmp.front();
    int middle= tmp[tmp.size()/2];
    int last  = tmp.back();

    int cand[3] = {first, middle, last};
    std::sort(cand, cand+3, [&](int a,int b){ return db[a] < db[b]; });
    return cand[1];
}

// =================== bounded SSSP core ===================
// Si todas las aristas pesan 1 y B finita -> Dial buckets (O(m+B))
// Si no, fallback a Δ-stepping sencillo
static void dijkstraDeltaSteppingBounded(
    const CSR& g,
    const NodeSet& S,
    float B,
    float delta,
    std::vector<float>& db,
    std::vector<int>& parent
){
    // Detecta si todas las aristas pesan 1
    bool all_weights_one = true;
    for (int u = 0; u < g.N && all_weights_one; ++u) {
        for (long long e = g.row_ptr[u]; e < g.row_ptr[u+1]; ++e) {
            if (std::fabs(g.w[e] - 1.0f) > 1e-9f) { all_weights_one = false; break; }
        }
    }

    if (all_weights_one && std::isfinite(B)) {
        // -------- Dial / Radix buckets: O(m + B) --------
        // Incluye la capa con distancia == floor(B) si db[v] < B (con tolerancia)
        const float Beff = B + 1e-6f;
        int maxD = (int)std::floor(Beff);
        if (maxD < 1) maxD = 1;

        std::vector<std::vector<int>> buckets(maxD+1);
        std::vector<char> closed(g.N, 0);

        auto push = [&](int v) {
            float dv = db[v];
            if (!(dv + 1e-6f < B)) return;           // dv < B con tolerancia
            int idx = (int)std::floor(dv + 1e-6f);   // index tolerante
            if (idx < 0) idx = 0;
            if (idx > maxD) idx = maxD;
            buckets[idx].push_back(v);
        };

        for (int v : S.s) if (db[v] + 1e-6f < B) push(v);

        for (int i = 0; i <= maxD; ++i) {
            auto &Q = buckets[i];
            while (!Q.empty()) {
                int u = Q.back(); Q.pop_back();
                if (closed[u]) continue;
                if (!(db[u] + 1e-6f < B)) continue;
                // tolerante: el bucket correcto para db[u]
                int iu = (int)std::floor(db[u] + 1e-6f);
                if (iu != i) continue; // obsoleto
                closed[u] = 1;

                for (long long e = g.row_ptr[u]; e < g.row_ptr[u+1]; ++e) {
                    int v = g.col_ind[e];
                    float nd = db[u] + 1.0f; // peso 1 exacto
                    if (!(nd + 1e-6f < B)) continue;
                    if (improves(nd, db[v])) {
                        db[v] = nd; parent[v] = u;
                        push(v);
                    } else if (ties(nd, db[v]) && parent[v] == -1) {
                        parent[v] = u; // cosido en empate
                    }
                }
            }
        }
        return;
    }

    // -------- fallback: Δ-stepping general --------
    BucketQueue pq(delta);
    std::vector<char> vis(g.N, 0);
    for (int v : S.s) if (db[v] + 1e-6f < B) pq.insert(v, db[v]);

    int u;
    while (pq.extractMin(u)) {
        if (vis[u]) continue;
        float du = db[u];
        if (!(du + 1e-6f < B)) continue;
        vis[u] = 1;

        for (long long e = g.row_ptr[u]; e < g.row_ptr[u+1]; ++e) {
            int v = g.col_ind[e];
            float nd = du + g.w[e];
            if (!(nd + 1e-6f < B)) continue;
            if (improves(nd, db[v])) {
                db[v] = nd; parent[v] = u;
                pq.decreaseKey(v, nd);
            } else if (ties(nd, db[v]) && parent[v] == -1) {
                parent[v] = u;
            }
        }
    }
}

// =================== partición y recursión principal ===================
static void BMSSP_recursive(
    const CSR& g,
    float B,
    NodeSet S,
    std::vector<float>& db,
    std::vector<int>& parent,
    float delta
){
    if(S.size() == 0) return;

    // Caso base pragmático
    if(S.size() == 1 || B <= 1.0f + 1e-9f){
        dijkstraDeltaSteppingBounded(g, S, B, delta, db, parent);
        return;
    }

    // Pivot y bound = min(B, db[pivot])
    int pivot = medianOfThreePivot(S, db);
    float bound = B;
    if(pivot != -1) bound = std::min(B, db[pivot]);

    // Si bound ≈ B, bounded directo
    if(std::fabs(bound - B) <= 1e-9f){
        dijkstraDeltaSteppingBounded(g, S, B, delta, db, parent);
        return;
    }

    // Bounded hasta "bound"
    dijkstraDeltaSteppingBounded(g, S, bound, delta, db, parent);

    // Partición en left (<= bound) y right (bound, B)
    NodeSet left, right;
    for(int u=0; u<g.N; ++u){
        if(!std::isfinite(db[u])) continue;
        if(db[u] <= bound + 1e-6f) left.add(u);
        else if(db[u] + 1e-6f < B) right.add(u);
    }

    // Recursión si particiones útiles
    if(left.size() > 0 && left.size() < S.size())
        BMSSP_recursive(g, bound, left, db, parent, delta);

    if(right.size() > 0 && right.size() < S.size())
        BMSSP_recursive(g, B, right, db, parent, delta);
}

// =================== reconstrucción de seguridad (igualdades) ===================
static bool reconstruct_bfs_equalities(
    const CSR& g, int s, int t,
    const std::vector<float>& db,
    std::vector<int>& parent
){
    if(!std::isfinite(db[t])) return false;
    std::vector<char> vis(g.N, 0);
    std::queue<int> q;
    parent.assign(g.N, -1);
    vis[s]=1; q.push(s);
    while(!q.empty()){
        int u=q.front(); q.pop();
        if(u==t) break;
        for(long long e=g.row_ptr[u]; e<g.row_ptr[u+1]; ++e){
            int v=g.col_ind[e];
            if(vis[v]) continue;
            if(!std::isfinite(db[v])) continue;
            if(ties(db[v], db[u] + g.w[e])){
                vis[v]=1; parent[v]=u; q.push(v);
            }
        }
    }
    return parent[t] != -1;
}

// =================== API pública ===================
bool bmssp_run(const CSR& g, int s, int t, float B, std::vector<int>& parent){
    parent.assign(g.N, -1);
    if(s<0||s>=g.N||t<0||t>=g.N) return false;

    std::vector<float> db(g.N, INF_F);
    db[s] = 0.0f;

    if(!(B > 0.0f) || std::isinf(B)) B = INF_F;

    float delta = 1.0f; // grids unitarios

    NodeSet S; S.add(s);
    BMSSP_recursive(g, B, S, db, parent, delta);

    // Validación permisiva (tolera igualdad por flotantes)
    if(!std::isfinite(db[t]) || (db[t] > B + 1e-6f)) return false;

    // Si ya hay cadena, valida llegada a s
    if(parent[t] != -1){
        int v=t, guard=0;
        while(v!=-1 && v!=s){ v=parent[v]; if(++guard>g.N){ parent[t]=-1; break; } }
        if(v==s) return true;
    }

    // Reconstrucción por igualdades si faltara coser
    if(reconstruct_bfs_equalities(g, s, t, db, parent)) return true;

    return false;
}
