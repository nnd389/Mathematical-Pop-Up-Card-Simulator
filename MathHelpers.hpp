#ifndef MATHHELPERS_HPP
#define MATHHELPERS_HPP

#include <vector>
#include <cmath>
#include <Eigen/Dense>

Eigen::Vector3f RodriguesRotation(const Eigen::Vector3f v, const Eigen::Vector3f k, float theta){
    // v_rot = v*cos(theta) + (k x v)*sin(theta) + k*(k dot v)*(1-cos(theta))
    // might need to normalize k? check wikipedia

    Eigen::Vector3f v_rot = v*cos(theta) + (k.cross(v))*sin(theta) + k*(k.dot(v))*(1-cos(theta));
    return v_rot;
}

Eigen::Vector3f IntersectThreeSpheres(const Eigen::Vector3f D, const float rD, const Eigen::Vector3f A, const float rA, const Eigen::Vector3f C, const float rC, const int flag){
    // gonna use Andrew Glassner's approach for this, which is:
    // 1: solve for two radical planes
    // 2: intersect two planes to get a radical line
    // 3: intersect line and any sphere to get intersection point(s)

    // I'm using D,A,C (instead of A,B,C) just cuz it matches a little better with Andrews notes (and mine for that matter)

    float d1 = (A-D).norm(); // d1 = distance from center A to center D
    float a1 = (rD*rD - rA*rA + d1*d1)/(2*d1); // a1 = distance from center D to radical plane(?) for spheres A and D
    Eigen::Vector3f J1 = D + (a1/d1)*(A-D); // J1 = point of intersection for the line AD and the radical plane for spheres A and D
    Eigen::Vector3f n1 = (A-D)/d1; // n1 = the normal which defines the radical plane for spheres A and D
    // the plane equation for the radical plane for sphere A and D is: n1 dot (x-J1) = 0

    float d2 = (C-A).norm(); // d2 = distance from center C to center A
    float a2 = (rA*rA - rC*rC + d2*d2)/(2*d2); // a2 = distance from center A to radical plane(?) for spheres C and A
    Eigen::Vector3f J2 = A + (a2/d2)*(C-A); // J2 = point of intersection for the line CA and the radical plane for spheres C and A
    Eigen::Vector3f n2 = (C-A)/d2; // n2 = the normal which defines the radical plane for spheres C and A
    // the plane equation for the radical plane for sphere C and A is: n2 dot (x-J2) = 0

    Eigen::Vector3f v = n2.cross(n1); // v = the direction of the intersecting line for radical planes 1 and 2
    Eigen::Vector3f p = (n2.dot(J2)*(n1.cross(v)) + n1.dot(J1)*((v).cross(n2))) / ((v).squaredNorm()); // p = a point on line v?
    // The line of intersection for radical planes 1 and 2 is: x(t) = p + t*v

    // Intersect line x(t) with sphere D by solving the quadratic equation: ||x(t) - D||^2 = rD^2
    Eigen::Vector3f q = p-D; // q = a substituting variable to help us simplify a quadratic equation
    float a = v.dot(v);
    float b = 2.0*q.dot(v);
    float c = q.dot(q) - rD*rD;
    float tPlus = (-b + std::sqrt(b*b - 4*a*c)) / (2.0*a);
    float tMinus = (-b - std::sqrt(b*b - 4*a*c)) / (2.0*a);

    // the point(s) of intersection of the three spheres is: S = p + t_{+-}*v
    Eigen::Vector3f S1 = p + tPlus*v;
    Eigen::Vector3f S2 = p + tMinus*v;


    // Checks:
    const float eps = 1e-6f;
    float v2 = v.squaredNorm();
    if (d1 < eps) {
        // Coincident sphere centers
    }
    if (v2 < eps) {
        // Radical planes are parallel
    }
    float discriminant = b*b - 4.0f*a*c;
    if (discriminant < -eps) {
        // No real intersection of line and sphere
    }
    // decide how to handle - if you continue to use this function for parrallel folds you'll definitely run into at least one of these issues 

    // want the zero flag to return the S point with the lower z value
    if (flag == 0) {
    return (S1.z() < S2.z()) ? S1 : S2;
    }
    else {
        return (S1.z() >= S2.z()) ? S1 : S2;
    }
}

#endif

