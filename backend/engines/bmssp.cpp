// engines/bmssp.cpp
#include <bits/stdc++.h>
using namespace std;

/*
  BMSSP-lite para grillas: "Bounded Multi-Source Shortest Path"
  - Frontera inicial = {start} (+ extras via env)
  - B = bound (via env), no expande nodos con dist >= B
  - 4 direcciones, coste 1, obstáculos bloquean
  - Salida compatible: Visited:, Parents:, Path:
*/

struct Node {
    int r, c; int d;
    bool operator>(const Node& o) const { return d > o.d; }
};

static const int INF = 1e9;
static const int dr[4] = {-1, 1, 0, 0};
static const int dc[4] = { 0, 0,-1, 1};

static bool inb(int r,int c,int R,int C){ return r>=0 && r<R && c>=0 && c<C; }

static vector<pair<int,int>> parseExtraSources(const string& s){
    vector<pair<int,int>> out;
    if (s.empty()) return out;
    string tmp; stringstream ss(s);
    while(getline(ss,tmp,';')){
        if(tmp.empty()) continue;
        int r=-1,c=-1;
        for (char& ch: tmp) if (ch==',') ch=' ';
        stringstream ps(tmp);
        ps >> r >> c;
        if(ps && r>=0 && c>=0) out.emplace_back(r,c);
    }
    return out;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int R,C,sr,sc,er,ec;
    if(!(cin>>R>>C>>sr>>sc>>er>>ec)) return 0;

    vector<int> grid(R*C,0);
    auto id = [&](int r,int c){ return r*C+c; };

    for(int r=0;r<R;r++)
        for(int c=0;c<C;c++)
            cin >> grid[id(r,c)];

    // Forzar libres
    grid[id(sr,sc)] = 0;
    grid[id(er,ec)] = 0;

    // Bound opcional
    int B = INF;
    if(const char* b = getenv("BMSSP_BOUND")){
        long long v=INF;
        try{ v = stoll(string(b)); }catch(...){ v=INF; }
        if (v>=1 && v<INF) B = (int)v;
    }

    // Multi-source opcional
    vector<pair<int,int>> sources;
    sources.emplace_back(sr,sc);
    if(const char* ex = getenv("BMSSP_EXTRA_SOURCES")){
        auto extras = parseExtraSources(string(ex));
        for(auto &p: extras){
            int r=p.first,c=p.second;
            if(r>=0 && r<R && c>=0 && c<C && grid[id(r,c)]==0){
                if(!(r==sr && c==sc)) sources.emplace_back(r,c);
            }
        }
    }

    // Dijkstra multi-source con cota B
    vector<int> dist(R*C, INF);
    vector<pair<int,int>> parent(R*C, {-1,-1});
    vector<vector<bool>> seen(R, vector<bool>(C,false));
    priority_queue<Node, vector<Node>, greater<Node>> pq;

    for(auto &s: sources){
        dist[id(s.first,s.second)] = 0;
        pq.push({s.first, s.second, 0});
    }

    vector<pair<int,int>> visitedOrder;

    while(!pq.empty()){
        auto cur = pq.top(); pq.pop();
        int r=cur.r, c=cur.c, d=cur.d;
        if (d!=dist[id(r,c)]) continue;
        if (seen[r][c]) continue;
        seen[r][c] = true;
        visitedOrder.emplace_back(r,c);

        if (r==er && c==ec) break;
        if (d >= B) continue; // respeta cota

        for(int k=0;k<4;k++){
            int nr = r+dr[k], nc = c+dc[k];
            if(!inb(nr,nc,R,C)) continue;
            if(grid[id(nr,nc)]==1) continue; // bloqueado
            int nd = d+1;
            if (nd < dist[id(nr,nc)]){
                dist[id(nr,nc)] = nd;
                parent[id(nr,nc)] = {r,c};
                pq.push({nr,nc,nd});
            }
        }
    }

    // Reconstrucción del camino (desde el inicio "principal" sr,sc)
    vector<pair<int,int>> path;
    if (dist[id(er,ec)] < INF){
        int r=er, c=ec;
        while(!(r==sr && c==sc)){
            path.push_back({r,c});
            auto pr = parent[id(r,c)];
            r=pr.first; c=pr.second;
        }
        path.push_back({sr,sc});
        reverse(path.begin(), path.end());
    }

    // === SALIDA ===
    cout << "Visited:\n";
    for(auto &p: visitedOrder) cout << p.first << " " << p.second << "\n";

    cout << "Parents:\n";
    for(int r=0;r<R;r++){
        for(int c=0;c<C;c++){
            auto pr = parent[id(r,c)];
            if (pr.first!=-1){
                cout << r << " " << c << " " << pr.first << " " << pr.second << "\n";
            }
        }
    }

    cout << "Path:\n";
    for(auto &p: path) cout << p.first << " " << p.second << "\n";

    return 0;
}
