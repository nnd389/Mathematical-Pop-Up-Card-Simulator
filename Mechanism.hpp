#ifndef MECHANISM_HPP
#define MECHANISM_HPP

#include <vector>
#include <Eigen/Dense>
#include <string>

//Lets focus on modeling the SYMMETRIC v-fold only.
//Assume every v-fold is a simple triangle with points A,D, and Cw
// and assume the user will define each v-fold and identify the points for you.
// Later I want to write a funciton that identifies points A,D, and Cw given the polygonal shape and the central fold.

class Mechanism; // can't have a pointer before the thing exists, so we create this forward declaration!

enum class PointType{
    rodriguesPoints, // I think all rodgrigues points will also be glue points
    gluePoints, 
    spherePoints, 
    Origin, 
    BottomLeft, 
    BottomRight, 
    General
};

struct Point { // a point is like a vertex with extra information, it has coordinates and it has an identity A,D, or Cw. point positions are defined by the applying the x,y weights to the unit vectors of the pointed to mechanisms
    Eigen::Vector3f weights; // these should always be calculated for you
    PointType type; // this should be identified for you
    //FIX: int id; // what vertix this is - this is redundant cuz we can assume that point index and vertex index are the same, but I think this will make it easier to track
    //Mechanism* pointMech; // what mechanism this points to
};

struct GlueDot {
    int i; // index of the vertex on this mechanism that you want to glue
    Eigen::Vector3f gluePosition; // the x,y,z position of where you want to glue on the flat pattern
    Mechanism* glueMech; // what mechanism it glues to 
};

enum class CreaseType {
    Mountain, 
    Valley
};

struct Crease { // a crease is a mountain or valley fold that connects vertices i and j. The crease should always connect form the origin (also a glue point?) to a sphere point. 
    int i;
    int j;
    CreaseType type;
};

struct CoordFrame { // a coordinate frame is a set of three unit vectors which can be used to access any coordinate on the mechanism (does not need to span R3)
    Eigen::Vector3f u; // I think the unit vectors can be implicitely caclucated from the points
    Eigen::Vector3f v;
    Eigen::Vector3f w;
};

class Mechanism{
    private:
        std::vector<Eigen::Vector3f> flatPos; // given by user, never changes
        std::vector<Crease> creases; // given by user, never changes
        std::vector<GlueDot> glueDots; //given by user, never changes
        std::vector<int> rodriguesPoints; // only here to identify the base card, given by user, never changes, but for all mechanisms except the base card this will be empty
        int id; // what mechanism is this, given by user, never changes

        std::vector<Eigen::Vector3f> currentPos; // calculated here, will change over time
        std::vector<Point> points; // calculated here, never changes
        CoordFrame frame; // calculated here, changes over time

    public:
    //Constructor
    Mechanism(const std::vector<Eigen::Vector3f>& _flatPos,
              const std::vector<Crease>& _creases,
              const std::vector<GlueDot>& _glueDots, 
              const std::vector<int>& _rodriguesPoints,
              int _id) 
              : flatPos(_flatPos), // the stuff after the : is the initializer list
              creases(_creases), 
              glueDots(_glueDots),
              rodriguesPoints(_rodriguesPoints), 
              id(_id) {
                // put calculated things here
                currentPos = flatPos; // for now, but currentPos will change over time
                points = identifyPointTypes();
                frame = calculateLocalFrame();
                points = calculateWeights();
              };



    //Selectors
    Point& getPoint(int i);
    Eigen::Vector3f getCurrentVertex(int i);
    Eigen::Vector3f getFrameVector(std::string vec);
    void printVertices();

    //void UpdateVertex(int i, Eigen::Vector3f& position);
    //void UpdateFrameVector(std::string vec);

    //Methods
    std::vector<Point> identifyPointTypes(); // only happens once
    CoordFrame calculateLocalFrame(); // the local frame will update every step
    std::vector<Point> calculateWeights(); // only happens once


    Mechanism actuateBaseCard(float theta);
    // have a function Simulate? this function will augment the vertex coordinates accordingly

};



// mech 0
// First I update meachanism 0 by updating the rodrigues points. 
// Then I update the u,v,w vectors for mechanism 0. 

// mech 1
// Then, I update Mech 1's glue points first using their respective weights on mech 0's u,v,w vectors. 
// Then I update mechanism 1's sphere points using the radii from the pattern and the updated glue point positions. 
// with the glue points and sphere point updated, I can update the u,v,w vectors. 
// Then, I updated the general/boundary points. these boundary points for mech 1 should be given by their weights on mech 1 and u,v,w for mech 1. 

// rodriguez points -> glue points -> sphere points -> general points
// My theory: All creases on v-fold mechanisms and parrelel fold mechanisms will intersect the central fold of the mechanism they are attatched to. 
// v-fold mechanisms meet on the card (or extension of the card) and parrallel folds meet at infinity. 


// Definition: A V-fold mechanism is a mechanism with exactly 2 patches and 1 crease such that
// the crease line the crease line intersects the gulley which actuates the mechanism. 
// the crease and the gulley can intersect, once, infinitely (overlapping), or at infinite (parrallel)

// Lemma: a V-fold is flat-folding if... (Duncan Birmingham knows why! and so does mathematics! just have to find it...)

#endif