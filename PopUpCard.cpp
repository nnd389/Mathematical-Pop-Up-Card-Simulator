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

std::vector<Eigen::Vector3f> PopUpCard::returnCurrentVertices() {
    std::vector<Eigen::Vector3f> allVertices;

    for (Mechanism& mech : mechanisms){
        std::vector<Eigen::Vector3f> mechVerts = mech.getCurrentPos();
        for (int i=0; i<mechVerts.size(); i++){
            allVertices.push_back(mech.getCurrentVertex(i));
        }
    }

    return allVertices;
};

std::vector<Mechanism>& PopUpCard::getMechanisms() {
    return mechanisms;
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
            int n = glue.n; // id for mechanism we are gluing to
            const Mechanism& target = mechanisms[n]; // mechanism we are gluing to
            const CoordFrame& targetFlatFrame = target.getFlatFrame(); // the flat frame for the target mechanism
            Eigen::Vector3f targetFlatOrigin = targetFlatFrame.origin; // the flat origin of the target mechanism
            Eigen::Vector3f p = glue.gluePosition - targetFlatOrigin; // position of the glue point position relative to the target frame's origin

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
                // Right side: use v and w
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
// also assume we always update from the flat position
void PopUpCard::actuateBaseCard(float theta){ // rotate points 4 and 5 around the central crease on Mechanism 0 and update the current position of the base card
    Eigen::Vector3f v4 = mechanisms[0].getFlatVertex(4); 
    Eigen::Vector3f v5 = mechanisms[0].getFlatVertex(5);
    Eigen::Vector3f k = mechanisms[0].getFlatFrame().v; // We only use this function for the base card, and v should NEVER change for the base card

    // Update our current position and current frame (update frame by updating the vectors and origin of this frame)
    mechanisms[0].setCurrentVertex(4, RodriguesRotation(v4, k, theta));
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



void PopUpCard::actuateMechanism(int i){
    // update glue points (for things to go smoothly the bottomleft, bottom, and bottomright should be a glue point!)
    Mechanism& mechI = mechanisms[i];
    std::vector<GlueDot>& glueDots = mechI.getGlueDots();
    GlueTabs glueTabs = mechI.getGlueTabs();
    std::vector<Crease> creases = mechI.getCreases();
    std::vector<Point> points = mechI.getPoints();

    for (GlueDot& glue : glueDots) {
        int glueVertIndex = glue.i;
        int ontoMechIndex = glue.n;

        // reconstruct the position of the glue vertex using the frame and weights derived from the onto mechanism. 
        CoordFrame ontoMechCurrentFrame = mechanisms[ontoMechIndex].getCurrentFrame();
        Eigen::Vector3f ontoCurrentOrigin = ontoMechCurrentFrame.origin;
        Eigen::Vector3f glueWeights = glue.glueWeights; // w_u, w_v, w_w
        Eigen::Vector3f newGluePos = ontoCurrentOrigin + glueWeights.x()*ontoMechCurrentFrame.u + glueWeights.y()*ontoMechCurrentFrame.v + glueWeights.z()*ontoMechCurrentFrame.w;

        mechI.setCurrentVertex(glueVertIndex, newGluePos);
    }

    // update current origin
    // Once I've updated the glue points I can update the current origin. I cannot update the current u v w frame until I have the sphere point
    mechI.updateCurrentFrame(); // FIX: I just need the current origin to be updated, don't want to mess with u v w necessarily
    // maybe write a separate function to just update the origin




    // update the sphere points

    int bottomLeftLeftIndex = glueTabs.bll; 
    // int bottomLeftIndex = glueTabs.bl;
    // int bottomRightIndex = glueTabs.br;
    int bottomRightRightIndex = glueTabs.brr;
    int sphereIndex = glueTabs.sphere;


    // To calculate the sphere point's position, need three centers and three radii
    // the centers are given by the bottomleftleft, origin, and bottomrightright CURRENT vertices
    // the radii are given by the centers in FLAT state and the FLAT ORIGIN
    Eigen::Vector3f sphereFlatPos = mechI.getFlatVertex(sphereIndex);
    Eigen::Vector3f originFlatPos = mechI.getFlatFrame().origin;

    Eigen::Vector3f bottomLeftLeftFlatPos = mechI.getFlatVertex(bottomLeftLeftIndex);
    Eigen::Vector3f bottomLeftLeftCurrentPos = mechI.getCurrentVertex(bottomLeftLeftIndex);

    Eigen::Vector3f bottomRightRightFlatPos = mechI.getFlatVertex(bottomRightRightIndex);
    Eigen::Vector3f bottomRightRightCurrentPos = mechI.getCurrentVertex(bottomRightRightIndex);

    float r1 = (sphereFlatPos - bottomLeftLeftFlatPos).norm();
    float r2 = (sphereFlatPos - originFlatPos).norm(); 
    float r3 = (sphereFlatPos - bottomRightRightFlatPos).norm();

    // if crease type = mountain, flag = 1, if crease type = valley, flag = 0
    int flag = (creases[0].type == CreaseType::Mountain) ? 1 : 0;
    Eigen::Vector3f currentOrigin = mechI.getCurrentFrame().origin;
    Eigen::Vector3f newSpherePos = IntersectThreeSpheres(bottomLeftLeftCurrentPos, r1, currentOrigin, r2, bottomRightRightCurrentPos, r3, flag);

    mechI.setCurrentVertex(sphereIndex, newSpherePos);
    mechI.updateCurrentFrame(); // update the frame and origin (origin was already updated but sure)
    // now that I updated the sphere points and glue points I can update the frame


    // update any general/boundary points
    for (Point& point : points){
        if (point.type == PointType::general){
            // reconstruct the general point using the origin and frame weights from this mechanism
            // already have origin above
            int generalVertIndex = point.i;
            Eigen::Vector3f weights = point.weights;
            CoordFrame currentFrame = mechI.getCurrentFrame();
            Eigen::Vector3f currentOrigin = mechI.getCurrentFrame().origin;

            Eigen::Vector3f generalPointNewPos = currentOrigin + weights.x() * currentFrame.u + weights.y() * currentFrame.v + weights.z() * currentFrame.w;
            mechI.setCurrentVertex(generalVertIndex, generalPointNewPos);
        }
    }

    //Done! we updated the glue points, the sphere point, the frame and origin, and the general points!
};



void PopUpCard::actuateWholeCard(float theta){
    actuateBaseCard(theta);
    
    // FIX: need to build a dependency tree, mechanism 0 build on nothing (base card), mechanism 1 builds on mechanism 0, mechanism 2 build on mechanisms 0 and 1. 
    for (int i=1; i<mechanisms.size(); i++){
        actuateMechanism(i);
    }
};

