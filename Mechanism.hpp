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
//  bl  b  br    // bl = bottomLeft, b = bottom, br = bottomRight
//      ^ crease line
// FIX: my diagram has been upside down this entire time, but that might have been a good thing!
// meaning, I should assume that one end of the crease is a sphere point, and the other is either a glue point, the origin, or a general point. 
// I think the sphere point can be identified as the vertex on the crease which is further away from the origin. 

// Assume every v-fold is a simple triangle with 1 crease, three glue points, glue points are at the bottom edge
// Assume the flat pattern is given in a way such that the vertices are connected cyclicly 
//FIX: I should really store the bl/b/br indices somehwere instead of finding it every time

enum class PointType{
    gluePoints, 
    spherePoints, 
    general
};

struct Point { // Every vertex maps to a point. A point has a type and it has weights for the localframe to tell you where it is relative to the true local origin
    int i; // index of this point (same as vertex index) 
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

struct GlueTabs {
    int bll; // BottomLeftLeft vertex index
    int bl;  // Bottomleft vertex index
    int brr; // BottomRightRightvertex index
    int br;  // BottomRight vertex index
    int sphere; // sphere index, not part of the glue tabs, but needed to store this info somewhere
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
        GlueTabs glueTabs; // calculated here, never changes. helps find the origin

    public:
    //Constructor
    //FIX: can get rid of bool base card because we can assume that mech 0 is always the base card
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
                glueTabs = findGlueTabs();
                points = identifyPointTypes();
                flatFrame = calculateFrameAndOrigin();
                currentFrame = flatFrame;
                points = calculateWeights();
              };

    //Selectors
    int getID();
    Point& getPoint(int i);
    std::vector<Point> getPoints();
    std::vector<Crease> getCreases();
    std::vector<GlueDot>& getGlueDots();
    void printFlatPattern();
    void printCurrentPos();
    void printPoints();

    Eigen::Vector3f getFlatVertex(int i) const;
    Eigen::Vector3f getCurrentVertex(int i) const;
    std::vector<Eigen::Vector3f> getCurrentPos() const;
    void setCurrentVertex(int i, const Eigen::Vector3f& position);

    const CoordFrame& getFlatFrame() const;    
    const CoordFrame& getCurrentFrame() const;
    const GlueTabs& getGlueTabs() const;
    void updateCurrentFrame();

    //Methods
    GlueTabs findGlueTabs(); // only happens once
    std::vector<Point> identifyPointTypes(); // only happens once
    CoordFrame calculateFrameAndOrigin(); // current Frame and Origin updates every step using currentPositions    
    std::vector<Point> calculateWeights(); // only happens once
    

    std::string pointTypeToString(PointType type);
};


// My theory: All creases on v-fold mechanisms and parrelel fold mechanisms will intersect the central fold of the mechanism they are attatched to. 
// v-fold mechanisms meet on the card (or extension of the card) and parrallel folds meet at infinity. 


// Definition: A V-fold mechanism is a mechanism with exactly 2 patches and 1 crease such that
// the crease line the crease line intersects the gulley which actuates the mechanism. 
// the crease and the gulley can intersect, once, infinitely (overlapping), or at infinite (parrallel)

// Lemma: a V-fold is flat-folding if... (Duncan Birmingham knows why! and so does mathematics! just have to find it...)

#endif
