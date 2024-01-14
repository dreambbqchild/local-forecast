#pragma once
#include <stdint.h>

enum WeatherModel {HRRR, GFS};

struct FieldData 
{
    uint16_t index;
    int32_t parameterNumber;
    const char* name, *level, *shortName, *stepRange, *typeOfLevel;
    double value;
};