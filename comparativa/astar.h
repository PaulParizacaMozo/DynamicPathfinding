#ifndef ASTAR_H
#define ASTAR_H

#include <vector>
#include <utility>

using Grid = std::vector<std::vector<int>>;
using Point = std::pair<int, int>;

struct AStarResult {
    std::vector<Point> path;
    int expansions;
    double time;
    double memoryMB;
};

AStarResult runAStar(const Grid &grid, Point start, Point goal);

#endif
