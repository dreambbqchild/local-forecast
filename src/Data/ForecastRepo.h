#pragma once
#include "Astronomy/Astronomy.h"
#include "Wx.h"

#include <stdint.h>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <span>

extern "C" {
    typedef struct {
        int16_t dewpoint;
        uint16_t gust;
        uint16_t lightning;
        double newPrecipitation;
        double precipitationRate;
        uint8_t precipitationType;
        double pressure;
        int16_t temperature;
        uint16_t totalCloudCover;
        double totalPrecipitation;
        double totalSnow;
        uint16_t visibility;
        uint16_t windDirection;
        uint16_t windSpeed;
    } WxSingle;

    typedef struct {
        double lat;
        double lon;
        uint16_t x;
        uint16_t y;
    } Coords;

    typedef struct {
        uint64_t rise;
        uint64_t set;
    } Sun;

    typedef struct {
        int32_t day;
        Sun sun;
    } LabeledSun;

    typedef struct {
        int32_t day;
        uint8_t phase;
    } LabeledLunarPhase;

    typedef struct {
        Coords coords;
        bool isCity;
        LabeledSun* suns;
        size_t sunsLen;
        WxSingle* wx;
        size_t wxLen;
    } Location;
}

struct SummaryData {
    std::string locationName;
    int32_t high = INT32_MIN, low = INT32_MAX, wind = INT32_MIN;
    std::chrono::system_clock::time_point sunrise, sunset;
    double snowTotal = 0.0, iceTotal = 0.0, rainTotal = 0.0;
};

inline double CalcSnowPrecip(const WxSingle* wxCurrent, const WxSingle* wxLast)
{
    return std::max(0.0, wxLast == nullptr ? 0 : wxCurrent->totalSnow - wxLast->totalSnow);
}

inline double CalcPrecipAmount(const WxSingle* wxCurrent, const WxSingle* wxLast)
{
    return wxCurrent->precipitationType == static_cast<uint8_t>(PrecipitationType::Snow)
        ? CalcSnowPrecip(wxCurrent, wxLast) 
        : wxCurrent->newPrecipitation;
}

class ILocation {
public:
    virtual const char* GetId() = 0;
    virtual const Coords& GetCoords() = 0;
    virtual bool IsCity() = 0;
    virtual std::span<LabeledSun> GetSunriseSunsets() = 0;
    virtual const WxSingle& GetForecastAt(uint32_t forecastIndex) = 0;
    virtual uint32_t GetForecastsFromNow(std::chrono::system_clock::time_point& now, uint32_t maxRows, std::function<void (std::chrono::system_clock::time_point&, const WxSingle* wx)> callback) = 0;
    virtual void CollectSummaryData(SummaryData& summaryData, uint32_t maxRows) = 0;
    virtual ~ILocation(){};
};

enum LocationMask {
    Cities = (1 << 0),
    Homes = (1 << 1),
    All = Cities | Homes
};

class IForecast {
public:
    virtual void SetNow(uint64_t now) = 0;
    virtual uint64_t GetNow() = 0;
    virtual uint64_t GetForecastTime(int32_t forecastIndex) = 0;
    virtual std::vector<std::unique_ptr<ILocation>> GetLocations(LocationMask mask) = 0;
    virtual std::vector<std::unique_ptr<ILocation>> GetSortedPlaceLocations() = 0;
    virtual uint32_t GetForecastsFromNow(std::chrono::system_clock::time_point& now, uint32_t maxRows, std::function<void (std::chrono::system_clock::time_point&, uint32_t forecastIndex)> callback) = 0;
    virtual LunarPhase GetLunarPhaseForDay(int32_t day) = 0;
    virtual ~IForecast(){};
};

class IForecastRepo {
public:
    virtual void AddForecastStartTime(std::chrono::system_clock::time_point startTime) = 0;
    virtual void AddLunarPhase(int32_t currentDay, LunarPhase lunarPhase) = 0;
    virtual void AddLocation(const std::string& locationKey, const Location& location) = 0;
    virtual void Save(std::filesystem::path forecastJsonPath) = 0;
    virtual IForecast* GetForecast() = 0;
    virtual ~IForecastRepo(){};
};

IForecastRepo* InitForecastRepo(const std::string& forecastKey);
IForecastRepo* LoadForecastRepo(const std::string& forecastKey, std::filesystem::path forecastJsonPath);