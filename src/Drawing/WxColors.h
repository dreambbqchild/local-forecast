#pragma once
#include "Wx.h"
#include "Drawing/DrawService.h"
#include <stdint.h>

typedef DSColor WxColor;

WxColor ColorFromDegrees(double farenheight);
WxColor ColorFromPrecipitation(PrecipitationType precipitationType, double rate);
WxColor ColorFromWind(double velocity);