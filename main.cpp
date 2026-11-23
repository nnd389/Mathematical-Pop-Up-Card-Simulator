#include "Mechanism.hpp"
#include "PopUpCard.hpp"
#include "Animation.hpp"
#include "FoldLoader.hpp"
#include <string>
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



//CONTINUE HERE:
// I had cluade write a paerse for me: foldLoader.hpp, it worked pretty well but also has room for problems!
// todo:
// fix how the origin is calculated, should be crease line intersection not glue tab intersection with spanning gulley
// fix patches, frame should be defined as connecting two patches, might have to add in a patch class
// there are problems with points that are inside a patch, try to fix those
int main(int argc, char** argv){
    // The card now comes from a flat-pattern file (see FoldLoader.hpp for the file format and conventions).
    // usage: ./popUp [pattern.txt] [scale]     (the old hand-built Tower Level 1 is in git history)
    std::string patternFile = (argc > 1) ? argv[1] : "popUpCard.txt";
    float scale = (argc > 2) ? std::stof(argv[2]) : 1.0f;

    std::vector<Mechanism> allMechs = loadCard(patternFile, scale);
    PopUpCard myPopUpCard(allMechs);
    myPopUpCard.calculateGlueWeights();






    float PI = 3.14159265359;
    std::cout << "\n\nACTUATING BY 0 DEGREES"; // actually radians
    myPopUpCard.actuateWholeCard(0.0);
    std::cout << "\nNEW Current vertices: ";
    myPopUpCard.printCurrentVertices();
    std::cout << "\n\n";

    std::cout << "\n\nACTUATING BY Pi/5 DEGREES"; // actually radians
    myPopUpCard.actuateWholeCard(-PI/5.0);
    std::cout << "\nNEW Current vertices: ";
    myPopUpCard.printCurrentVertices();
    std::cout << "\n\n";

    std::cout << "\n\nACTUATING BY 2Pi/5 DEGREES"; // actually radians
    myPopUpCard.actuateWholeCard(-2.0*PI/5.0);
    std::cout << "\nNEW Current vertices: ";
    myPopUpCard.printCurrentVertices();
    std::cout << "\n\n";

    std::cout << "\n\nACTUATING BY 3Pi/5 DEGREES"; // actually radians
    myPopUpCard.actuateWholeCard(-3.0*PI/5.0);
    std::cout << "\nNEW Current vertices: ";
    myPopUpCard.printCurrentVertices();
    std::cout << "\n\n";

    std::cout << "\n\nACTUATING BY 4Pi/5 DEGREES"; // actually radians
    myPopUpCard.actuateWholeCard(-4.0*PI/5.0);
    std::cout << "\nNEW Current vertices: ";
    myPopUpCard.printCurrentVertices();
    std::cout << "\n\n";

    std::cout << "\n\nACTUATING BY 5Pi/5 DEGREES"; // actually radians
    //myPopUpCard.actuateWholeCard(-4.999*PI/5.0);
    std::cout << "\nNEW Current vertices: ";
    myPopUpCard.printCurrentVertices();
    std::cout << "\n\n";

    // run command: ./popup
    // python visualize command: python3 visualize_flat.py TowerLevel2FlatPattern.txt
    // c++ render command: g++ -std=c++17 -I/usr/local/include/eigen3 -I$(brew --prefix nlohmann-json)/include -I$(brew --prefix raylib)/include main.cpp FoldLoader.cpp PopUpCard.cpp Mechanism.cpp Animation.cpp -L$(brew --prefix raylib)/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o popUp

    // Animate the card!
    Animation myCardAnimation(myPopUpCard);
    myCardAnimation.animate();


    return 0;
}