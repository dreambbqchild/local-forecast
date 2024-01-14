#include "DateTime.h"
#include "ForecastJsonExtension.h"

#include <cassert>
#include <fstream>

using namespace std;
using namespace std::chrono;

const char* FindDayPart(string& shortDate)
{
    auto ptr = shortDate.c_str();
    while(*ptr != ' ')
        ptr++;

    ptr++;

    if(*ptr == '0')
        ptr++;

    return ptr;
}

void GetSunriseSunset(Json::Value& sun, std::string& forecastDate, system_clock::time_point& sunrise, system_clock::time_point& sunset)
{
    auto day = FindDayPart(forecastDate);
    auto sunriseTime = sun[day]["rise"].asInt64();
    auto sunsetTime = sun[day]["set"].asInt64();

    assert(sunriseTime != 0);
    assert(sunsetTime != 0);

    sunrise = system_clock::from_time_t(sunriseTime);
    sunset = system_clock::from_time_t(sunsetTime);
}

void GetSortedHomeLocationsFrom(Json::Value& root, vector<Json::Value>& locations)
{
    int32_t index = 0;
    for(auto itr = root["locations"].begin(); itr != root["locations"].end(); itr++)
    {
        if((*itr)["isCity"].asBool())
            continue;

        (*itr)["id"] = itr.name();
        locations.push_back(*itr);
    }

    std::sort(locations.begin(), locations.end(), SortByLongitude);

    for(Json::Value& itr : locations)
        itr["index"] = index++;
}

int32_t GetForecastsFromNow(Json::Value& root, std::chrono::system_clock::time_point& now, int32_t maxRows, function<void (std::chrono::system_clock::time_point&, int32_t)> callback)
{
    int32_t rowsRendered = 0, 
            forecastIndex = 0;

    auto start = root["forecastTimes"].begin();
    auto end = root["forecastTimes"].end();
    for(auto itr = start; itr != end && rowsRendered < maxRows; itr++, forecastIndex++)
    {
        auto forecastTime = system_clock::from_time_t(itr->asInt64());
        if(forecastTime + minutes(59) + seconds(59) < now)
            continue;

        callback(forecastTime, forecastIndex);

        rowsRendered++;
    }

    return rowsRendered;
}

void CollectSummaryDataForLocation(const string& locationName, SummaryData& summaryData, Json::Value& root, Json::Value& location, int32_t maxRows)
{
    string forecastDate;
    auto now = system_clock::from_time_t(root["now"].asInt64());
    summaryData.locationName = locationName;

    GetForecastsFromNow(root, now, maxRows, [&](system_clock::time_point& forecastTime, int32_t forecastIndex)
    {
        auto currentDate = GetShortDate(forecastTime);        
        if(currentDate != forecastDate)
        {
            forecastDate = currentDate;

            if(summaryData.sunrise < now || summaryData.sunset < now)
            {
                system_clock::time_point localSunrise, localSunset;
                GetSunriseSunset(location["sun"], forecastDate, localSunrise, localSunset);

                if(summaryData.sunrise < now)
                    summaryData.sunrise = localSunrise;
                
                if(summaryData.sunset < now)
                    summaryData.sunset = localSunset;
            }
        }

        auto wx = location["wx"];
        auto temperature = wx["temperature"][forecastIndex].asInt();
        auto windSpd = wx["windSpd"][forecastIndex].asInt();
        summaryData.high = max(summaryData.high, temperature);
        summaryData.low = min(summaryData.low, temperature);
        summaryData.wind = max(summaryData.wind, windSpd);
        
        auto precipType = GetPrecipType(wx, forecastIndex);
        if(precipType == "snow")
            summaryData.snowTotal += CalcSnowPrecip(wx, forecastIndex);
        else if(precipType == "ice")
            summaryData.iceTotal += CalcWaterPrecip(wx, forecastIndex);
        else if(precipType == "rain")
            summaryData.rainTotal += CalcWaterPrecip(wx, forecastIndex);
    });
}