#include "dstar_lite.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <cmath>
#include <chrono>
#include <algorithm>

struct Clave {
    double k1, k2;
    bool operator<(const Clave& o) const {
        if (k1 != o.k1) return k1 < o.k1;
        return k2 < o.k2;
    }
};

struct Entrada {
    Clave clave;
    Point p;
    bool operator>(const Entrada& o) const { 
        if (clave.k1 != o.clave.k1) return clave.k1 > o.clave.k1;
        return clave.k2 > o.clave.k2;
    }
};

static inline double infinito() {
    return std::numeric_limits<double>::infinity();
}

static inline int absInt(int x){ return x < 0 ? -x : x; }

static inline double heuristica(const Point& a, const Point& b) {
    return std::abs(a.first - b.first) + std::abs(a.second - b.second);
}

static std::vector<Point> vecinos4(const Point& p, int H, int W) {
    static const int dr[4] = {1,-1,0,0};
    static const int dc[4] = {0,0,1,-1};
    std::vector<Point> v;
    v.reserve(4);
    for (int i = 0; i < 4; ++i) {
        int nr = p.first + dr[i];
        int nc = p.second + dc[i];
        if (nr >= 0 && nr < H && nc >= 0 && nc < W) {
            v.push_back({nr,nc});
        }
    }
    return v;
}

DStarLiteResult runDStarLite(const Grid& grilla, Point inicio, Point objetivo) {
    const int H = (int)grilla.size();
    const int W = H ? (int)grilla[0].size() : 0;

    auto t0 = std::chrono::high_resolution_clock::now();

    auto codificar = [W](const Point& p)->long long {
        return (long long)p.first * (long long)W + (long long)p.second;
    };

    std::unordered_map<long long, double> g;   // g(s)
    std::unordered_map<long long, double> rhs; // rhs(s)
    auto costo = [&](const Point& a, const Point& b)->double {
        if (grilla[b.first][b.second] == 1) return infinito();
        return 1.0;
    };

    double km = 0.0;

    auto calcularClave = [&](const Point& s)->Clave {
        long long id = codificar(s);
        double gs = g.count(id) ? g[id] : infinito();
        double rhss = rhs.count(id) ? rhs[id] : infinito();
        double min_gr = std::min(gs, rhss);
        return { min_gr + heuristica(inicio, s) + km, min_gr };
    };

    std::priority_queue<Entrada, std::vector<Entrada>, std::greater<Entrada>> abiertos;
    std::unordered_map<long long, Clave> clave_actual;

    auto empujar = [&](const Point& s) {
        Clave k = calcularClave(s);
        long long id = codificar(s);
        clave_actual[id] = k;
        abiertos.push({k, s});
    };

    std::function<void(const Point&)> actualizarVertice = [&](const Point& s) {
        long long id = codificar(s);
        if (!(s == objetivo)) {
            // rhs(s) = min_{s' succ de s} (g(s') + c(s,s'))
            double mejor = infinito();
            for (auto& nb : vecinos4(s, H, W)) {
                if (grilla[nb.first][nb.second] == 1) continue;
                long long idnb = codificar(nb);
                double gnb = g.count(idnb) ? g[idnb] : infinito();
                double c = costo(s, nb);
                if (c == infinito()) continue;
                double cand = gnb + c;
                if (cand < mejor) mejor = cand;
            }
            rhs[id] = mejor;
        }
        double gs = g.count(id) ? g[id] : infinito();
        double rhss = rhs.count(id) ? rhs[id] : infinito();
        if (gs != rhss) {
            empujar(s);
        }
    };

    g.clear(); rhs.clear();
    g[codificar(objetivo)] = infinito();
    rhs[codificar(objetivo)] = 0.0;
    empujar(objetivo);

    int expansiones = 0;

    auto clavesMenor = [&](const Clave& a, const Clave& b)->bool {
        return (a.k1 < b.k1) || (a.k1 == b.k1 && a.k2 < b.k2);
    };

    // computeShortestPath
    auto computeShortestPath = [&]() {
        while (!abiertos.empty()) {
            Entrada top = abiertos.top();
            Clave k_old = top.clave;
            Point u = top.p;
            long long idu = codificar(u);

            if (!clave_actual.count(idu) || !(k_old.k1 == clave_actual[idu].k1 && k_old.k2 == clave_actual[idu].k2)) {
                abiertos.pop();
                continue;
            }

            Clave k_new = calcularClave(u);
            Clave k_start = calcularClave(inicio);
            double g_start = g.count(codificar(inicio)) ? g[codificar(inicio)] : infinito();
            double rhs_start = rhs.count(codificar(inicio)) ? rhs[codificar(inicio)] : infinito();

            if (!clavesMenor(k_old, k_start) && !(g_start != rhs_start)) {
                break;
            }

            abiertos.pop();
            clave_actual.erase(idu);

            double gu = g.count(idu) ? g[idu] : infinito();
            double rh = rhs.count(idu) ? rhs[idu] : infinito();

            if (clavesMenor(k_old, k_new)) {
                empujar(u);
                continue;
            } else if (gu > rh) {
                g[idu] = rh;
                ++expansiones;
                for (auto& pred : vecinos4(u, H, W)) {
                    if (grilla[pred.first][pred.second] == 1) continue;
                    actualizarVertice(pred);
                }
            } else {
                double gu_old = gu;
                g[idu] = infinito();
                ++expansiones;
                actualizarVertice(u);
                for (auto& pred : vecinos4(u, H, W)) {
                    if (grilla[pred.first][pred.second] == 1) continue;
                    actualizarVertice(pred);
                }
            }
        }
    };

    computeShortestPath();

    std::vector<Point> ruta;
    ruta.reserve(H*W/2);

    auto esAlcanzable = [&](const Point& s)->bool {
        long long id = codificar(s);
        double gs = g.count(id) ? g[id] : infinito();
        return std::isfinite(gs) || (s == objetivo);
    };

    if (!esAlcanzable(inicio)) {
        auto t1 = std::chrono::high_resolution_clock::now();
        double tiempo = std::chrono::duration<double>(t1 - t0).count();
        return { {}, expansiones, tiempo, 0.0 };
    }

    Point actual = inicio;
    ruta.push_back(actual);
    const int limitePasos = H*W + 5;

    for (int pasos = 0; pasos < limitePasos && !(actual == objetivo); ++pasos) {
        double mejor = infinito();
        Point sig = actual;
        for (auto& nb : vecinos4(actual, H, W)) {
            if (grilla[nb.first][nb.second] == 1) continue;
            long long idnb = codificar(nb);
            double gnb = g.count(idnb) ? g[idnb] : infinito();
            double c = costo(actual, nb);
            if (!std::isfinite(c) || !std::isfinite(gnb)) continue;
            double cand = gnb + c;
            if (cand < mejor) {
                mejor = cand;
                sig = nb;
            }
        }
        if (sig == actual || !std::isfinite(mejor)) {
            ruta.clear();
            break;
        }
        actual = sig;
        ruta.push_back(actual);
    }
    if (ruta.empty() || !(ruta.back() == objetivo)) {
        ruta.clear();
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double tiempo = std::chrono::duration<double>(t1 - t0).count();

    return { ruta, expansiones, tiempo, 0.0 };
}
