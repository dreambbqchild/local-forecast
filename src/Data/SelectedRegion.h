#pragma once

#include "Grib/Grib.h"

#include <boost/functional/hash.hpp>
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

template<> 
struct std::hash<GeoCoord>
{
  size_t operator()(const GeoCoord& k) const
  {
    auto h1 = hash<double>{}(k.lat);
    auto h2 = hash<double>{}(k.lon);

    size_t seed = 0;
    boost::hash_combine(seed, h1);
    boost::hash_combine(seed, h2);
    return seed;
  }
};

inline bool operator==(const GeoCoord& lhs, const GeoCoord& rhs) {
    return lhs.lat == rhs.lat && lhs.lon == rhs.lon;
}

struct GeoBounds {
    double leftLon = 0, rightLon = 0, topLat = 0, bottomLat = 0;
};

class SelectedRegion {
private:
    const Location empty;
    std::string mapBackground;
    std::string outputFolder;
    GeoCoord regionalCoord;
    GeoBounds regionBoundsWithOverflow, renderableRegionBounds, forecastAreaBounds;

    std::vector<Location> allLocations;

public:
    SelectedRegion(WeatherModel wxModel, std::string key);

    inline const std::string& GetMapBackgroundFileName() const {return mapBackground; }
    inline const GeoCoord& GetRegionalCoord() const { return regionalCoord; }
    inline const GeoBounds& GetRegionBoundsWithOverflow() const { return regionBoundsWithOverflow; }
    inline const GeoBounds& GetRenderableRegionBounds() const { return renderableRegionBounds; }
    inline const GeoBounds& GetForecastAreaBounds() const { return forecastAreaBounds; }
    inline const std::string& GetOutputFolder() const { return outputFolder; }

    inline const std::vector<Location>& GetAllLocations() const { return allLocations; }
};