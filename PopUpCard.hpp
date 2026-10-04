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

    void actuateBaseCard(float theta); // mechanism 0 
    void actuateMechanism(int i); // mechanism 1 and above, only actuates a single mechanism. 
    void actuateWholeCard(float theta); 

    void printCurrentVertices();
    void printCurrentPoints();
    std::vector<Eigen::Vector3f> returnCurrentVertices();
    std::vector<Mechanism>& getMechanisms();

};





#endif