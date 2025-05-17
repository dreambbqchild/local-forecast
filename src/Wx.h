#pragma once
#include "Calcs.h"

enum PrecipitationType : uint8_t
{
    NoPrecipitation = 0,
    Rain = 1 << 0,
    FreezingRain = 1 << 1,
    Snow = 1 << 2
};

inline PrecipitationType operator|(PrecipitationType a, PrecipitationType b) { return static_cast<PrecipitationType>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
inline PrecipitationType operator&(PrecipitationType a, PrecipitationType b) { return static_cast<PrecipitationType>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
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
