#include "Error.h"
#include "SelectedRegion.h"
#include "Drawing/ForecastImages/RegionalForecast.h"

#include <fstream>
#include <json/json.h>
#include <limits>

using namespace std;
namespace fs = std::filesystem;

void GetGeoBoundsFromJsonValue(WeatherModel wxModel, GeoBounds& geoBounds, Json::Value& bounds)
{
    geoBounds.leftLon = bounds["leftLon"].asDouble();
    geoBounds.rightLon = bounds["rightLon"].asDouble();
    geoBounds.topLat = bounds["topLat"].asDouble();
    geoBounds.bottomLat = bounds["bottomLat"].asDouble();

    if(wxModel == WeatherModel::HRRR)
    {
        geoBounds.leftLon -= 0.25,
        geoBounds.topLat += 0.25,
        geoBounds.rightLon += 0.25,
        geoBounds.bottomLat -= 0.25;
    }
    else if(wxModel == WeatherModel::GFS)
    {
        geoBounds.leftLon -= 0.5,
        geoBounds.topLat += 0.5,
        geoBounds.rightLon += 0.5,
        geoBounds.bottomLat -= 0.5;
    }
}

SelectedRegion::SelectedRegion(WeatherModel wxModel, string key)
{
    Json::Value settings;

    ifstream forecastJson;
    forecastJson.open("settings.json");
    forecastJson >> settings;
    forecastJson.close();

    forecastAreaBounds.bottomLat = forecastAreaBounds.leftLon = numeric_limits<double>::max();
    forecastAreaBounds.topLat = forecastAreaBounds.rightLon = numeric_limits<double>::lowest();

    for(auto& member : settings)
    {
        GeoBounds currentArea = {0};
        GetGeoBoundsFromJsonValue(wxModel, currentArea, member["geoBounds"]);
        
        forecastAreaBounds.leftLon = min(forecastAreaBounds.leftLon, currentArea.leftLon);
        forecastAreaBounds.rightLon = max(forecastAreaBounds.rightLon, currentArea.rightLon);
        forecastAreaBounds.topLat = max(forecastAreaBounds.topLat, currentArea.topLat);
        forecastAreaBounds.bottomLat = min(forecastAreaBounds.bottomLat, currentArea.bottomLat);
    }

    if(!settings.isMember(key))
        ERR_OUT(key << " is not a member of settings.json");

    auto settingData = settings[key];
    auto bounds = settingData["geoBounds"];
    auto locations = settingData["locations"];
    auto regionalForecastCoords = settingData["regionalForecastCoords"];

    mapBackground = settingData["mapBackground"].asString();
    outputFolder = settingData["outputFolder"].asString();

    regionalCoord.lat = regionalForecastCoords["lat"].asDouble();
    regionalCoord.lon = regionalForecastCoords["lon"].asDouble();

    GetGeoBoundsFromJsonValue(wxModel, regionBoundsWithOverflow, bounds);
    GetGeoBoundsFromJsonValue(WeatherModel::NoWeatherModel, renderableRegionBounds, bounds);

    for(auto& jLocation : locations)
    {
        Location location {
            .coords {
                .lat = jLocation["lat"].asDouble(),
                .lon = jLocation["lon"].asDouble()
            },
            .isCity = jLocation["isCity"].asBool(),
            .suns = nullptr,
            .sunsLen = 0,
            .wx = nullptr,
            .wxLen = 0
        };

        allLocations.push_back({jLocation["name"].asString(), location});
    }
}

void SelectedRegion::RenderRegionalForecastPaths(fs::path& pathToJson, fs::path& pathToPng) const
{
    fs::path openweatherPath = fs::path("forecasts") / GetOutputFolder();
    pathToJson = openweatherPath / string("openweather.json");
    pathToPng = openweatherPath /  string("openweather.png");
}

void SelectedRegion::RenderRegionalForecast(const fs::path& pathToJson, const fs::path& pathToPng) const
{
    RegionalForecast regionalForecast;
    regionalForecast.Render(*this, pathToJson, pathToPng);
}