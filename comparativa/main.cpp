#include "astar.h"
#include "dijkstra.h"
#include <iostream>
#include <random>

Grid generateGrid(int H, int W, double obstacleRate) {
    Grid grid(H, std::vector<int>(W, 0));
    std::mt19937 rng(42);
    std::uniform_real_distribution<> dist(0.0, 1.0);
    for (int i = 0; i < H; ++i)
        for (int j = 0; j < W; ++j)
            if (dist(rng) < obstacleRate) grid[i][j] = 1;
    return grid;
}

int main() {
    std::vector<int> sizes = {500, 1000}; 
    for (int size : sizes) {
        Grid grid = generateGrid(size, size, 0.1);
        Point start = {0, 0};
        Point goal = {size - 1, size - 1};
        grid[start.first][start.second] = 0;
        grid[goal.first][goal.second] = 0;

        std::cout << "\n=== Tamaño Grid: " << size << "x" << size << " ===\n";

        auto dres = runDijkstra(grid, start, goal);
        std::cout << "DIJKSTRA -> Tiempo: " << dres.time 
                  << " s, Expansiones: " << dres.expansions
                  << ", Ruta: " << dres.path.size() << " nodos\n";

        auto ares = runAStar(grid, start, goal);
        std::cout << "ASTAR    -> Tiempo: " << ares.time 
                  << " s, Expansiones: " << ares.expansions
                  << ", Ruta: " << ares.path.size() << " nodos\n";
    }
    return 0;
}
