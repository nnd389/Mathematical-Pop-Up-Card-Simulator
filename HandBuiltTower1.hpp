#ifndef HANDBUILTTOWER1_HPP
#define HANDBUILTTOWER1_HPP

// The original hand-written Tower Level 1 card, restored from main.cpp (kept for comparison with the file loader).
#include "Mechanism.hpp"
#include <vector>

inline std::vector<Mechanism> buildHandBuiltTower1() {
    std::vector<Eigen::Vector3f> baseCardPoints = {
        Eigen::Vector3f{139.5180, -153.6278, 0.0},  //0
        Eigen::Vector3f{88.0178, -153.6278, 0.0}, //1
        Eigen::Vector3f{88.0178, -50.6272, 0.0}, //2
        Eigen::Vector3f{139.5180, -50.6272, 0.0},  //3
        Eigen::Vector3f{191.0182, -50.6272, 0.0},  //4
        Eigen::Vector3f{191.0182, -153.6278, 0.0},   //5
        Eigen::Vector3f{100, -100, 0.0},  //random inner point
    };
    std::vector<Crease> CentralFold = { Crease {0, 3, CreaseType::Valley} };
    std::vector<GlueDot> emptyGlues;
    Mechanism baseCard(baseCardPoints, CentralFold, emptyGlues, true, 0);

    std::vector<Eigen::Vector3f> mech1Points = {
        Eigen::Vector3f{272.88648, -64.715832, 0.0}, Eigen::Vector3f{228.43008, -64.715832, 0.0},
        Eigen::Vector3f{228.43008, -20.259720, 0.0}, Eigen::Vector3f{272.88648, -20.259720, 0.0},
        Eigen::Vector3f{317.34216, -20.259720, 0.0}, Eigen::Vector3f{317.34216, -64.715832, 0.0},
    };
    std::vector<Crease> mech1Crease = { Crease {0, 3, CreaseType::Mountain} };
    std::vector<GlueDot> mech1Glues = {
        GlueDot {1, 0, Eigen::Vector3f{95.73696, -57.671496, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {0, 0, Eigen::Vector3f{139.51800, -65.391264, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {5, 0, Eigen::Vector3f{183.2990, -57.6715, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
    };
    Mechanism mech1(mech1Points, mech1Crease, mech1Glues, false, 1);

    std::vector<Eigen::Vector3f> mech2Points = {
        Eigen::Vector3f{272.88648, -153.62784, 0.0}, Eigen::Vector3f{228.43008, -153.62784, 0.0},
        Eigen::Vector3f{228.43008, -109.17216, 0.0}, Eigen::Vector3f{272.88648, -109.17216, 0.0},
        Eigen::Vector3f{317.34216, -109.17216, 0.0}, Eigen::Vector3f{317.34216, -153.62784, 0.0},
    };
    std::vector<Crease> mech2Crease = { Crease {0, 3, CreaseType::Mountain} };
    std::vector<GlueDot> mech2Glues = {
        GlueDot {1, 0, Eigen::Vector3f{95.73696, -102.127680, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {0, 0, Eigen::Vector3f{139.51800, -109.847520, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {5, 0, Eigen::Vector3f{183.2990, -102.1277, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
    };
    Mechanism mech2(mech2Points, mech2Crease, mech2Glues, false, 2);

    // Mechanism 3: a floating plane (all glue points, no free sphere point)
    std::vector<Eigen::Vector3f> mech3Points = {
        Eigen::Vector3f{272.88648, -242.53992, 0.0}, Eigen::Vector3f{229.10544, -234.82080, 0.0},
        Eigen::Vector3f{229.10544, -190.36440, 0.0}, Eigen::Vector3f{272.88648, -198.08424, 0.0},
        Eigen::Vector3f{316.66680, -190.36440, 0.0}, Eigen::Vector3f{316.66680, -234.82080, 0.0},
    };
    std::vector<Crease> mech3Crease = { Crease {0, 3, CreaseType::Valley} };
    std::vector<GlueDot> mech3Glues = {
        GlueDot {2, 1, Eigen::Vector3f{228.43008, -20.259720, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {3, 1, Eigen::Vector3f{272.88648, -20.259720, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {4, 1, Eigen::Vector3f{317.34216, -20.259720, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {1, 2, Eigen::Vector3f{228.43008, -109.17216, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {0, 2, Eigen::Vector3f{272.88648, -109.17216, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {5, 2, Eigen::Vector3f{317.34216, -109.17216, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
    };
    Mechanism mech3(mech3Points, mech3Crease, mech3Glues, false, 3);

    return {baseCard, mech1, mech2, mech3};
}

#endif
