#include "PopUpCard.hpp"
#include "MathHelpers.hpp"
#include <iostream>

void PopUpCard::addMechanism(const Mechanism& mechanism){
    mechanisms.push_back(mechanism);
}

void PopUpCard::printCurrentVertices(){
    for (Mechanism& mech : mechanisms){
        int m = mech.getID();
        std::cout << "\n\nMechanism " << m << " vertices: ";
        mech.printCurrentPos();
    }
};

void PopUpCard::printCurrentPoints(){
    for (Mechanism& mech : mechanisms){
        int m = mech.getID();
        std::cout << "\n\nMechanism " << m << " points: ";
        mech.printPoints();
    }
};


void PopUpCard::calculateGlueWeights(){
    for (Mechanism& mechanism : mechanisms) {
        std::vector<GlueDot>& glueDots = mechanism.getGlueDots();

        for (GlueDot& glue : glueDots){
            int n = glue.n; // id for mechanism we are glueing to
            const Mechanism& target = mechanisms[n]; // mechanism we are glueing to
            const CoordFrame& targetFlatFrame = target.getFlatFrame(); // the flat frame for the target mechanism
            Eigen::Vector3f origin = targetFlatFrame.origin; // the origin of the target mechanism
            Eigen::Vector3f p = glue.gluePosition - origin; // position of the glue point position relative to the target frame's origin

            //Determine which side of the target mechanism's crease we are on
            float crossZ = targetFlatFrame.v.x() * p.y() - targetFlatFrame.v.y() * p.x();
            Eigen::Vector3f weights = Eigen::Vector3f::Zero();

            if (crossZ > 0){
                // left side: use u and v
                Eigen::Matrix2f M;

                M << targetFlatFrame.u.x(), targetFlatFrame.v.x(),
                     targetFlatFrame.u.y(), targetFlatFrame.v.y();

                Eigen::Vector2f b(p.x(), p.y());

                Eigen::Vector2f w =
                    M.colPivHouseholderQr().solve(b);

                weights.x() = w.x();
                weights.y() = w.y();
                weights.z() = 0.0f;
            }
            else {
                // Right side: v and w
                Eigen::Matrix2f M;

                M << targetFlatFrame.v.x(), targetFlatFrame.w.x(),
                     targetFlatFrame.v.y(), targetFlatFrame.w.y();

                Eigen::Vector2f b(p.x(), p.y());

                Eigen::Vector2f w =
                    M.colPivHouseholderQr().solve(b);

                weights.x() = 0.0f;
                weights.y() = w.x();
                weights.z() = w.y();
            }
            glue.glueWeights = weights;
        }
    }
};


// we assume the base card is always a valley fold
void PopUpCard::actuateBaseCard(float theta){ // all this function will do is rotate vertices 4 and 5 on mechanism 0
    // assume that points 4 and 5 are rodriguez points
    // rotate points 4 and 5 around the central crease and update the current position of the base card

    Eigen::Vector3f v4 = mechanisms[0].getFlatVertex(4); // let's assume we always calculate things from the flat position?
    Eigen::Vector3f v5 = mechanisms[0].getFlatVertex(5);
    Eigen::Vector3f k = mechanisms[0].getFlatFrame().v; // We only use this function for the base card, and v should NEVER change for the base card

    // Update our current position and current frame (update frame by updating the vectors and origin of this frame)
    mechanisms[0].setCurrentVertex(4, RodriguesRotation(v4, k, theta)); // I want to update the current position of this mechanism
    mechanisms[0].setCurrentVertex(5, RodriguesRotation(v5, k, theta));
    //CHECK: chat thinks theres a problem with this rodrigues rotation formula, that its rotating about a line passing thorugh (0,0,0)

    mechanisms[0].updateCurrentFrame();
};




// mech 0
// First I update meachanism 0 by updating the rodrigues points. 
// Then I update the frame for mechanism 0. 

// mech 1
// Then, I update Mech 1's glue points first using their respective weights on mech 0's u,v,w vectors. 
// Then I update mechanism 1's sphere points using the radii from the pattern and the updated glue point positions. 
// with the glue points and sphere point updated, I can update the u,v,w vectors. 
// Then, I updated the general/boundary points. these boundary points for mech 1 should be given by their weights on mech 1 and u,v,w for mech 1. 

// rodriguez points -> glue points -> sphere points -> frame -> general points



//FIX: my v-fold diagram was upside down the entire time, so there might be some problems in how we are identifying the sphere point vs the origin point
void PopUpCard::actuateMechanism(int i){
    // update glue points (for things to go smoothly the bottomleft, bottom, and bottomright should be a glue point!)
    //NOTE: Mechanism mechI = mechanisms[i]; // this returns a COPY not a REFERENCE! you need to put the & to make it a reference!
    Mechanism& mechI = mechanisms[i];
    std::vector<GlueDot>& glueDots = mechI.getGlueDots();
    // FIX: should maybe change this to for point in points for consistency
    for (GlueDot& glue : glueDots) {
        int glueVertIndex = glue.i;
        int ontoMechIndex = glue.n;

        // reconstruct the position of the glue vertex using the frame and weights derived from the onto mechanism. 
        CoordFrame ontoMechCurrentFrame = mechanisms[ontoMechIndex].getCurrentFrame();
        Eigen::Vector3f ontoOrigin = ontoMechCurrentFrame.origin;
        Eigen::Vector3f glueWeights = glue.glueWeights; // w_u, w_v, w_w

        Eigen::Vector3f newGluePos = ontoOrigin + glueWeights.x()*ontoMechCurrentFrame.u + glueWeights.y()*ontoMechCurrentFrame.v + glueWeights.z()*ontoMechCurrentFrame.w;

        mechI.setCurrentVertex(glueVertIndex, newGluePos);
    }



    // update the sphere points

    // first, find the sphere point, bottomLeft, bottom, and bottomRight indices
    // there should only be one sphere point and there should only be one crease
    std::vector<Point> points = mechI.getPoints();
    std::vector<Crease> creases = mechI.getCreases();
    int sphereVertIndex = -1;
    int bottomLeftIndex = -1;
    int bottomIndex = -1;
    int bottomRightIndex = -1;

    for (Point& point : points) {
        PointType type = point.type;

        if (type == PointType::spherePoints){
            sphereVertIndex = point.i;
            if (sphereVertIndex == creases[0].i){
                bottomIndex = creases[0].j;
            } else if (sphereVertIndex == creases[0].j){
                bottomIndex = creases[0].i;
            }
        }
    }
    // bottomLeftIndex = bottomIndex-1;
    // bottomRightIndex = bottomIndex+1;
    int n = static_cast<int>(points.size());
    bottomLeftIndex = (bottomIndex - 1 + n) % n;
    bottomRightIndex = (bottomIndex + 1) % n;

    // To calculate the sphere point's position, need three centers and three radii
    // FIX: the centers are given by the bottomleft, bottom, and bottomright vertices(?)
    // the radii are given by the centers and the ORIGIN, which currently the origin calculation needs fixing 
    Eigen::Vector3f sphereFlatPos = mechI.getFlatVertex(sphereVertIndex);

    Eigen::Vector3f bottomLeftFlatPos = mechI.getFlatVertex(bottomLeftIndex);
    Eigen::Vector3f bottomLeftCurrentPos = mechI.getCurrentVertex(bottomLeftIndex);

    //FIX:
    Eigen::Vector3f bottomFlatPos = mechI.getFlatVertex(bottomIndex); // same as origin FOR NOW (FIX ORIGIN)
    Eigen::Vector3f bottomCurrentPos = mechI.getCurrentVertex(bottomIndex); // same as origin FOR NOW (FIX ORIGIN)


    Eigen::Vector3f bottomRightFlatPos = mechI.getFlatVertex(bottomRightIndex);
    Eigen::Vector3f bottomRightCurrentPos = mechI.getCurrentVertex(bottomRightIndex);

    

    float r1 = (sphereFlatPos - bottomLeftFlatPos).norm();
    float r2 = (sphereFlatPos - bottomFlatPos).norm(); // FIX: right now the origin is the same as the bottomFlatPos
    float r3 = (sphereFlatPos - bottomRightFlatPos).norm();

    //CHECK: right now this works because I assume that the bottomleft, bottom, and bottomright vertices are also glue points
    //FIX: one of the centers should be the origin
    //Eigen::Vector3f newSpherePos = IntersectThreeSpheres(bottomLeftCurrentPos, r1, origin, r2, bottomRightCurrentPos, r3);

    // if crease type = mountain, flag = 1, if creatype = valley, flag = 0
    int flag = (creases[0].type == CreaseType::Mountain) ? 1 : 0;
    Eigen::Vector3f newSpherePos = IntersectThreeSpheres(bottomLeftCurrentPos, r1, bottomCurrentPos, r2, bottomRightCurrentPos, r3, flag);

    mechI.setCurrentVertex(sphereVertIndex, newSpherePos);
    mechI.updateCurrentFrame(); // update the frame and FIX: update the origin! 
    // now that I updated the sphere points, bl/b/br points (which are also glue points in my assumption), I can update the frame
    // FIX: should really hold the bl/b/br indices somewhere instead of deriving them every time


    // update any general/boundary points
    for (Point& point : points){
        if (point.type == PointType::general){
            // reconstruct the general point using the origin and frame weights from this mechanism
            // already have origin above
            int generalVertIndex = point.i;
            Eigen::Vector3f weights = point.weights;
            CoordFrame currentFrame = mechI.getCurrentFrame();
            Eigen::Vector3f origin = mechI.getCurrentFrame().origin;

            Eigen::Vector3f generalPointNewPos = origin + weights.x() * currentFrame.u + weights.y() * currentFrame.v + weights.z() * currentFrame.w;
            mechI.setCurrentVertex(generalVertIndex, generalPointNewPos);
        }
    }

    //Done! we updated the glue points (which are secretly also the bl/b/br points), the sphere point, the frame and origin, and the general points!
};



void PopUpCard::actuateWholeCard(float theta){
    actuateBaseCard(theta);
    
    // FIX: need to build a dependency tree, mechanism 0 build on nothing (base card), mechanism 1 builds on mechanism 0, mechanism 2 build on mechanisms 0 and 1. 
    for (int i=1; i<mechanisms.size(); i++){
        actuateMechanism(i);
    }
};

