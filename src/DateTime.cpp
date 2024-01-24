#include "DateTime.h"
#include "StringExtension.h"
#include <sstream>

using namespace std;
using namespace chrono;

tm ToUTCTm(std::chrono::system_clock::time_point timePoint)
{
    tm tmAtTimePoint = {0};
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    gmtime_r(&time, &tmAtTimePoint);
    return tmAtTimePoint;
}

tm ToLocalTm(std::chrono::system_clock::time_point timePoint)
{
    tm tmAtTimePoint = {0};
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    localtime_r(&time, &tmAtTimePoint);
    return tmAtTimePoint;
}

string StringifyDate(const char* formatString, std::chrono::system_clock::time_point timePoint)
{
    using namespace chrono;
    char result[32] = {0};
    time_t time = std::chrono::system_clock::to_time_t(timePoint);

    tm tm = {0};
    localtime_r(&time, &tm);
    strftime(result, 32, formatString, &tm);

    return result;
}

string GetShortDayOfWeek(std::chrono::system_clock::time_point timePoint) {
    return StringifyDate("%a", timePoint);
}

string GetShortDateTime(std::chrono::system_clock::time_point timePoint)
{
    return StringifyDate("%b %d %-I %p", timePoint);
}

string GetShortDate(std::chrono::system_clock::time_point timePoint)
{
    return StringifyDate("%b %d", timePoint);
}

string GetHoursAndMinutes(std::chrono::system_clock::time_point timePoint)
{
    return StringifyDate("%-I:%M %p", timePoint);
}

string GetHour(std::chrono::system_clock::time_point timePoint)
{
    return StringifyDate("%-I %p", timePoint);
}

string GetShortHour(std::chrono::system_clock::time_point timePoint)
{
    auto hour = GetHour(timePoint);
    char result[8] = {0};
    int32_t i = 0;
    
    for(auto c : hour)
    {
        if(c == ' ')
            continue;

        if(c == 'M')
            break;

        result[i++] = c;
    }

    return result;
}

std::string GetLongDateTime(std::chrono::system_clock::time_point timePoint)
{
    time_t time = std::chrono::system_clock::to_time_t(timePoint);

    tm tm = {0};
    localtime_r(&time, &tm);

    stringstream dateStream;
    dateStream << StringifyDate("%A %B %-d", timePoint) 
        << NumberSuffix(tm.tm_mday) 
        << ", " 
        << GetHoursAndMinutes(timePoint);
        
    return dateStream.str();
}