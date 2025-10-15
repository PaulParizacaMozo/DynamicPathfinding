#ifndef BMSSP_H
#define BMSSP_H

#include <vector>
#include <utility>

using Grid  = std::vector<std::vector<int>>;
using Point = std::pair<int,int>;

struct BMSSPResultado {
    std::vector<Point> ruta; 
    int expansiones;
    double tiempo;
    double memoriaMB;
};

BMSSPResultado runBMSSP(const Grid& grilla,
                        const std::vector<Point>& fuentes,
                        Point objetivo,
                        double limiteB);

#endif
