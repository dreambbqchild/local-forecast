#pragma once

#include "Geography/Geo.h"
#include "Grib/GribData.h"

#include <json/json.h>
#include <filesystem>

class IWeatherMaps
{
public:
    virtual void GenerateForecastMaps(std::filesystem::path forecastDataOutputDir) = 0;
    virtual ~IWeatherMaps() = default;
};

IWeatherMaps* AllocWeatherMaps(Json::Value& root, const std::unique_ptr<GribData>& gribData, GeographicCalcs& geoCalcs, const std::string& mapBackgroundFile);