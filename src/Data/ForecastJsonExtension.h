#pragma once

#include <json/json.h>

#include <chrono>
#include <filesystem>
#include <functional>

struct SummaryData {
    std::string locationName;
    int32_t high = INT32_MIN, low = INT32_MAX, wind = INT32_MIN;
    std::chrono::system_clock::time_point sunrise, sunset;
    double snowTotal = 0.0, iceTotal = 0.0, rainTotal = 0.0;
};

void LoadForecastJson(std::filesystem::path forecastPath, Json::Value& root);
void GetSunriseSunset(Json::Value& sun, std::string& forecastDate, std::chrono::system_clock::time_point& sunrise, std::chrono::system_clock::time_point& sunset);
void GetSunriseSunset(Json::Value& sun, std::string& forecastDate, tm& sunrise, tm& sunset);
void GetSortedPlaceLocationsFrom(Json::Value& root, std::vector<Json::Value>& locations);
int32_t GetForecastsFromNow(Json::Value& root, std::chrono::system_clock::time_point& now, int32_t maxRows, std::function<void (std::chrono::system_clock::time_point&, int32_t)> callback);
void CollectSummaryDataForLocation(const std::string& locationName, SummaryData& summaryData, Json::Value& root, Json::Value& location, int32_t maxRows);

inline bool SortByLongitude(Json::Value& l, Json::Value& r)
{
    return l["coords"]["x"].asDouble() - r["coords"]["x"].asDouble() < 0;
}

inline std::string GetPrecipType(Json::Value& wx, int32_t forecastIndex)
{
    return wx["precipType"][forecastIndex].asString();
}

inline double CalcSnowPrecip(Json::Value& wx, int32_t forecastIndex)
{
    return std::max(0.0, forecastIndex == 0 ? 0 : wx["totalSnow"][forecastIndex].asDouble() - wx["totalSnow"][forecastIndex - 1].asDouble());
}

inline double CalcWaterPrecip(Json::Value& wx, int32_t forecastIndex)
{
    return wx["newPrecip"][forecastIndex].asDouble();
}

inline double CalcPrecipAmount(Json::Value& wx, int32_t forecastIndex)
{
    return GetPrecipType(wx, forecastIndex) == "snow" 
        ? CalcSnowPrecip(wx, forecastIndex) 
        : CalcWaterPrecip(wx, forecastIndex);
}