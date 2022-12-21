#ifndef MAPCOLORS_H
#define MAPCOLORS_H
#include "wx.h"
#include <stdint.h>

struct MapColor 
{
    uint8_t r, g, b, a;
};

MapColor ColorFromDegrees(double farenheight);
MapColor ColorFromPrecipitation(PrecipitationType precipitationType, double rate);

#endif