#pragma once

#include <string>
#include <vector>

#include "Geography/Geo.h"
#include "Grib.h"

class GribReader 
{
private:
    WeatherModel wxModel;
    int32_t rows, columns, numberOfValues;
    std::string fileName;
    std::vector<GeoCoord> geoCoords;

public:
    GribReader(std::string fileName, WeatherModel wxModel);
    bool GetFieldData(std::vector<FieldData>& result);
    std::vector<GeoCoord> GetGeoCoords(){ return geoCoords; }
    int32_t GetRows(){return rows;}
    int32_t GetColumns(){return columns;}
    int32_t GetNumberOfValues(){return numberOfValues;}
};