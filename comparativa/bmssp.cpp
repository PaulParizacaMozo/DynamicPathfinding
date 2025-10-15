#include "bmssp.h"
#include <queue>
#include <unordered_map>
#include <limits>
#include <chrono>
#include <algorithm>

struct Nodo {
    Point p;
    double costo;
    bool operator>(const Nodo& o) const { return costo > o.costo; }
};

static std::vector<Point> vecinos4(const Point& p, int H, int W) {
    static const int dr[4] = {1,-1,0,0};
    static const int dc[4] = {0,0,1,-1};
    std::vector<Point> v; v.reserve(4);
    for (int i = 0; i < 4; ++i) {
        int nr = p.first + dr[i];
        int nc = p.second + dc[i];
        if (nr >= 0 && nr < H && nc >= 0 && nc < W) v.push_back({nr,nc});
    }
    return v;
}

BMSSPResultado runBMSSP(const Grid& grilla,
                        const std::vector<Point>& fuentes,
                        Point objetivo,
                        double limiteB)
{
    const int H = (int)grilla.size();
    const int W = H ? (int)grilla[0].size() : 0;

    auto t0 = std::chrono::high_resolution_clock::now();

    const double INF = std::numeric_limits<double>::infinity();
    auto id = [W](Point p)->long long { return (long long)p.first * (long long)W + (long long)p.second; };


    std::vector<std::vector<double>> dist(H, std::vector<double>(W, INF));
    std::unordered_map<long long, Point> padre;

    std::priority_queue<Nodo, std::vector<Nodo>, std::greater<Nodo>> abiertos;


    for (const auto& s : fuentes) {
        if (s.first < 0 || s.first >= H || s.second < 0 || s.second >= W) continue;
        if (grilla[s.first][s.second] == 1) continue; 
        if (dist[s.first][s.second] > 0.0) {
            dist[s.first][s.second] = 0.0;
            abiertos.push({s, 0.0});
        }
    }

    int expansiones = 0;

    while (!abiertos.empty()) {
        Nodo nodo = abiertos.top(); abiertos.pop();
        Point u = nodo.p;
        double du = nodo.costo;

        if (du != dist[u.first][u.second]) continue;

        if (du > limiteB) break;

        ++expansiones;

        if (u == objetivo) break;

        for (const auto& v : vecinos4(u, H, W)) {
            if (grilla[v.first][v.second] == 1) continue;
            double nuevo = du + 1.0; // costo uniforme
            if (nuevo < dist[v.first][v.second] && nuevo <= limiteB) {
                dist[v.first][v.second] = nuevo;
                padre[id(v)] = u;
                abiertos.push({v, nuevo});
            }
        }
    }

    std::vector<Point> ruta;
    if (objetivo.first >= 0 && objetivo.first < H &&
        objetivo.second >= 0 && objetivo.second < W &&
        dist[objetivo.first][objetivo.second] != INF &&
        dist[objetivo.first][objetivo.second] <= limiteB)
    {
        Point cur = objetivo;
        ruta.push_back(cur);
        while (true) {
            auto it = padre.find(id(cur));
            if (it == padre.end()) break;
            cur = it->second;
            ruta.push_back(cur);
        }
        std::reverse(ruta.begin(), ruta.end());
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double tiempo = std::chrono::duration<double>(t1 - t0).count();

    return { ruta, expansiones, tiempo, 0.0  };
}
