#pragma once
#include "Wx.h"
#include "Drawing/DrawService.h"
#include <stdint.h>

DSColor ColorFromDegrees(double farenheight);
DSColor ColorFromPrecipitation(PrecipitationType precipitationType, double rate);
DSColor ColorFromWind(double velocity);