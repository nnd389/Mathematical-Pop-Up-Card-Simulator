#include "Mechanism.hpp"
#include <iostream>
#include <cmath>

int main(){
    // Creating the base card
    std::vector<Point> baseCardPoints = {
        Point {Eigen::Vector3f{0.0, 0.0, 0.0}, PointType::Origin, nullptr}, //0
        Point {Eigen::Vector3f{-1.0, 0.0, 0.0}, PointType::Boundary, nullptr}, //1
        Point {Eigen::Vector3f{-1.0, 1.0, 0.0}, PointType::Boundary, nullptr}, //2
        Point {Eigen::Vector3f{0.0, 1.0, 0.0}, PointType::Boundary, nullptr}, //3
        Point {Eigen::Vector3f{1.0, 1.0, 0.0}, PointType::Boundary, nullptr}, //4
        Point {Eigen::Vector3f{1.0, 0.0, 0.0}, PointType::Boundary, nullptr} //5
    };

    std::vector<Crease> CentralFold = {
        Crease {0, 3, CreaseType::Valley}
    };

    // these should define the vectors running along the bottom edge and central fold
    // I think I should replace eigen:vector with a pointer to two points on the card A and B. u = (A-B)/||A-B||
    CoordFrame baseCardFrame = {
        {-1.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0}, 
    };

    Mechanism baseCard = {baseCardPoints, CentralFold, baseCardFrame, 0};


    constexpr float PI = 3.14159265358979323846f;
    Mechanism ActivatedCard = baseCard.ActuateBaseCard(PI/2.0);
    ActivatedCard.printVertices();

}