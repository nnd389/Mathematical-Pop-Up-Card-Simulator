#include "Mechanism.hpp"
#include "PopUpCard.hpp"
#include "Animation.hpp"
#include <iostream>
#include <cmath>

// it's hard to capture all the pattern cases, so this code builds on some essential assumptions to create a working card. 
// we assume the following about the inputted flat pattern:
// the glue dots are listed in order left to right (smallest x to largest x)
// the sphere point is the point on the crease with the higher y value, impying that all mechanisms have glue tabs on their lower edges. (no upside down mechanisms in the pattern)
// no flipping your mechanism to glue it?



//CONTINUE HERE: I changed a bunch of things so time to debug. 
// mainly, I added glue tabs which helps find the origin! need to find any time I'm mannually finding bl b br and update it cuz it should come from glutabs
// FIX: find any time I'm incorrectly finding the origin
// FIX: instead of specifying where to put glue dots, can change it so that you specify where to position the origin and the glue tab angle. 
// hrmmm might end up with some problems...
// how do we deal with mechanisms attatched to 2 different mechanisms?




int main(){
    // Time to make a card!! first lets make the base, card, then make some mechanisms, then put it together in a card!
    // a mechanism has flatpos, creases, gluedots, baseCard (bool), and id

    // Creating the base card
    //   2      3     4
    //    -------------
    //   |      |      |
    //   |      |      |
    //   |      |      |
    //   |      |      | 
    //    -------------
    //   1      0      5
    //        origin      (for now!!)
    //          cf        (cf = central fold)
    std::vector<Eigen::Vector3f> baseCardPoints = {
        Eigen::Vector3f{0.0, 0.0, 0.0},  //0
        Eigen::Vector3f{-1.0, 0.0, 0.0}, //1
        Eigen::Vector3f{-1.0, 1.0, 0.0}, //2
        Eigen::Vector3f{0.0, 1.0, 0.0},  //3
        Eigen::Vector3f{1.0, 1.0, 0.0},  //4
        Eigen::Vector3f{1.0, 0.0, 0.0}   //5
    };

    std::vector<Crease> CentralFold = {
        Crease {0, 3, CreaseType::Valley}
    };

    std::vector<GlueDot> emptyGlues;

    Mechanism baseCard(baseCardPoints, CentralFold, emptyGlues, true, 0);

    // Creating mechanism 1, a simple right angle v-fold

    //      s             2
    //     /|\           /|\
    //    / | \         / | \
    //   /__|__\       /__|__\
    //   g  g  g      1   0   3
    //  bl  b  br    (bl = bottomLeft, b = bottom, br = bottomRight)
    //      o        (o = origin)           
    //      ^ crease line

    // std::vector<Eigen::Vector3f> mech1Points = {
    //     Eigen::Vector3f{5.0, 0.0, 0.0},  //0
    //     Eigen::Vector3f{4.5, 0.0, 0.0}, //1
    //     Eigen::Vector3f{5.0, 1.0, 0.0}, //2
    //     Eigen::Vector3f{5.5, 0.0, 0.0},  //3
    // };

    // std::vector<Crease> mech1Crease = {
    //     // Crease {i, j, type}
    //     Crease {0, 2, CreaseType::Mountain}
    // };

    // // dont forget to call calculate glue weights
    // float d = std::sqrt(0.125);
    // std::vector<GlueDot> mech1Glues = { // this part is tricky because it's up to the artists to make sure that lengths stay the same 
    //     // GlueDot {i, n, gluePosition, weights}
    //     GlueDot {1, 0, Eigen::Vector3f{-d, d, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
    //     GlueDot {0, 0, Eigen::Vector3f{0.0, 0.0, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
    //     GlueDot {3, 0, Eigen::Vector3f{d, d, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
    // };

    // Mechanism mech1(mech1Points, mech1Crease, mech1Glues, false, 1);


    //    __s__        2__3__4
    //   |  |  |       |  |  |
    //   |  |  |       |  |  |
    //   |__|__|       |__|__|
    //   g  g  g      1   0   5
    //  bl  b  br    (bl = bottomLeft, b = bottom, br = bottomRight)
    //      o        (o = origin)           
    //      ^ crease line


    std::vector<Eigen::Vector3f> mech1Points = {
        Eigen::Vector3f{5.0, 0.0, 0.0},  //0
        Eigen::Vector3f{4.5, 0.0, 0.0},  //1
        Eigen::Vector3f{4.5, 1.0, 0.0},  //2
        Eigen::Vector3f{5.0, 1.0, 0.0},  //3
        Eigen::Vector3f{5.5, 1.0, 0.0},  //4
        Eigen::Vector3f{5.5, 0.0, 0.0},  //5
    };

    std::vector<Crease> mech1Crease = {
        // Crease {i, j, type}
        Crease {0, 3, CreaseType::Mountain}
    };

    // dont forget to call calculate glue weights
    float d = std::sqrt(0.125);
    std::vector<GlueDot> mech1Glues = { // this part is tricky because it's up to the artists to make sure that lengths stay the same 
        // GlueDot {i, n, gluePosition, weights}
        GlueDot {1, 0, Eigen::Vector3f{-d, d, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {0, 0, Eigen::Vector3f{0.0, 0.0, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
        GlueDot {5, 0, Eigen::Vector3f{d, d, 0.0}, Eigen::Vector3f{0.0,0.0,0.0}},
    };

    Mechanism mech1(mech1Points, mech1Crease, mech1Glues, false, 1);

    
    // Creating the pop up card!!
    //        ______________________
    //       /       .   /         /
    //      /       /|\ /         /
    //     /       / | \         /
    //    /        \_!_/        /
    //   /__________/__________/

    std::vector<Mechanism> allMechs = {baseCard, mech1};
    constexpr float PI = 3.14159265358979323846f;
    PopUpCard myPopUpCard(allMechs);
    myPopUpCard.calculateGlueWeights();

    std::cout << "Current vertices: ";
    myPopUpCard.printCurrentVertices();
    myPopUpCard.printCurrentPoints();


    std::cout << "\n\nACTUATING BY 5 DEGREES"; // actually radians
    myPopUpCard.actuateWholeCard(0.0);
    std::cout << "\nNEW Current vertices: ";
    myPopUpCard.printCurrentVertices();
    myPopUpCard.printCurrentPoints();

    // compile command: g++ -std=c++17 -I/usr/local/include/eigen3 main.cpp PopUpCard.cpp Mechanism.cpp Animation.cpp -o popUp
    // run command: ./popup
    // python visualize command: ./popUp | python3 visualize_card.py --all

    // g++ -std=c++17 -I/usr/local/include/eigen3 -I$(brew --prefix raylib)/include \
    // main.cpp PopUpCard.cpp Mechanism.cpp Animation.cpp \
    // -L$(brew --prefix raylib)/lib -lraylib \
    // -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo \
    // -o popUp

    //c++ render command: g++ -std=c++17 -I/usr/local/include/eigen3 -I$(brew --prefix raylib)/include main.cpp PopUpCard.cpp Mechanism.cpp Animation.cpp -L$(brew --prefix raylib)/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o popUp

    // Animate the card!
    Animation myCardAnimation(myPopUpCard);
    myCardAnimation.animate();


    return 0;
}