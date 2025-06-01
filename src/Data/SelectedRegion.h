#pragma once

#include "Grib/Grib.h"
#include "Data/ForecastRepo.h"

#include <boost/functional/hash.hpp>
#include <stdint.h>
#include <string>
#include <vector>

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
    std::string mapBackground;
    std::string outputFolder;
    GeoCoord regionalCoord;
    GeoBounds regionBoundsWithOverflow, renderableRegionBounds, forecastAreaBounds;

    std::vector<std::tuple<std::string, Location>> allLocations;

public:
    SelectedRegion(WeatherModel wxModel, std::string key);

    void RenderRegionalForecastPaths(std::filesystem::path& pathToJson, std::filesystem::path& pathToPng) const;
    void RenderRegionalForecast(const std::filesystem::path& pathToJson, const std::filesystem::path& pathToPng) const;

    inline const std::string& GetMapBackgroundFileName() const {return mapBackground; }
    inline const GeoCoord& GetRegionalCoord() const { return regionalCoord; }
    inline const GeoBounds& GetRegionBoundsWithOverflow() const { return regionBoundsWithOverflow; }
    inline const GeoBounds& GetRenderableRegionBounds() const { return renderableRegionBounds; }
    inline const GeoBounds& GetForecastAreaBounds() const { return forecastAreaBounds; }
    inline const std::string& GetOutputFolder() const { return outputFolder; }

    inline const std::vector<std::tuple<std::string, Location>>& GetAllLocations() const { return allLocations; }
};