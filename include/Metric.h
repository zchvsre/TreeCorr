/* Copyright (c) 2003-2015 by Mike Jarvis
 *
 * TreeCorr is free software: redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions, and the disclaimer given in the accompanying LICENSE
 *    file.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the disclaimer given in the documentation
 *    and/or other materials provided with the distribution.
 */

#ifndef TreeCorr_Metric_H
#define TreeCorr_Metric_H

// We use a code for the metric to use:
// Euclidean is Euclidean in (x,y) or (x,y,z)
// Perp uses the perpendicular component of the separation as the distance
// Arc uses the great circle distance between two points on the sphere
enum Metric { Euclidean=1, Perp=2, Arc=3 };

template <int M>
struct MetricHelper;

//
//
// Euclidean is valid for Coord == Flat, ThreeD, or Sphere
//
//

template <>
struct MetricHelper<Euclidean>
{
    ///
    //
    // Flat
    //
    ///

    static double DistSq(const Position<Flat>& p1, const Position<Flat>& p2)
    { 
        Position<Flat> r = p1-p2;
        return r.getX()*r.getX() + r.getY()*r.getY(); 
    }
    static double Dist(const Position<Flat>& p1, const Position<Flat>& p2)
    { return sqrt(DistSq(p1,p2)); }

    static bool CCW(const Position<Flat>& p1, const Position<Flat>& p2, const Position<Flat>& p3)
    {
        // If cross product r21 x r31 > 0, then the points are counter-clockwise.
        double x2 = p2.getX() - p1.getX();
        double y2 = p2.getY() - p1.getY();
        double x3 = p3.getX() - p1.getX();
        double y3 = p3.getY() - p1.getY();
        return (x2*y3 - x3*y2) > 0.;
    }

    static bool TooSmallDist(const Position<Flat>& , const Position<Flat>& , double s1ps2, 
                             double dsq, double minsep, double minsepsq)
    { return dsq < minsepsq && s1ps2 < minsep && dsq < SQR(minsep - s1ps2); }

    static bool TooLargeDist(const Position<Flat>& , const Position<Flat>& , double s1ps2, 
                             double dsq, double maxsep, double maxsepsq)
    { return dsq >= maxsepsq && dsq >= SQR(maxsep + s1ps2); }



    ///
    //
    // ThreeD
    // Note: Position<Sphere> *isa* Position<ThreeD>, we don't need to do any overloads for Sphere.
    // 
    ///

    static double DistSq(const Position<ThreeD>& p1, const Position<ThreeD>& p2)
    { 
        Position<ThreeD> r = p1-p2;
        return r.getX()*r.getX() + r.getY()*r.getY() + r.getZ()*r.getZ(); 
    }
    static double Dist(const Position<ThreeD>& p1, const Position<ThreeD>& p2)
    { return sqrt(DistSq(p1,p2)); }

    static bool CCW(const Position<ThreeD>& p1, const Position<ThreeD>& p2,
                    const Position<ThreeD>& p3)
    {
        // Now it's slightly more complicated, since the points are in three dimensions.  We do
        // the same thing, computing the cross product with respect to point p1.  Then if the 
        // cross product points back toward Earth, the points are viewed as counter-clockwise.
        // We check this last point by the dot product with p1.
        double x2 = p2.getX() - p1.getX();
        double y2 = p2.getY() - p1.getY();
        double z2 = p2.getZ() - p1.getZ();
        double x3 = p3.getX() - p1.getX();
        double y3 = p3.getY() - p1.getY();
        double z3 = p3.getZ() - p1.getZ();
        double cx = y2*z3 - y3*z2;
        double cy = z2*x3 - z3*x2;
        double cz = x2*y3 - x3*y2;
        return cx*p1.getX() + cy*p1.getY() + cz*p1.getZ() < 0.;
    }

    static bool TooSmallDist(const Position<ThreeD>& , const Position<ThreeD>& , double s1ps2, 
                             double dsq, double minsep, double minsepsq)
    { return dsq < minsepsq && s1ps2 < minsep && dsq < SQR(minsep - s1ps2); }

    static bool TooLargeDist(const Position<ThreeD>& , const Position<ThreeD>& , double s1ps2, 
                             double dsq, double maxsep, double maxsepsq)
    { return dsq >= maxsepsq && dsq >= SQR(maxsep + s1ps2); }

};

//
//
// Perp is only valid for Coord == ThreeD
//
//

template <>
struct MetricHelper<Perp>
{
    static double DistSq(const Position<ThreeD>& p1, const Position<ThreeD>& p2)
    { 
        // r_perp^2 + r_parallel^2 = d^2
        Position<ThreeD> r = p1-p2;
        double dsq = r.getX()*r.getX() + r.getY()*r.getY() + r.getZ()*r.getZ(); 
        double r1sq = p1.getX()*p1.getX() + p1.getY()*p1.getY() + p1.getZ()*p1.getZ(); 
        double r2sq = p2.getX()*p2.getX() + p2.getY()*p2.getY() + p2.getZ()*p2.getZ(); 
        // r_parallel^2 = (r1-r2)^2 = r1^2 + r2^2 - 2r1r2
        double rparsq = r1sq + r2sq - 2.*sqrt(r1sq*r2sq);
        return dsq - rparsq;
    }
    static double Dist(const Position<ThreeD>& p1, const Position<ThreeD>& p2)
    { return sqrt(DistSq(p1,p2)); }

    static bool CCW(const Position<ThreeD>& p1, const Position<ThreeD>& p2, 
                    const Position<ThreeD>& p3)
    {
        // This is the same as ThreeD
        double x2 = p2.getX() - p1.getX();
        double y2 = p2.getY() - p1.getY();
        double z2 = p2.getZ() - p1.getZ();
        double x3 = p3.getX() - p1.getX();
        double y3 = p3.getY() - p1.getY();
        double z3 = p3.getZ() - p1.getZ();
        double cx = y2*z3 - y3*z2;
        double cy = z2*x3 - z3*x2;
        double cz = x2*y3 - x3*y2;
        return cx*p1.getX() + cy*p1.getY() + cz*p1.getZ() < 0.;
    }

    // This one is a bit subtle.  The maximum possible rp can be larger than just (rp + s1ps2).
    // The most extreme case is if the two cells are in opposite directions from Earth.
    // Of course, this won't happen too often in practice, but might as well use the 
    // most conservative case here.  In this case, the cell size can serve both to
    // increase d by s1ps2 and decrease |r1-r2| by s1ps2.  So rp can become
    // rp'^2 = (d+s1ps2)^2 - (rpar-s1ps2)^2
    //       = d^2 + 2d s1ps2 + s1ps2^2 - rpar^2 + 2rpar s1ps2 - s1ps2^2
    //       = rp^2 + 2(d+rpar) s1ps2
    // rp'^2 < minsep^2
    // rp^2 + 2(d + rpar) s1ps2 < minsepsq
    // 4 (d^2 + 2d rpar + rpar^2) s1ps2^2 < (minsepsq - rp^2)^2
    static bool TooSmallDist(const Position<ThreeD>& p1, const Position<ThreeD>& p2, double s1ps2, 
                             double dsq, double minsep, double minsepsq)

    {
        // First a simple check that will work most of the time.
        if (dsq >= minsepsq || s1ps2 >= minsep || dsq >= SQR(minsep - s1ps2)) return false;
        // Now check the subtle case.
        double r1sq = p1.getX()*p1.getX() + p1.getY()*p1.getY() + p1.getZ()*p1.getZ(); 
        double r2sq = p2.getX()*p2.getX() + p2.getY()*p2.getY() + p2.getZ()*p2.getZ(); 
        double rparsq = r1sq + r2sq - 2.*sqrt(r1sq*r2sq);
        double d3sq = rparsq + dsq;  // The 3d distance.  Remember dsq is really rp^2.
        return (d3sq + 2.*sqrt(d3sq * rparsq) + rparsq) * SQR(2.*s1ps2) < SQR(minsepsq - dsq);
    }

    // This one is similar.  The minimum possible rp can be smaller than just (rp - s1ps2).
    // rp'^2 = (d-s1ps2)^2 - (rpar+s1ps2)^2
    //       = d^2 - 2d s1ps2 + s1ps2^2 - rpar^2 - 2rpar s1ps2 - s1ps2^2
    //       = rp^2 - 2(d+rpar) s1ps2
    // rp'^2 > maxsep^2
    // rp^2 - 2(d + rpar) s1ps2 > maxsepsq
    // 4 (d^2 + 2d rpar + rpar^2) s1ps2^2 < (rp^2 - maxsepsq)^2
    static bool TooLargeDist(const Position<ThreeD>& p1, const Position<ThreeD>& p2, double s1ps2,
                             double dsq, double maxsep, double maxsepsq)
    { 
        if (dsq < maxsepsq || dsq < SQR(maxsep + s1ps2)) return false;
        // Now check the subtle case.
        double r1sq = p1.getX()*p1.getX() + p1.getY()*p1.getY() + p1.getZ()*p1.getZ(); 
        double r2sq = p2.getX()*p2.getX() + p2.getY()*p2.getY() + p2.getZ()*p2.getZ(); 
        double rparsq = r1sq + r2sq - 2.*sqrt(r1sq*r2sq);
        double d3sq = rparsq + dsq;  // The 3d distance.  Remember dsq is really rp^2.
        return (d3sq + 2.*sqrt(d3sq * rparsq) + rparsq) * SQR(2.*s1ps2) <= SQR(dsq - maxsepsq);
    }

};

#endif

