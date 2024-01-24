#include "Error.h"
#include "SelectedLocation.h"

#include <fstream>
#include <json/json.h>

using namespace std;

SelectedLocation::SelectedLocation(string key)
{
    Json::Value settings;

    ifstream forecastJson;
    forecastJson.open("settings.json");
    forecastJson >> settings;
    forecastJson.close();

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

    geoBounds.leftLon = bounds["leftLon"].asDouble();
    geoBounds.rightLon = bounds["rightLon"].asDouble();
    geoBounds.topLat = bounds["topLat"].asDouble();
    geoBounds.bottomLat = bounds["bottomLat"].asDouble();

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