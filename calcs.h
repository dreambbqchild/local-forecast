#ifndef CALCS_H
#define CALCS_H
#include <stdint.h>
#include <math.h>

struct Point {
    double x, y;
};

struct ValuedPoint : Point {
    double value;
};

struct IndexedPoint : Point {
    uint16_t index;
};

inline double ToFarenheight(double k) { return (k - 273.15) * 9/5.0 + 32.0; }
inline double ToInchesFromKgPerSquareMeter(double kgPerSquareMeter) { return kgPerSquareMeter * 0.04; }
inline double ToInchesFromMeters(double m) { return m * 39.37; }
inline double ToInHg(double pa) { return pa / 3386.0; }
inline double ToInPerHour(double mmPerSec) { return mmPerSec * 141.7; }
inline double ToMiles(double m) { return m * 0.000621371; }
inline double ToMPH(double mPerS) { return mPerS * 2.237; }
inline double WindDirection(double u, double v) { return (180 / 3.14159265358979323846) * atan2(u, v) + 180;}
double inline WindSpeed(double u, double v) { return sqrt(u * u + v * v) * 1.94384; }

inline double FindSlope(Point& l, Point& r) 
{
    auto m =  (r.y - l.y) / (r.x - l.x);
    if(r.y < l.y)
        m *= -1;

    return m;
};

inline double FindLineOffset(Point& pt, double m) { return pt.y - (m * pt.x); }

inline double FindYForLineBetweenAtX(Point& l, Point& r, double x) 
{
    auto m = FindSlope(l, r);
    auto b = FindLineOffset(l, m);
    return m * x + b;
}

inline double Lerp(double c1, double c2, double c, double v1, double v2) 
{
    return (((c2 - c) / (c2 - c1)) * v1) + (((c - c1) / (c2 - c1)) * v2);
}

inline double Blerp(ValuedPoint lt, ValuedPoint rt, ValuedPoint rb, ValuedPoint lb, Point pt)
{
    auto r1 = Lerp(lt.x, rt.x, pt.x, lt.value, rt.value);
    auto r2 = Lerp(lb.x, rb.x, pt.x, lb.value, rb.value);
    auto ty = FindYForLineBetweenAtX(lt, rt, pt.x);
    auto by = FindYForLineBetweenAtX(lb, rb, pt.x);
    return Lerp(ty, by, pt.y, r1, r2);
}

inline double Distance(double x1, double y1, double x2, double y2) { return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)); }

inline double TriangleArea(int x1, int y1, int x2, int y2, int x3, int y3) { return abs((x1*(y2-y3) + x2*(y3-y1)+ x3*(y1-y2))/2.0); }

inline bool IsInsideTriangleOf(int x1, int y1, int x2, int y2, int x3, int y3, int x, int y)
{
    double A = TriangleArea (x1, y1, x2, y2, x3, y3);
    double A1 = TriangleArea (x, y, x2, y2, x3, y3);

    double A2 = TriangleArea (x1, y1, x, y, x3, y3);
    double A3 = TriangleArea (x1, y1, x2, y2, x, y);

    return (A == A1 + A2 + A3);
}

#endif