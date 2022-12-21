#ifndef WX_H
#define WX_H
#include "calcs.h"

enum PrecipitationType 
{
    None = 0,
    Rain = 1 << 0,
    FreezingRain = 1 << 1,
    Snow = 1 << 2
};

inline PrecipitationType operator|(PrecipitationType a, PrecipitationType b) { return static_cast<PrecipitationType>(static_cast<int>(a) | static_cast<int>(b)); }
inline PrecipitationType operator&(PrecipitationType a, PrecipitationType b) { return static_cast<PrecipitationType>(static_cast<int>(a) & static_cast<int>(b)); }
inline PrecipitationType& operator |=(PrecipitationType& a, PrecipitationType b) { return a = a | b; }

struct Wx 
{
    PrecipitationType type;
    double
        dewpoint, 
        precipitationRate, 
        gust,
        lightning,
        newPrecipitation,
        pressure,
        snowDepth,
        temperature, 
        totalCloudCover,
        totalPrecipitation,
        totalSnow,
        visibility,
        windSpeed,
        windU,
        windV;

    inline double WindDirection() { return ::WindDirection(windU, windV); }
    inline double WindSpeed() { return ::WindSpeed(windU, windV); }
};

double ScaledValueForTypeAndTemp(PrecipitationType type, double value, double temperature);

#endif