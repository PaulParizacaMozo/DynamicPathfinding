#include "dijkstra.h"
#include <queue>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

struct Node {
    Point p;
    double cost;
    bool operator>(const Node &o) const { return cost > o.cost; }
};

static std::vector<Point> neighbors(const Point &p, int H, int W) {
    std::vector<Point> n;
    int dr[4] = {1,-1,0,0};
    int dc[4] = {0,0,1,-1};
    for (int i = 0; i < 4; i++) {
        int nr = p.first + dr[i];
        int nc = p.second + dc[i];
        if (nr >= 0 && nr < H && nc >= 0 && nc < W)
            n.push_back({nr, nc});
    }
    return n;
}

DijkstraResult runDijkstra(const Grid &grid, Point start, Point goal) {
    int H = grid.size();
    int W = grid[0].size();
    auto t0 = std::chrono::high_resolution_clock::now();

    std::unordered_map<long long,double> g;
    std::unordered_map<long long,Point> parent;
    auto encode = [W](Point p){ return (long long)p.first * W + p.second; };

    std::priority_queue<Node,std::vector<Node>,std::greater<Node>> open;
    open.push({start, 0});
    g[encode(start)] = 0;

    int expansions = 0;
    std::unordered_set<long long> visited;

    while(!open.empty()) {
        Node node = open.top(); open.pop();
        long long id = encode(node.p);
        if (visited.count(id)) continue;
        visited.insert(id);
        expansions++;

        if (node.p == goal) break;

        for (auto nb : neighbors(node.p, H, W)) {
            if (grid[nb.first][nb.second] == 1) continue;
            double new_cost = g[id] + 1;
            long long nbid = encode(nb);
            if (!g.count(nbid) || new_cost < g[nbid]) {
                g[nbid] = new_cost;
                parent[nbid] = node.p;
                open.push({nb, new_cost});
            }
        }
    }

    std::vector<Point> path;
    Point cur = goal;
    while (cur != start) {
        if (!parent.count(encode(cur))) break;
        path.push_back(cur);
        cur = parent[encode(cur)];
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    return {path, expansions, elapsed, 0.0};
}
