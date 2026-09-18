#ifndef MECHANISM_HPP
#define MECHANISM_HPP

#include <vector>
#include <Eigen/Dense>
#include <string>

//Lets focus on modeling the SYMMETRIC v-fold only.

//      s
//     /|\
//    / | \
//   /__|__\
//   g  g  g
//      ^ crease line

// Assume every v-fold is a simple triangle with 1 crease, three glue points, glue points are at the bottom edge
// Assume the flat pattern is given in a way such that the vertices are connected cyclicly 

enum class PointType{
    gluePoints, 
    spherePoints, 
    general
};

struct Point { // Every vertex maps to a point. A point has a type and it has weights for the localframe to tell you where it is relative to the true local origin
    Eigen::Vector3f weights;
    PointType type; 
};

struct GlueDot { // currently on mechanism m, gluing to mechanism n, where n<m
    int i; // index of the vertex on this mechanism that you want to glue
    int n; // what mechanism it glues to 
    Eigen::Vector3f gluePosition; // the x,y,z position of where you want to glue on the flat pattern on mechanism n
    Eigen::Vector3f glueWeights; // the weights of the gluePosition using mechanisms n's frame
};

enum class CreaseType {
    Mountain, 
    Valley
};

struct Crease { // a crease is a mountain or valley fold that connects vertices i and j. The crease connects a bottom point to a sphere point. 
    int i;
    int j;
    CreaseType type;
};

struct CoordFrame { // a coordinate frame is a set of three unit vectors which can be used to access any coordinate on the mechanism (does not need to span R3)
    Eigen::Vector3f u; 
    Eigen::Vector3f v;
    Eigen::Vector3f w;
    Eigen::Vector3f origin; // origin is going to be it's own point not associated with the pattern
    // origin is always defined in x,y,z coordinates, but the origin for currentposition can look different than the flat origin position
    // the origin can always be derived from the current position
    // for now, the origin is defined as the bottom crease vertex position, later the origin might be off the pattern
};

class Mechanism{
    private: // FIX: is there a way to make it so that things that "never change" are innaccesible by the user? actually aren't all these private members innaccessible by the user?
        std::vector<Eigen::Vector3f> flatPos; // given by user, never changes
        std::vector<Crease> creases; // given by user, never changes
        std::vector<GlueDot> glueDots; //given by user, never changes. Part of glueDots will be calculated later in the PopUpCard class, but the user needs to call calculateglueweights
        bool baseCard; // given by user, never changes. If this mechanism is the base card, this is turned on. Should only have 1 base card
        int id; // what mechanism is this, given by user, never changes

        std::vector<Eigen::Vector3f> currentPos; // calculated here, will change over time
        std::vector<Point> points; // calculated here, never changes
        CoordFrame flatFrame; // calculated here, does not change
        CoordFrame currentFrame; // calculated here, changes over time

    public:
    //Constructor
    Mechanism(const std::vector<Eigen::Vector3f>& _flatPos,
              const std::vector<Crease>& _creases,
              const std::vector<GlueDot>& _glueDots, 
              const bool& _baseCard, // points 4 and 5 on the base card will always be the one we rotate
              int _id) 
              : flatPos(_flatPos),
              creases(_creases), 
              glueDots(_glueDots),
              baseCard(_baseCard), 
              id(_id) {
                currentPos = flatPos;
                points = identifyPointTypes();
                flatFrame = calculateFrameAndOrigin();
                currentFrame = flatFrame;
                points = calculateWeights();
              };

    //Selectors
    Point& getPoint(int i);
    std::vector<GlueDot>& getGlueDots();
    void printFlatPattern();

    Eigen::Vector3f getFlatVertex(int i) const;
    Eigen::Vector3f getCurrentVertex(int i) const;
    void setCurrentVertex(int i, const Eigen::Vector3f& position);

    const CoordFrame& getFlatFrame() const;    
    const CoordFrame& getCurrentFrame() const;
    void updateCurrentFrame();

    

    //Methods
    std::vector<Point> identifyPointTypes(); // only happens once
    CoordFrame calculateFrameAndOrigin(); // current Frame and Origin updates every step using currentPositions    
    std::vector<Point> calculateWeights(); // only happens once
    


    // FIX: need to build a dependency tree? mechanism 0 build on nothing (base card), mechanism 1 builds on mechanism 0, mechanism 2 build on mechanisms 0 and 1. 


    
    // it's starting to come together! I now have the flat Positions, Points types and wiehgts, and Frame!
    // now to start putting it together and actuating the mechanisms! 
    //CONTINUE HERE: follow the steps indicated below, starting from the base card to the current mechanism. Once that is done you can ask claude to help you visualize it. 

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