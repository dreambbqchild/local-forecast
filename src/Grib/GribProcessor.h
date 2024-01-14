#pragma once

#include "Calcs.h"
#include "LocationWeatherData.h"

#include <chrono>
#include <filesystem>
#include <json/json.h>
#include <string>

class GribProcessor
{
private:
    class ParallelProcessor;
    
    WeatherModel wxModel;
    uint16_t maxGribIndex, skipToGribNumber;
    std::chrono::system_clock::time_point forecastStartTime;
    std::string gribPathTemplate;
    std::filesystem::path forecastDataOutputDir;
    GeographicCalcs& geoCalcs;

public:
    GribProcessor(std::string gribPathTemplate, WeatherModel wxModel, std::chrono::system_clock::time_point forecastStartTime, uint16_t maxGribIndex, GeographicCalcs& geoCalcs, uint16_t skipToGribNumber = 0);
    void Process(Json::Value& root, LocationWeatherData& locationWeatherData);
};