#pragma once
#include <chrono>
#include <string>

struct SunriseSunset {
    std::chrono::system_clock::time_point rise, set;
};

struct LunarPhase {
    double age;
    std::string name;
    std::string emoji;
};

class Astronomy {
    public:
    static SunriseSunset GetSunRiseSunset(double lat, double lon, std::chrono::system_clock::time_point timePoint);
    static LunarPhase GetLunarPhase(std::chrono::system_clock::time_point timePoint);
};