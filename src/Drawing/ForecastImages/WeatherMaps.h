#pragma once
#include "Grib/LocationWeatherData.h"

#include <json/json.h>

#include <filesystem>

class IWeatherMaps
{
public:
    virtual void GenerateForecastMaps(std::filesystem::path forecastDataOutputDir) = 0;
    virtual ~IWeatherMaps() = default;
};

IWeatherMaps* AllocWeatherMaps(Json::Value& root, LocationWeatherData& locationWeatherData, GeographicCalcs& geoCalcs);