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

class SelectedLocation {
private:
    const Location empty;
    std::string mapBackground;
    std::string outputFolder;
    GeoCoord regionalCoord;
    GeoBounds geoBounds;

    std::vector<Location> allLocations;

public:
    SelectedLocation(std::string key);

    inline const std::string& GetMapBackgroundFileName() const {return mapBackground; }
    inline const GeoCoord& GetRegionalCoord() const { return regionalCoord; }
    inline const GeoBounds& GetGeoBounds() const { return geoBounds; }
    inline const std::string& GetOutputFolder() const { return outputFolder; }

#define LocationGetters(label, vector) inline size_t label ## LocationCount() const { return vector.size(); }\
inline const Location& const GetLocation(size_t index) const \
{\
    if(index < 0 || index >= vector.size())\
        return empty;\
\
    return vector[index];\
}
    inline const std::vector<Location>& GetAllLocations() const { return allLocations; }
#undef LocationGetters
};