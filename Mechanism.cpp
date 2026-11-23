#include "Mechanism.hpp"
#include "MathHelpers.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

int Mechanism::getID(){
    return id;
};

Point& Mechanism::getPoint(int i){
    return points[i];
};

std::vector<Point> Mechanism::getPoints(){
    return points; 
};

std::vector<Crease> Mechanism::getCreases() {
    return creases;
};

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

void Mechanism::printCurrentPos(){
    for (int i=0; i<currentPos.size(); i++){
        std::cout<< "\nVertex " << i << " is: (" <<
        currentPos[i][0] << ", " <<
        currentPos[i][1] << ", " <<
        currentPos[i][2] << ")";
    }
};

void Mechanism::printPoints(){
    for (int i=0; i<points.size(); i++){
        std::cout<< "\nPoint " << i << " with id " << points[i].i << " is type" << pointTypeToString(points[i].type)
        << " and has weights " << points[i].weights.x() << ", " << points[i].weights.y() << ", " << points[i].weights.z();
    }
};

void Mechanism::printCurrentFrame(){
    std::cout<< "\nFrame origin is: (" <<
    currentFrame.origin.x() << ", " <<
    currentFrame.origin.y() << ", " <<
    currentFrame.origin.z() << ")";

    std::cout<< "\nFrame u is: (" <<
    currentFrame.u.x() << ", " <<
    currentFrame.u.y() << ", " <<
    currentFrame.u.z() << ")";

    std::cout<< "\nFrame v is: (" <<
    currentFrame.v.x() << ", " <<
    currentFrame.v.y() << ", " <<
    currentFrame.v.z() << ")";

    std::cout<< "\nFrame w is: (" <<
    currentFrame.w.x() << ", " <<
    currentFrame.w.y() << ", " <<
    currentFrame.w.z() << ")";
};

std::string Mechanism::pointTypeToString(PointType type) {
    switch (type) {
        case PointType::gluePoints:
            return "gluePoints";

        case PointType::spherePoints:
            return "spherePoints";

        case PointType::general:
            return "general";

        default:
            return "unknown";
    }
}


Eigen::Vector3f Mechanism::getFlatVertex(int i) const{
    return flatPos[i];
}; 

Eigen::Vector3f Mechanism::getCurrentVertex(int i) const{
    return currentPos[i];
}; 

std::vector<Eigen::Vector3f> Mechanism::getCurrentPos() const{
    return currentPos;
};

void Mechanism::setCurrentVertex(int i, const Eigen::Vector3f& position){
    currentPos[i] = position;
};

const CoordFrame& Mechanism::getFlatFrame() const{
    return flatFrame;
};

const CoordFrame& Mechanism::getCurrentFrame() const{
    return currentFrame;
};

const GlueTabs& Mechanism::getGlueTabs() const{
    return glueTabs;
};

void Mechanism::updateCurrentFrame(){
    currentFrame = calculateFrameAndOrigin();
}





// FIX LATER: I will have a problem with mechanisms that have cutouts around the crease-- 
// the origin might not even be on the mechanism, it might be on an extension of the mechanism
// the origin should be defined as the intersection of the crease line (in current state) and the crease of the mechanism it is glued to (in current state) (I think)
// I'll also run into problems where the mechanism crease doesn't intersect the crease of the mechanism below
std::vector<Point> Mechanism::identifyPointTypes(){
    std::vector<Point> identifiedPoints(flatPos.size()); // initialize 

    //Initialize general points for all
    for (int i=0; i<flatPos.size(); i++){
        identifiedPoints[i].i = i; 
        identifiedPoints[i].type = PointType::general; 
        identifiedPoints[i].weights = Eigen::Vector3f::Zero();
    }

    // Identify sphere point
    int sphereIndex = glueTabs.sphere; 
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

    // Points
    Eigen::Vector3f flatOrigin = flatFrame.origin; // coordinate
    Eigen::Vector3f crease = flatFrame.v; // vector

    for (int i=0; i<flatPos.size(); i++){
        Eigen::Vector3f p_i = flatPos[i] - flatOrigin;
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






GlueTabs Mechanism::findGlueTabs(){
    // glueDots.size == 3: (3 g's)
    //        s
    //       /|\
    //      / | \
    //     /  |  \
    //    /   |   \
    //   /____|____\
    //bll  bl/o/br  brr
    //  g     g     g


    // glueDots.size == 4: (4 g's)
    //        s1
    //       /|\
    //      / | \
    //     / s|2 \
    //    /  /.\  \
    //   /__/ . \__\
    //bll  bl o br  brr
    //  g  g     g  g
    GlueTabs myTabs;

    int bottomLeftLeftIndex = -1; 
    int bottomLeftIndex = -1; 
    int bottomRightRightIndex = -1;
    int bottomRightIndex = -1;
    int sphereIndex = -1; 

    int creaseIndexI = creases[0].i;
    int creaseIndexJ = creases[0].j;


    // in either case the sphere index is the "top" crease index, hard to define top FIX
    // by "top" I mean the point on the crease that is further away from the origin. FIX: (implement distance constraint way to find sphere point)
    // I'll just let the "top" be the one with the higher y value for now FIX: 
    Eigen::Vector3f vertA = flatPos[creaseIndexI];
    Eigen::Vector3f vertB = flatPos[creaseIndexJ];

    // default: the crease point with the higher y value
    if (vertA.y() <= vertB.y()) {
        sphereIndex = creaseIndexJ;
    } else {
        sphereIndex = creaseIndexI;
    }

    // if that point is a glue point it can't be the sphere point, so use the opposite crease point
    auto isGluePoint = [&](int vertIndex) {
        for (const GlueDot& dot : glueDots) {
            if (dot.i == vertIndex) return true;
        }
        return false;
    };

    if (isGluePoint(sphereIndex)) {
        int otherIndex = (sphereIndex == creaseIndexI) ? creaseIndexJ : creaseIndexI;
        // if both crease points are glued there is no valid sphere point, so keep the original choice
        if (!isGluePoint(otherIndex)) {
            sphereIndex = otherIndex;
        }
    }

    //        s1
    //       /|\
    //      / | \
    //     / s|2 \
    //    /  /.\  \
    //   /__/ . \__\
    //bll  bl o br  brr
    //  g  g     g  g

    //FIX: the sphere point isn't always the point witht he hgigher y value, its the point on the crease that is not a glue point (by this def can have multiple)
    //FIX: how do we deal with two potential pshere points? easy! just pick one! any one!
    // we only need one sphere point to recalc the fram and reconstruct the other sphere point. 
    // be careful, need to be consistent on what radii to use

    // go ahead and assume the glue dots are defined in order left to right for now 
    // FIX: how do we figure out "left" and "right" tabs? like left of the flat crease and right of the flat crease? 
    // FIX: this is such a bad way to find the origin?? there could be who knows how many glue dots! let the origin be the intersection with the crease and the spanning gulley! 
    
    if (glueDots.size() == 3){ // a regular v fold
        bottomLeftLeftIndex = glueDots[0].i;
        bottomLeftIndex = glueDots[1].i;
        bottomRightIndex = glueDots[1].i;
        bottomRightRightIndex = glueDots[2].i;
    } else if (glueDots.size() == 4) { // a handlebar v-fold // FIX: Parrallel folds also have 4 glue dots, need to handle parrallel fold mechanisms differently
        bottomLeftLeftIndex = glueDots[0].i;
        bottomLeftIndex = glueDots[1].i;
        bottomRightIndex = glueDots[2].i;
        bottomRightRightIndex = glueDots[3].i;
    } else { // no glue dots (base card), all glue dots (floating plane), or other amount of glue dots. origin is the non-sphere crease points
        if (sphereIndex == creaseIndexJ){
            bottomLeftIndex = creaseIndexI;
            bottomRightIndex = creaseIndexI;
        } else{ // if sphereIndex == creaseIndexI
            bottomLeftIndex = creaseIndexJ;
            bottomRightIndex = creaseIndexJ;
        }
        // bottomLeftIndex = bottomRightIndex here
        int n = outlineCount; // neighbours come from the outline only, not interior points
        // // the two polygon neighbors of the origin point are the left and right wing points. Which neighbor is which depends
        // // on the crease direction (origin -> sphere) and NOT on the index order, so decide by the side of the crease,
        // // using the same convention as calculateWeights (crossZ > 0 = left = u side)
        int nextIndex = (bottomLeftIndex + 1) % n;
        int prevIndex = (bottomLeftIndex - 1 + n) % n;
        Eigen::Vector3f creaseDir = flatPos[sphereIndex] - flatPos[bottomLeftIndex];
        Eigen::Vector3f toNext = flatPos[nextIndex] - flatPos[bottomLeftIndex];
        float crossZ = creaseDir.x() * toNext.y() - creaseDir.y() * toNext.x();
        if (crossZ > 0) {
            bottomLeftLeftIndex = nextIndex;
            bottomRightRightIndex = prevIndex;
        } else {
            bottomLeftLeftIndex = prevIndex;
            bottomRightRightIndex = nextIndex;
        }
        // u and w must point at GLUE points: a general point's position is rebuilt from the frame, so a frame built from
        // one is circular and never moves. If bll/brr is a general point, keep walking the outline away from the
        // origin until we hit a glue point that is still on the same side of the crease.
        if (!glueDots.empty()) {
            auto sideOfCrease = [&](int idx) {
                Eigen::Vector3f p = flatPos[idx] - flatPos[bottomLeftIndex];
                return creaseDir.x() * p.y() - creaseDir.y() * p.x();
            };
            auto findGlueOnSide = [&](int wing) {
                if (isGluePoint(wing)) return wing;
                int step = (wing == (bottomLeftIndex + 1) % n) ? 1 : -1; // direction leading away from the origin
                float side = sideOfCrease(wing);
                int idx = wing;
                for (int k = 0; k < n; k++) {
                    idx = ((idx + step) % n + n) % n;
                    if (idx == bottomLeftIndex) break; // wrapped all the way round
                    if (sideOfCrease(idx) * side <= 0) continue; // other side of (or on) the crease
                    if (isGluePoint(idx)) return idx;
                }
                return wing; // no glue point on that side; keep the original
            };
            bottomLeftLeftIndex = findGlueOnSide(bottomLeftLeftIndex);
            bottomRightRightIndex = findGlueOnSide(bottomRightRightIndex);
        }
        // bottomLeftLeftIndex = (bottomLeftIndex + 1) % n;
        // bottomRightRightIndex = (bottomLeftIndex - 1 + n) % n; // might need to switch back to this for tower 2?

        std::cout<< "\nCurrently on base card! ID is:  " << id;
        std::cout<< "\nBLL is: " << bottomLeftLeftIndex;
        std::cout<< "\nBL is: " << bottomLeftIndex;
        std::cout<< "\nBRR is: " << bottomRightRightIndex;
        std::cout<< "\nBR is: " << bottomRightIndex;
        std::cout<< "\nSphere Index is: " << sphereIndex;
        // sphereIndex = -1; the base card shouldn't have a sphere index? hrmmm but it might be used for the frame anyways
    }


    myTabs.bll = bottomLeftLeftIndex;
    myTabs.bl = bottomLeftIndex;
    myTabs.br = bottomRightIndex;
    myTabs.brr = bottomRightRightIndex;
    myTabs.sphere = sphereIndex;

    return myTabs;
};



CoordFrame Mechanism::calculateFrameAndOrigin(){ // points and vertices are a one-to-one mapping, where the vertex index corresponds to that point index
    // Initialize and find indices for bottom, bottomLeft, bottomRight, and sphere points
    CoordFrame localFrame;
    int bottomLeftLeftIndex = glueTabs.bll; 
    int bottomLeftIndex = glueTabs.bl; 
    int bottomRightIndex = glueTabs.br;
    int bottomRightRightIndex = glueTabs.brr;
    int sphereIndex = glueTabs.sphere; 

    Eigen::Vector3f bllPos = currentPos[bottomLeftLeftIndex];
    Eigen::Vector3f blPos = currentPos[bottomLeftIndex];
    Eigen::Vector3f brPos = currentPos[bottomRightIndex];
    Eigen::Vector3f brrPos = currentPos[bottomRightRightIndex];


    // Calculate the Origin
    // For now, let the origin be located at the bottom crease point
    // FIX: this will not always be true^, later the origin will need to be calculated differently
    Eigen::Vector3f localOrigin = lineIntersection(blPos, bllPos, brPos, brrPos); // CHECK: written by chat
    // FIX: these lines might be:
    // parrallel - that means this is a parrallel fold mechanism!
    // askew - possible, the glue dots are moving around a lot
    // overlapping - you glued on a line instead of a plane, therefore not a pop-uppable mechanism



    // Calculate the frame
    // Eigen::Vector3f u = (localOrigin-currentPos[bottomLeftLeftIndex]).normalized();
    // Eigen::Vector3f v = (localOrigin-currentPos[sphereIndex]).normalized();
    // Eigen::Vector3f w = (localOrigin-currentPos[bottomRightRightIndex]).normalized();
    Eigen::Vector3f u = (currentPos[bottomLeftLeftIndex] - localOrigin).normalized();
    Eigen::Vector3f v = (currentPos[sphereIndex] - localOrigin).normalized();
    Eigen::Vector3f w = (currentPos[bottomRightRightIndex] - localOrigin).normalized();
    
    localFrame.u = u;
    localFrame.v = v;
    localFrame.w = w;
    localFrame.origin = localOrigin;

    return localFrame;
};