#include <bits/stdc++.h>
using namespace std;

/*
  Dijkstra en grilla 2D (4-dir, coste 1).
  Entrada:
    rows cols sr sc er ec
    grid (0 libre, 1 obstáculo)
  Salida:
    Visited:
    r c
    ...
    Parents:
    r c pr pc
    ...
    Path:
    r c
    ...
*/

struct Node { int r,c,dist; bool operator>(const Node& o) const { return dist>o.dist; } };
static const int DR[4]={-1,1,0,0}, DC[4]={0,0,-1,1};
static inline bool inb(int r,int c,int R,int C){ return r>=0&&r<R&&c>=0&&c<C; }

int main(){
  ios::sync_with_stdio(false); cin.tie(nullptr);
  int R,C,sr,sc,er,ec;
  if(!(cin>>R>>C>>sr>>sc>>er>>ec)){ cout<<"Visited:\nParents:\nPath:\n"; return 0; }

  vector<vector<int>> g(R, vector<int>(C));
  for(int r=0;r<R;r++) for(int c=0;c<C;c++){ int v; cin>>v; g[r][c]=v?1:0; }

  auto freeCell=[&](int r,int c){ return inb(r,c,R,C) && g[r][c]==0; };
  if(!freeCell(sr,sc) || !freeCell(er,ec)){ cout<<"Visited:\nParents:\nPath:\n"; return 0; }
  if(sr==er && sc==ec){ cout<<"Visited:\n"<<sr<<" "<<sc<<"\nParents:\nPath:\n"<<sr<<" "<<sc<<"\n"; return 0; }

  const int INF=INT_MAX;
  vector<vector<int>> dist(R, vector<int>(C, INF));
  vector<vector<pair<int,int>>> par(R, vector<pair<int,int>>(C, {-1,-1}));
  vector<vector<bool>> closed(R, vector<bool>(C,false));
  priority_queue<Node, vector<Node>, greater<Node>> pq;
  vector<pair<int,int>> visitedOrder;

  dist[sr][sc]=0; pq.push({sr,sc,0});

  while(!pq.empty()){
    auto cur=pq.top(); pq.pop();
    int r=cur.r, c=cur.c, d=cur.dist;
    if(closed[r][c]) continue;
    closed[r][c]=true;
    visitedOrder.push_back({r,c});
    if(r==er && c==ec) break;

    for(int k=0;k<4;k++){
      int nr=r+DR[k], nc=c+DC[k];
      if(!inb(nr,nc,R,C) || g[nr][nc]==1) continue;
      int nd=d+1;
      if(nd<dist[nr][nc]){
        dist[nr][nc]=nd;
        par[nr][nc]={r,c};
        pq.push({nr,nc,nd});
      }
    }
  }

  // Path final
  vector<pair<int,int>> path;
  if(dist[er][ec]!=INF){
    int r=er,c=ec;
    while(!(r==sr && c==sc)){
      path.push_back({r,c});
      auto [pr,pc]=par[r][c];
      if(pr==-1&&pc==-1){ path.clear(); break; }
      r=pr;c=pc;
    }
    if(!path.empty()){ path.push_back({sr,sc}); reverse(path.begin(),path.end()); }
  }

  cout<<"Visited:\n";
  for(auto [r,c]:visitedOrder) cout<<r<<" "<<c<<"\n";

  cout<<"Parents:\n";
  for(int r=0;r<R;r++) for(int c=0;c<C;c++){
    auto [pr,pc]=par[r][c];
    if(pr!=-1 || pc!=-1) cout<<r<<" "<<c<<" "<<pr<<" "<<pc<<"\n";
  }

  cout<<"Path:\n";
  for(auto [r,c]:path) cout<<r<<" "<<c<<"\n";
  return 0;
}

