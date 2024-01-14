#pragma once

#include <stdint.h>

struct Location {
    char name[32];
    double lat, lon;
    bool isCity;
};

inline int32_t CountCityLocations(const Location* locs, int32_t length)
{
    auto count = 0;
    for(auto i = 0 ; i < length; i++)
    {
        if(locs[i].isCity)
            count++;
    }

    return count;
}

inline int32_t CountPeopleLocations(const Location* locs, int32_t length) 
{
    auto cities = CountCityLocations(locs, length);
    return length - cities;
}

extern Location locations[];
extern const int locationsLength;