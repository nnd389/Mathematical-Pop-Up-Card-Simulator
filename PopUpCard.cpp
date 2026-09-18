#include "PopUpCard.hpp"
#include "MathHelpers.hpp"

void PopUpCard::addMechanism(const Mechanism& mechanism){
    mechanisms.push_back(mechanism);
}


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
