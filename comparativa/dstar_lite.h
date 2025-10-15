#ifndef DSTAR_LITE_H
#define DSTAR_LITE_H

#include <vector>
#include <utility>

using Grid = std::vector<std::vector<int>>;
using Point = std::pair<int,int>;

struct DStarLiteResult {
    std::vector<Point> ruta;
    int expansiones;
    double tiempo;
    double memoriaMB;
};

DStarLiteResult runDStarLite(const Grid& grilla, Point inicio, Point objetivo);

#endif
