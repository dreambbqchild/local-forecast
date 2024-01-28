#pragma once

#include <stdint.h>
#include <string>
#include <vector>

struct Location {
    std::string name;
    double lat = 0, lon = 0;
    bool isCity = false;
};

struct GeoCoord {
    double lat, lon;
};

struct GeoBounds {
    double leftLon = 0, rightLon = 0, topLat = 0, bottomLat = 0;
};

class SelectedRegion {
private:
    const Location empty;
    std::string mapBackground;
    std::string outputFolder;
    GeoCoord regionalCoord;
    GeoBounds regionBounds;
    GeoBounds forecastAreaBounds;

    std::vector<Location> allLocations;

public:
    SelectedRegion(std::string key);

    inline const std::string& GetMapBackgroundFileName() const {return mapBackground; }
    inline const GeoCoord& GetRegionalCoord() const { return regionalCoord; }
    inline const GeoBounds& GetRegionBounds() const { return regionBounds; }
    inline const GeoBounds& GetForecastAreaBounds() {return forecastAreaBounds; }
    inline const std::string& GetOutputFolder() const { return outputFolder; }

    inline const std::vector<Location>& GetAllLocations() const { return allLocations; }
};