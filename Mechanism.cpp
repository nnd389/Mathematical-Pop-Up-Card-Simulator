#include "Mechanism.hpp"
#include "MathHelpers.hpp"
#include <iostream>
#include <stdexcept>

Point& Mechanism::getPoint(int i){
    return points[i];
}

Eigen::Vector3f Mechanism::getCurrentVertex(int i){
    return currentPos[i];
};

Eigen::Vector3f Mechanism::getFrameVector(std::string vec){
    if (vec == "u"){
        return frame.u;
    }  
    else if (vec == "v"){
        return frame.v;
    } 
    else if (vec == "w"){
        return frame.w;
    } 
    else{
        throw std::invalid_argument("Invalid frame vector: " + vec);
    }
};

Mechanism Mechanism::actuateBaseCard(float theta){
    Mechanism activatedCard = *this;
    // assume that points 4 and 5 are rodriguez points
    // FIX: you should really identify rodriguezpoints using Points
    // ideally, the base card should have some special code written specifically for it

    // rotate points 4 and 5 around the central crease and return a copy of the base card. 
    // the original base card is kept clean for printing purposes. 
    Eigen::Vector3f v4 = activatedCard.flatPos[4];
    Eigen::Vector3f v5 = activatedCard.flatPos[5];

    int cfi = activatedCard.creases[0].i;
    int cfj = activatedCard.creases[0].j;
    Eigen::Vector3f k = (activatedCard.flatPos[cfi] - activatedCard.flatPos[cfj]).normalized();

    activatedCard.flatPos[4] =  RodriguesRotation(v4, k, theta);
    activatedCard.flatPos[5] =  RodriguesRotation(v5, k, theta);
    //FIX: you also have to rotate the unit vectors describing this card
    
    return activatedCard;
};

void Mechanism::printVertices(){
    for (int i=0; i<points.size(); i++){
        std::cout<< "Vertex " << i << " is: (" <<
        flatPos[i][0] << ", " <<
        flatPos[i][1] << ", " <<
        flatPos[i][2] << ")\n";
    }
};




// FIX LATER: I will have a problem with mechanisms that have cutouts around the crease-- 
// the origin might not even be on the mechanism, it might be on an extension of the mechanism
// the origin should be defined as the intersection of the animated crease line and the crease of the mahcnism it s glued to (I think)
// I'll also run into problems where the mechanism crease doesn't intersect the crease of the mechanism below
std::vector<Point> Mechanism::identifyPointTypes(){
    std::vector<Point> identifiedPoints(flatPos.size()); // initialize
    int originIndex;
    int sphereIndex;

    //Initialize general points for all
    for (int i=0; i<flatPos.size(); i++){
        identifiedPoints[i].type = PointType::General; // will be ovverrode with other types
        identifiedPoints[i].weights = Eigen::Vector3f::Zero(); // don't know the weights yet
    }

    // Identify the origin and sphere point
    int CreaseIndexI = creases[0].i;
    int CreaseIndexJ = creases[0].j;
    Eigen::Vector3f vertA = flatPos[CreaseIndexI];
    Eigen::Vector3f vertB = flatPos[CreaseIndexJ];

    if (vertA.y() <= vertB.y()) {
        originIndex = CreaseIndexI;
        sphereIndex = CreaseIndexJ;
    } else {
        originIndex = CreaseIndexJ;
        sphereIndex = CreaseIndexI;
    }
    identifiedPoints[originIndex].type = PointType::Origin;
    identifiedPoints[sphereIndex].type = PointType::spherePoints;

    // Identify bottom left and bottom right points 
    identifiedPoints[originIndex-1].type = PointType::BottomLeft;
    identifiedPoints[originIndex+1].type = PointType::BottomRight;

    // Identify glue points
    for (const GlueDot& glue : glueDots) {
        identifiedPoints[glue.i].type = PointType::gluePoints;
    }

    // Identify Rodriguez Points
    for (int index : rodriguesPoints) {
        identifiedPoints[index].type = PointType::rodriguesPoints;
    }

    // throw an error if I accidentally ovverride the origin, spherepoint, bottomleft, bottomright, or gluepoints

    return identifiedPoints;
};

std::vector<Point> Mechanism::calculateWeights(){
    // solve p - p0 = w_u*u + w_v*v + w_w*w
    // the coordinate frame is not intended to span R3, it is intended to span the mechanism in its current state
    // If a vertex lies to the right of the crease, it has weights for v and w, and the u weight equals zero
    // if the vertex lies to the left of the crease, weights u and v should be used, and weight for w = 0

    // Weights for special points will also be special!
    // origin: (0,0,0)
    // bottomLeft: (w_u, 0, 0)
    // sphere: (0, w_v, 0)
    // bottomRight: (0,0, w_w)


    //CONTINUEHERE!!
    // see chat
    std::vector<Point> calculatedPoints = points;

    //Find special points
    int originIndex = -1;
    int sphereIndex = -1;

    for (int i=0; i<points.size(); i++){
        if (points[i].type == PointType::Origin){
            originIndex = i;
        }
        else if (points[i].type == PointType::spherePoints){
            sphereIndex = i;
        }
    }

    // Points
    Eigen::Vector3f origin = flatPos[originIndex];
    Eigen::Vector3f sphere = flatPos[sphereIndex];
    Eigen::Vector3f crease = frame.v;
    // Direction of central crease is v 

    for (int i=0; i<flatPos.size(); i++){
        Eigen::Vector3f p_i = flatPos[i] - origin;
        calculatedPoints[i].weights = Eigen::Vector3f::Zero(); // initialize as zero
        if (i == originIndex){ // don't calculate weights for origin
            continue;
        }

        // find if the point on the right or left of the crease
        // this is what chat suggested CHECK
        float crossZ = crease.x() * p_i.y() - crease.y() * p_i.x();

        if (crossZ > 0){ // Left side; only use u and v weights
            Eigen::Matrix2f M;
            M << frame.u.x(), frame.v.x(), 
                 frame.u.y(), frame.v.y();
            
            Eigen::Vector2f b(p_i.x(), p_i.y());
            Eigen::Vector2f weights = M.colPivHouseholderQr().solve(b);

            calculatedPoints[i].weights.x() = weights.x(); // w_u
            calculatedPoints[i].weights.y() = weights.y(); // w_v
            calculatedPoints[i].weights.z() = 0.0f;        // w_w
        }
        else if (crossZ >=0 ){ // Right side; only use v and w weights
            Eigen::Matrix2f M;
            M << frame.v.x(), frame.w.x(), 
                 frame.v.y(), frame.w.y();

            Eigen::Vector2f b(p_i.x(), p_i.y());
            Eigen::Vector2f weights = M.colPivHouseholderQr().solve(b);

            calculatedPoints[i].weights.x() = 0.0f;        // w_u
            calculatedPoints[i].weights.y() = weights.x(); // w_v
            calculatedPoints[i].weights.z() = weights.y(); // w_w
        }
    }
    //FIX: we might want to go and fix the special points weights to make sure they are exact

    return calculatedPoints;
};

CoordFrame Mechanism::calculateLocalFrame(){
    // points and vertices are a one-to-one mapping, where the vertex index corresponds to that point index

    int originIndex = -1;
    int sphereIndex = -1;
    int bottomLeftIndex = -1;
    int bottomRightIndex = -1;

    // find the special points
    for (int i=0; i<points.size(); i++){
        if (points[i].type == PointType::Origin){
            originIndex = i;
        } 
        else if (points[i].type == PointType::spherePoints){
            sphereIndex = i;
        }
        else if (points[i].type == PointType::BottomLeft){
            bottomLeftIndex = i;
        }
        else if (points[i].type == PointType::BottomRight){
            bottomRightIndex = i;
        }
    }
    
    // Calculate the frame
    Eigen::Vector3f u = (currentPos[originIndex]-currentPos[bottomLeftIndex]).normalized();
    Eigen::Vector3f v = (currentPos[originIndex]-currentPos[sphereIndex]).normalized();
    Eigen::Vector3f w = (currentPos[originIndex]-currentPos[bottomRightIndex]).normalized();

    CoordFrame localFrame;
    localFrame.u = u;
    localFrame.v = v;
    localFrame.w = w;

    return localFrame;
};