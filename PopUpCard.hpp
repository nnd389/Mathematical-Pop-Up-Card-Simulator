#ifndef POPUPCARD_HPP
#define POPUPCARD_HPP

#include "Mechanism.hpp"
#include <vector>
#include <Eigen/Dense>
#include <string>



class PopUpCard {
    private:
        std::vector<Mechanism> mechanisms;

    public:
    PopUpCard(const std::vector<Mechanism>& _mechanisms) 
              : mechanisms(_mechanisms) {};

    void addMechanism(const Mechanism& mechanism);
    void calculateGlueWeights(); // only happens once

    void actuateBaseCard(float theta); // mechanism 0 // this should be a private method no?
    void actuateMechanism(int i); // mechanism 1 and above, only acuates one of them. the crease tells you to pop up or down
    void actuateWholeCard(float theta); // CONTINUE HERE!!

    void printCurrentVertices();
    void printCurrentPoints();

};





#endif