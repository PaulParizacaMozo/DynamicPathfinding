#include <bits/stdc++.h>
using namespace std;

/*
  D* Lite persistente (4 direcciones, coste unitario, obstáculos bloquean)
  Protocolo por stdin (una orden por línea salvo INIT que incluye filas posteriores):

    INIT rows cols sr sc er ec
    <rows líneas con cols enteros 0/1>
      -> responde: "OK\nEND\n"  (y mantiene el estado interno)
    UPDATE r c cost
      -> cost = 1 (libre) o 1000000000 (bloqueado) por simplicidad
      -> responde: "OK\nEND\n"
    MOVE r c
      -> actualiza km y start; responde: "OK\nEND\n"
    PLAN
      -> corre ComputeShortestPath() y responde:
         Visited:\n...
         Parents:\n...
         Path:\n...
         END\n

  - Heurística Manhattan (consistente).
  - g/rhs/cola U, km, start/goal persisten entre comandos.
  - "Visited" registra los nodos realmente procesados en esta corrida de PLAN.
  - "Parents" se alimenta en updateVertex (mejor predecesor) y también durante la reconstrucción final.
*/

struct Key {
    double k1, k2;
    bool operator<(const Key& o) const {
        if (k1 != o.k1) return k1 < o.k1;
        return k2 < o.k2;
    }
};
struct PQItem {
    int id; Key key;
    bool operator<(const PQItem& o) const {
        if (key.k1 != o.key.k1) return key.k1 > o.key.k1;
        return key.k2 > o.key.k2;
    }
};
static const double INF = 1e18;
static const double BLOCK = 1e9;

struct DStarLite {
    int rows=0, cols=0;
    int Sstart=-1, Sgoal=-1;
    long long km=0;

    vector<int> grid;      // 0 libre, 1 obstáculo
    vector<double> g, rhs; // valores D* Lite
    vector<int> parent;    // para UI
    vector<Key> bestKey;   // para lazy deletion de U
    priority_queue<PQItem> U;

    DStarLite() {}

    int id(int r,int c) const { return r*cols+c; }
    bool inb(int r,int c) const { return r>=0 && r<rows && c>=0 && c<cols; }

    int manhattan(int a, int b) const {
        int ar=a/cols, ac=a%cols, br=b/cols, bc=b%cols;
        return abs(ar-br)+abs(ac-bc);
    }
    Key calcKey(int s) const {
        double val=min(g[s], rhs[s]);
        return Key{ val + (double)manhattan(Sstart, s) + (double)km, val };
    }
    bool inU(int s) const {
        return !(bestKey[s].k1>=INF/2 && bestKey[s].k2>=INF/2);
    }
    void pushU(int s){
        Key k=calcKey(s);
        bestKey[s]=k;
        U.push(PQItem{s,k});
    }
    void removeFromU(int s){
        bestKey[s]=Key{INF,INF}; // lazy
    }
    vector<int> neighbors4(int u) const {
        int r=u/cols, c=u%cols;
        static const int dr[4]={-1,1,0,0};
        static const int dc[4]={0,0,-1,1};
        vector<int> out; out.reserve(4);
        for(int k=0;k<4;k++){
            int nr=r+dr[k], nc=c+dc[k];
            if(!inb(nr,nc)) continue;
            if(grid[id(nr,nc)]==1) continue;
            out.push_back(id(nr,nc));
        }
        return out;
    }
    double cost(int /*a*/, int /*b*/) const { return 1.0; }

    void init(int R,int C,int sr,int sc,int er,int ec,const vector<int>& G){
        rows=R; cols=C; grid=G;
        int s=id(sr,sc), t=id(er,ec);
        grid[s]=0; grid[t]=0;
        Sstart=s; Sgoal=t; km=0;
        int N=rows*cols;
        g.assign(N, INF); rhs.assign(N, INF); parent.assign(N,-1);
        bestKey.assign(N, Key{INF,INF});
        U=priority_queue<PQItem>();
        rhs[Sgoal]=0.0; pushU(Sgoal);
    }

    void updateCell(int r,int c,double newCost){
        // newCost == BLOCK → obstáculo; == 1 → libre
        int u=id(r,c);
        int prev = grid[u];
        grid[u] = (newCost>=BLOCK/2)? 1:0;
        // Actualizar vecinos y el propio si corresponde:
        // Si celda cambia a obstáculo, ya no es transitable ni pred de otros.
        // Llamamos updateVertex a cada sucesor/vecino afectado.
        updateVertex(u);
        for(int s: neighbors4(u)) updateVertex(s);
    }

    void moveStart(int r,int c){
        int newS=id(r,c);
        km += manhattan(Sstart,newS);
        Sstart=newS;
    }

    void updateVertex(int s){
        if (s!=Sgoal){
            double new_rhs=INF; int bestPred=-1;
            for(int sp: neighbors4(s)){
                if(g[sp]>=INF/2) continue;
                double cand=g[sp]+cost(sp,s);
                if(cand<new_rhs){ new_rhs=cand; bestPred=sp; }
            }
            rhs[s]=new_rhs;
            if(bestPred!=-1) parent[s]=bestPred;
        }
        if (g[s]!=rhs[s]) pushU(s);
        else if (inU(s)) removeFromU(s);
    }

    void computeShortestPath(vector<pair<int,int>>& visitedOrder){
        auto keyLess=[&](const Key&a,const Key&b){
            if(a.k1!=b.k1) return a.k1<b.k1;
            return a.k2<b.k2;
        };
        while(true){
            if(U.empty()){
                if(g[Sstart]==rhs[Sstart]) break;
            }else{
                PQItem t=U.top();
                if(!keyLess(t.key, calcKey(Sstart)) && g[Sstart]==rhs[Sstart]) break;
            }
            PQItem it=U.top(); U.pop();
            if(!(it.key.k1==bestKey[it.id].k1 && it.key.k2==bestKey[it.id].k2)) continue; // lazy
            int u=it.id;
            visitedOrder.emplace_back(u/cols,u%cols);

            Key k_old=it.key, k_new=calcKey(u);
            if (keyLess(k_new,k_old)){
                pushU(u);
                continue;
            } else if (g[u]>rhs[u]){
                g[u]=rhs[u];
                removeFromU(u);
                for(int s: neighbors4(u)) updateVertex(s);
            } else {
                double g_old=g[u];
                g[u]=INF;
                updateVertex(u);
                for(int s: neighbors4(u)) updateVertex(s);
            }
        }
    }

    vector<pair<int,int>> reconstructPath(){
        vector<pair<int,int>> path;
        if(g[Sstart]>=INF/2) return path; // vacío
        int cur=Sstart;
        path.emplace_back(cur/cols, cur%cols);
        int guard=0, GUARD=rows*cols+5;
        while(cur!=Sgoal && guard++<GUARD){
            double best=INF; int bestN=-1;
            for(int nb: neighbors4(cur)){
                double cand=g[nb]+cost(cur,nb);
                if(cand<best){ best=cand; bestN=nb; }
            }
            if(bestN==-1 || g[bestN]>=INF/2) break;
            parent[bestN]=cur; // útil para UI
            cur=bestN;
            path.emplace_back(cur/cols, cur%cols);
        }
        if(cur!=Sgoal && g[Sgoal]<INF/2){
            path.emplace_back(Sgoal/cols,Sgoal%cols);
        }
        return path;
    }
};

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    DStarLite dsl;
    string line;

    auto trim = [](string s){
        while(!s.empty() && (s.back()=='\r' || s.back()=='\n')) s.pop_back();
        return s;
    };

    while (true){
        if(!std::getline(cin, line)) break;
        line=trim(line);
        if(line.empty()) continue;

        string cmd; {
            stringstream ss(line); ss>>cmd;
        }

        if(cmd=="INIT"){
            stringstream ss(line);
            string _; int R,C,sr,sc,er,ec;
            ss>>_>>R>>C>>sr>>sc>>er>>ec;
            vector<int> G(R*C,0);
            for(int r=0;r<R;r++){
                string row; getline(cin,row);
                row=trim(row);
                if(row.empty()){ r--; continue; }
                stringstream rs(row);
                for(int c=0;c<C;c++){ int v; rs>>v; G[r*C+c]=v; }
            }
            dsl.init(R,C,sr,sc,er,ec,G);
            cout<<"OK\nEND\n"<<flush;
        }
        else if(cmd=="UPDATE"){
            stringstream ss(line);
            string _; int r,c; double cost;
            ss>>_>>r>>c>>cost;
            dsl.updateCell(r,c,cost);
            cout<<"OK\nEND\n"<<flush;
        }
        else if(cmd=="MOVE"){
            stringstream ss(line);
            string _; int r,c; ss>>_>>r>>c;
            dsl.moveStart(r,c);
            cout<<"OK\nEND\n"<<flush;
        }
        else if(cmd=="PLAN"){
            vector<pair<int,int>> visited;
            dsl.computeShortestPath(visited);
            auto path = dsl.reconstructPath();

            cout<<"Visited:\n";
            for(auto &p: visited) cout<<p.first<<" "<<p.second<<"\n";
            cout<<"Parents:\n";
            for(size_t s=0;s<dsl.parent.size();++s){
                if(dsl.parent[s]!=-1){
                    int r=s/dsl.cols, c=s%dsl.cols;
                    int pr=dsl.parent[s]/dsl.cols, pc=dsl.parent[s]%dsl.cols;
                    cout<<r<<" "<<c<<" "<<pr<<" "<<pc<<"\n";
                }
            }
            cout<<"Path:\n";
            for(auto &p: path) cout<<p.first<<" "<<p.second<<"\n";
            cout<<"END\n"<<flush;
        }
        else if(cmd=="EXIT"){
            cout<<"BYE\nEND\n"<<flush;
            break;
        }
        else {
            cout<<"ERR unknown\nEND\n"<<flush;
        }
    }
    return 0;
}

