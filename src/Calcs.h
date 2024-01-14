#pragma once
#include <stdint.h>
#include <math.h>

template <typename T>
struct PointTemplate {
    T x, y;
};

template <typename T>
struct SizeTemplate
{
    T width, height;
};

typedef PointTemplate<double> DoublePoint;
typedef PointTemplate<int16_t> Int16Point;

typedef SizeTemplate<int16_t> Int16Size;

inline bool operator==(const DoublePoint& lhs, const DoublePoint& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline double ToFarenheight(double k) { return (k - 273.15) * 9/5.0 + 32.0; }
inline double ToInchesFromKgPerSquareMeter(double kgPerSquareMeter) { return kgPerSquareMeter * 0.04; }
inline double ToInchesFromMeters(double m) { return m * 39.37; }
inline double ToInHg(double pa) { return pa / 3386.0; }
inline double ToInPerHour(double mmPerSec) { return mmPerSec * 141.7; }
inline double ToMiles(double m) { return m * 0.000621371; }
inline double ToMPH(double mPerS) { return mPerS * 2.237; }
inline double WindDirection(double u, double v) { return (180 / 3.14159265358979323846) * atan2(u, v) + 180;}
inline double WindSpeed(double u, double v) { return sqrt(u * u + v * v) * 1.94384; }

inline bool TestDouble(double dValue, int32_t multiplier = 100, int32_t threshold = 1) { return static_cast<int32_t>(floor(dValue * multiplier)) >= threshold; }