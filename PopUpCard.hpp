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

    void addMechanism(const Mechanism& mechanism);
    void calculateGlueWeights(); // only happens once

    void actuateBaseCard(float theta);

};


#endif