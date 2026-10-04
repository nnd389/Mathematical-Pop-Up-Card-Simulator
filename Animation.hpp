#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <vector>
#include <Eigen/Dense>
#include <string>
#include "Mechanism.hpp"
#include "PopUpCard.hpp"

struct Frame {
    float openingAngleTheta;
    std::vector<Eigen::Vector3f> graphPoints;
};

// How to slice a flat vertex list back into per-mechanism shapes,
// and each mechanism's creases (local vertex indices, same as Mechanism's own).
struct MechanismTopology {
    int vertexCount;
    std::vector<Crease> creases;
};

class Animation {
    private:
        std::vector<Frame> animationFrames;
        PopUpCard* card;
        std::vector<MechanismTopology> topology;

        void buildTopology();

    public:
    Animation(const std::vector<Frame>& _animationFrames) : animationFrames(_animationFrames), card(nullptr) {};
    Animation(PopUpCard& _card) : card(&_card) {};

    void addFrame(const Frame& frame);
    void animate(float thetaMin = 0.0f, float thetaMax = 2.8f, int windowWidth = 1000, int windowHeight = 800);
};

#endif