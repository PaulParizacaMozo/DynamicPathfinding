#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <vector>
#include <utility>

using Grid = std::vector<std::vector<int>>;
using Point = std::pair<int, int>;

struct DijkstraResult {
    std::vector<Point> path;
    int expansions;
    double time;
    double memoryMB;
};

DijkstraResult runDijkstra(const Grid &grid, Point start, Point goal);

#endif
