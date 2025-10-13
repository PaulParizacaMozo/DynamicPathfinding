#pragma once
#include <vector>
#include <string>
#include <chrono>

// -------- Representación del grafo (CSR) --------
struct CSR {
    int N = 0;                  // # vértices
    long long M = 0;            // # aristas
    std::vector<long long> row_ptr; // tamaño N+1
    std::vector<int>        col_ind; // tamaño M
    std::vector<float>      w;       // tamaño M

    // (Opcional) coordenadas para heurística en A*/grid
    bool has_coords = false;
    std::vector<float> x, y;    // tamaño N cuando has_coords=true

    // (Opcional) metadatos de grid
    int  rows = 0, cols = 0;
    bool diag8 = false;
};

// -------- Utilidades de E/S --------
void save_csr_bin(const CSR& g, const std::string& path);
CSR  load_csr_bin(const std::string& path);

// -------- Generadores --------
CSR gen_grid(int rows, int cols, bool diag8,
             float wmin=1.0f, float wmax=1.0f, unsigned seed=42);

CSR gen_er(int N, long long M,
           float wmin=1.0f, float wmax=10.0f,
           unsigned seed=42, bool directed=true);

// -------- Temporizador --------
struct Timer {
    using clk = std::chrono::high_resolution_clock;
    clk::time_point t0;
    void start(){ t0 = clk::now(); }
    double ms() const {
        return std::chrono::duration<double, std::milli>(clk::now() - t0).count();
    }
};

// -------- Ruta --------
int   path_length(int s, int t, const std::vector<int>& parent);
float path_cost(int s, int t, const CSR& g, const std::vector<int>& parent); // opcional

// -------- Heurística (A*) --------
// Si el grafo no tiene coords, devuelve 0 (A* -> Dijkstra).
float heuristic_grid(const CSR& g, int u, int t);

// -------- Firmas de algoritmos (se implementan en sus .cpp) --------
// Deben llenar 'parent' y devolver true si existe ruta s->t.
bool dijkstra_run(const CSR& g, int s, int t, std::vector<int>& parent);
bool astar_run   (const CSR& g, int s, int t, std::vector<int>& parent);
bool bmssp_run   (const CSR& g, int s, int t, float B, std::vector<int>& parent);
bool dstar_lite_run_static(const CSR& g, int s, int t, std::vector<int>& parent);
