#include "Error.h"
#include "SelectedRegion.h"

#include <fstream>
#include <json/json.h>
#include <limits>

using namespace std;

void GetGeoBoundsFromJsonValue(GeoBounds& geoBounds, Json::Value& bounds)
{
    geoBounds.leftLon = bounds["leftLon"].asDouble();
    geoBounds.rightLon = bounds["rightLon"].asDouble();
    geoBounds.topLat = bounds["topLat"].asDouble();
    geoBounds.bottomLat = bounds["bottomLat"].asDouble();
}

SelectedRegion::SelectedRegion(string key)
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
        GetGeoBoundsFromJsonValue(currentArea, member["geoBounds"]);
        
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

    GetGeoBoundsFromJsonValue(regionBounds, bounds);

    for(auto& jLocation : locations)
    {
        Location location {
            .name = jLocation["name"].asString(),
            .lat = jLocation["lat"].asDouble(),
            .lon = jLocation["lon"].asDouble(),
            .isCity = jLocation["isCity"].asBool()
        };

        allLocations.push_back(location);
    }
}