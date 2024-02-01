#pragma once
#include <boost/functional/hash.hpp>
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

template<>
struct std::hash<DoublePoint>
{
  size_t operator()(const DoublePoint& k) const
  {
    auto h1 = hash<double>{}(k.x);
    auto h2 = hash<double>{}(k.y);

    size_t seed = 0;
    boost::hash_combine(seed, h1);
    boost::hash_combine(seed, h2);
    return seed;
  }
};

inline bool operator==(const DoublePoint& lhs, const DoublePoint& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

typedef SizeTemplate<int16_t> Int16Size;

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

template <typename T>
inline bool IsBetween(T left, T eval, T right) { return eval >= left && eval <= right; }

inline double Distance(const DoublePoint& l, const DoublePoint& r) { return sqrt(pow(r.x - l.x , 2) + pow(r.y - l.y, 2)); }