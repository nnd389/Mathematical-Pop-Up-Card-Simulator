#include "Mechanism.hpp"
#include "MathHelpers.hpp"
#include <iostream>
#include <stdexcept>

Point& Mechanism::getPoint(int i){
    return points[i];
}

std::vector<GlueDot>& Mechanism::getGlueDots(){
    return glueDots;
};

void Mechanism::printFlatPattern(){
    for (int i=0; i<flatPos.size(); i++){
        std::cout<< "Vertex " << i << " is: (" <<
        flatPos[i][0] << ", " <<
        flatPos[i][1] << ", " <<
        flatPos[i][2] << ")\n";
    }
};

Eigen::Vector3f Mechanism::getFlatVertex(int i) const{
    return flatPos[i];
}; 

Eigen::Vector3f Mechanism::getCurrentVertex(int i) const{
    return currentPos[i];
}; 

void Mechanism::setCurrentVertex(int i, const Eigen::Vector3f& position){
    currentPos[i] = position;
}

const CoordFrame& Mechanism::getFlatFrame() const{
    return flatFrame;
};

const CoordFrame& Mechanism::getCurrentFrame() const{
    return currentFrame;
};

void Mechanism::updateCurrentFrame(){
    currentFrame = calculateFrameAndOrigin();
}





// FIX LATER: I will have a problem with mechanisms that have cutouts around the crease-- 
// the origin might not even be on the mechanism, it might be on an extension of the mechanism
// the origin should be defined as the intersection of the crease line (in current state) and the crease of the mechanism it is glued to (in current state) (I think)
// I'll also run into problems where the mechanism crease doesn't intersect the crease of the mechanism below
std::vector<Point> Mechanism::identifyPointTypes(){
    std::vector<Point> identifiedPoints(flatPos.size());
    int sphereIndex;

    //Initialize general points for all
    for (int i=0; i<flatPos.size(); i++){
        identifiedPoints[i].type = PointType::general; 
        identifiedPoints[i].weights = Eigen::Vector3f::Zero();
    }

    // Identify sphere point
    int CreaseIndexI = creases[0].i;
    int CreaseIndexJ = creases[0].j;
    Eigen::Vector3f vertA = flatPos[CreaseIndexI];
    Eigen::Vector3f vertB = flatPos[CreaseIndexJ];

    if (vertA.y() <= vertB.y()) {
        sphereIndex = CreaseIndexJ;
    } else {
        sphereIndex = CreaseIndexI;
    }
    identifiedPoints[sphereIndex].type = PointType::spherePoints;

    // Identify glue points
    for (const GlueDot& glue : glueDots) {
        identifiedPoints[glue.i].type = PointType::gluePoints;
    }

    // throw an error if I accidentally ovverride the spherepoint or gluepoint

    return identifiedPoints;
};

std::vector<Point> Mechanism::calculateWeights(){
    // solve p - p0 = w_u*u + w_v*v + w_w*w
    // the coordinate frame is not intended to span R3, it is intended to span the mechanism in its current state
    // If a vertex lies to the right of the crease, it has weights for v and w, and the u weight equals zero
    // if the vertex lies to the left of the crease, weights u and v should be used, and weight for w = 0

    // Weights for sphere points will be special!
    // sphere: (0, w_v, 0)
    //FIX: we might want to go and fix the special points weights to make sure they are exact


    std::vector<Point> calculatedPoints = points;

    //Find special points
    int sphereIndex = -1;

    for (int i=0; i<points.size(); i++){
        if (points[i].type == PointType::spherePoints){
            sphereIndex = i;
            break;
        }
    }

    // Points
    Eigen::Vector3f origin = flatFrame.origin; // coordinate
    Eigen::Vector3f sphere = flatPos[sphereIndex]; // coordinate
    Eigen::Vector3f crease = flatFrame.v; // vector

    for (int i=0; i<flatPos.size(); i++){
        Eigen::Vector3f p_i = flatPos[i] - origin;
        calculatedPoints[i].weights = Eigen::Vector3f::Zero(); 

        // find if the point on the right or left of the crease
        // this is what chat suggested CHECK
        float crossZ = crease.x() * p_i.y() - crease.y() * p_i.x();

        if (crossZ > 0){ // Left side; only use u and v weights
            Eigen::Matrix2f M;
            M << flatFrame.u.x(), flatFrame.v.x(), 
                 flatFrame.u.y(), flatFrame.v.y();
            
            Eigen::Vector2f b(p_i.x(), p_i.y());
            Eigen::Vector2f weights = M.colPivHouseholderQr().solve(b);

            calculatedPoints[i].weights.x() = weights.x(); // w_u
            calculatedPoints[i].weights.y() = weights.y(); // w_v
            calculatedPoints[i].weights.z() = 0.0f;        // w_w
        }
        else { // Right side; only use v and w weights
            Eigen::Matrix2f M;
            M << flatFrame.v.x(), flatFrame.w.x(), 
                 flatFrame.v.y(), flatFrame.w.y();

            Eigen::Vector2f b(p_i.x(), p_i.y());
            Eigen::Vector2f weights = M.colPivHouseholderQr().solve(b);

            calculatedPoints[i].weights.x() = 0.0f;        // w_u
            calculatedPoints[i].weights.y() = weights.x(); // w_v
            calculatedPoints[i].weights.z() = weights.y(); // w_w
        }
    }

    return calculatedPoints;
};



CoordFrame Mechanism::calculateFrameAndOrigin(){ // points and vertices are a one-to-one mapping, where the vertex index corresponds to that point index
    // Initialize and find indices for bottom, bottomLeft, bottomRight, and sphere points
    CoordFrame localFrame;
    int bottomIndex = -1; // bottom of the crease
    int sphereIndex = -1; // top of the crease

    for (int i=0; i<points.size(); i++){
        if (points[i].type == PointType::spherePoints){
            sphereIndex = i;
            break;
        }
    }

    if (sphereIndex == creases[0].i){
        bottomIndex = creases[0].j;
    } else if (sphereIndex == creases[0].j){
        bottomIndex = creases[0].i;
    }

    int bottomLeftIndex = bottomIndex-1;
    int bottomRightIndex = bottomIndex+1;


    // Calculate the Origin
    // For now, let the origin be located at the bottom crease point
    // FIX: this will not always be true^, later the origin will need to be calculated differently
    localFrame.origin = currentPos[bottomIndex];

    // Calculate the frame
    Eigen::Vector3f u = (currentPos[bottomIndex]-currentPos[bottomLeftIndex]).normalized();
    Eigen::Vector3f v = (currentPos[bottomIndex]-currentPos[sphereIndex]).normalized();
    Eigen::Vector3f w = (currentPos[bottomIndex]-currentPos[bottomRightIndex]).normalized();
    
    localFrame.u = u;
    localFrame.v = v;
    localFrame.w = w;

    return localFrame;
};