#pragma once
#include <chrono>
#include <string>

enum LunarPhase : uint8_t {
    New,
    WaxingCrescent,
    FirstQuarter,
    WaxingGibbous,
    WolfMoon,
    SnowMoon,
    WormMoon,
    PinkMoon,
    FlowerMoon,
    StrawberryMoon,
    BuckMoon,
    SturgeonMoon,
    CornMoon,
    HunterMoon,
    BeaverMoon,
    ColdMoon,
    BlueMoon,
    HarvestMoon,
    WaningGibbous,
    LastQuarter,
    WaningCrescent
};

struct SunriseSunset {
    std::chrono::system_clock::time_point rise, set;
};

class Astronomy {
public:
    static SunriseSunset GetSunRiseSunset(double lat, double lon, std::chrono::system_clock::time_point timePoint);
    static LunarPhase GetLunarPhase(std::chrono::system_clock::time_point timePoint);
    static const char* GetLunarPhaseEmoji(LunarPhase lunarPhase);
    static const char* GetLunarPhaseLabel(LunarPhase lunarPhase);
};