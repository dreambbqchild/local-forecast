#include "Astronomy.h"
#include "Calcs.h"
#include "DateTime.h"
#include <sunset.h>
#include <stdio.h>

#include <sstream>

using namespace std;
using namespace chrono;

struct FullMoon {
    const char* emoji, 
              * name;
};

static constexpr days LUNAR_MONTH_FULL_DAYS{29}; 
static constexpr hours LUNAR_MONTH_FULL_HOURS{12};
static constexpr minutes LUNAR_MONTH_FULL_MINUTES{44};
static constexpr seconds LUNAR_MONTH_FULL_SECONDS{3};
static constexpr seconds LUNAR_MONTH_TIMEPOINT = LUNAR_MONTH_FULL_DAYS + LUNAR_MONTH_FULL_HOURS + LUNAR_MONTH_FULL_MINUTES + LUNAR_MONTH_FULL_SECONDS;

static const double LUNAR_MONTH = 29.530588853;
static const double RECENT_NEW_MOON = 2460261.893750; //November 13 2023 @ 9:27;

static const char* NEW_LABEL = "New",
  * WAXING_CRESCENT_LABEL = "Waxing Crescent",
  * FIRST_QUARTER_LABEL = "First Quarter",
  * WAXING_GIBBOUS_LABEL = "Waxing Gibbous",
  * FULL_LABEL = "Full",
  * WANING_GIBBOUS_LABEL = "Waning Gibbous",
  * LAST_QUARTER_LABEL = "Last Quarter",
  * WANING_CRESCENT_LABEL = "Waning Crescent";

//For Northern Hemisphere
static const char* NEW_EMOJI = "🌑",
    * WAXING_CRESCENT_EMOJI = "🌒",
    * FIRST_QUARTER_EMOJI = "🌓",
    * WAXING_GIBBOUS_EMOJI = "🌔",
    * FULL_EMOJI = "🌕",
    * WANING_GIBBOUS_EMOJI = "🌖",
    * LAST_QUARTER_EMOJI = "🌗",
    * WANING_CRESCENT_EMOJI = "🌘";

static FullMoon FULL_MOONS[] = {
    {  "🐺", "Wolf" },
    {  "❄️", "Snow" },
    {  "🪱", "Worm" },
    {  "🌸", "Pink" },
    {  "🌼", "Flower" },
    {  "🍓", "Strawberry" },
    {  "🦌", "Buck" },
    {  "🐟", "Sturgeon" },
    {  "🌽", "Corn" },
    {  "🏹", "Hunter" },
    {  "🦫", "Beaver" },
    {  "🥶", "Cold" }
};

void GetHourMinute(double value, tm& tm)
{
    int32_t iValue = static_cast<int32_t>(value);
    tm.tm_hour = iValue / 60;
    tm.tm_min = iValue % 60;
    tm.tm_sec = 0;
} 

SunriseSunset Astronomy::GetSunRiseSunset(double lat, double lon, system_clock::time_point timePoint) 
{
    SunSet sun;
    SunriseSunset result;

    if(lon > 180)
        lon = lon - 360;
    
    tm localTime = ToLocalTm(timePoint), rise = {0}, set = {0};

    sun.setPosition(lat, lon, (int32_t)(localTime.tm_gmtoff / 3600));
    sun.setCurrentDate(localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday);
    
    rise = set = localTime;

    GetHourMinute(sun.calcSunrise(), rise);
    GetHourMinute(sun.calcSunset(), set);

    result.rise = system_clock::from_time_t(mktime(&rise));
    result.set = system_clock::from_time_t(mktime(&set));

    return result;
}

inline double Normalize(double value) 
{
    value -= floor(value);
    if (value < 0) 
        value += 1;

    return value;
};

double CalcLunarAge(const system_clock::time_point& timePoint)
{
    using namespace chrono_literals;
    using doubleDays = duration<double, days::period>;
    
    const auto julianStart = sys_days{November/24/-4713} + 12h;

    auto currentJulianDay = duration_cast<doubleDays>(timePoint - julianStart).count();
    auto lunarAge = Normalize((currentJulianDay - RECENT_NEW_MOON) / LUNAR_MONTH);
    return ((lunarAge < 0) ? lunarAge + 1 : lunarAge) * LUNAR_MONTH;
}

inline double CalcLunarAgePercent(double lunarAge) {return Normalize(lunarAge / LUNAR_MONTH); }

inline bool IsFullLunarAge(double age) { return age >= 12.91963212127 && age < 16.61095558449; }

bool IsKnownHarvestMoon(year utcYear, month utcMonth, day utcDay)
{
    auto m = static_cast<unsigned>(utcMonth);
    if(m != 9 && m != 10)
        return false;

    #define CheckHarvestMoon(y, m, d) if(utcYear == year(y) && utcMonth == month(m) && utcDay == day(d)) return true;

    CheckHarvestMoon(2024, 9, 18)
    CheckHarvestMoon(2025, 10, 6)
    CheckHarvestMoon(2026, 9, 10)
    CheckHarvestMoon(2027, 9, 15)
    CheckHarvestMoon(2028, 10, 3)
    CheckHarvestMoon(2029, 9, 22)
    CheckHarvestMoon(2030, 9, 11)

    return false;
    #undef CheckHarvestMoon
}

bool IsBlueMoon(year utcYear, month utcMonth)
{
    auto rootDay = sys_days{utcMonth/1/utcYear};
    int8_t newMoons = 0;

    for(auto d = 0; d < 4; d++)
    {
        if(d == 3)
            return false;

        if(IsFullLunarAge(CalcLunarAge(rootDay + days{d})))
        {
            rootDay += days{d};
            break;
        }
    }

    auto firstFull = ToUTCTm(rootDay);
    auto secondFull = ToUTCTm(rootDay + LUNAR_MONTH_TIMEPOINT);
    return firstFull.tm_mon == secondFull.tm_mon;
}

LunarPhase FullMoonDetails(double lunarAge, year utcYear, month utcMonth, day utcDay)
{
    auto fullMoon = FULL_MOONS[static_cast<unsigned>(utcMonth) - 1];

    stringstream nameBuilder;
    if(IsBlueMoon(utcYear, utcMonth))
        nameBuilder << "Blue";
    else if(IsKnownHarvestMoon(utcYear, utcMonth, utcDay))
        nameBuilder << "Harvest";
    else
        nameBuilder << FULL_LABEL << " " << fullMoon.name;
    
    return {lunarAge, nameBuilder.str(), fullMoon.emoji + string(FULL_EMOJI)};
}

LunarPhase Astronomy::GetLunarPhase(system_clock::time_point timePoint)
{
    auto cTime = std::chrono::system_clock::to_time_t(timePoint);
    auto cTimeOffset = cTime % secondsInDay; //Set to start of the UTC Day.

    cTime -= cTimeOffset;

    auto utcTimePoint = system_clock::from_time_t(cTime);
    auto lunarAge = CalcLunarAge(utcTimePoint);
    auto agePercent = CalcLunarAgePercent(lunarAge);
    auto nextAgePercent = CalcLunarAgePercent(CalcLunarAge(utcTimePoint + days(1)));

    if(agePercent > 0.9 && nextAgePercent < 0.1)
        return { lunarAge, NEW_LABEL, NEW_EMOJI };
    else if (IsBetween(agePercent, 0.75, nextAgePercent))
        return { lunarAge, LAST_QUARTER_LABEL, LAST_QUARTER_EMOJI };
    else if (IsBetween(agePercent, 0.5, nextAgePercent))
    {
        auto time = ToUTCTm(utcTimePoint);
        ToYearMonthDay(time, utcYear, utcMonth, utcDay)
        return FullMoonDetails(lunarAge, utcYear, utcMonth, utcDay);
    }
    else if (IsBetween(agePercent, 0.25, nextAgePercent))
        return { lunarAge, FIRST_QUARTER_LABEL, FIRST_QUARTER_EMOJI };
    
    if (agePercent < 0.25) return { lunarAge, WAXING_CRESCENT_LABEL, WAXING_CRESCENT_EMOJI };
    else if (agePercent < 0.5) return { lunarAge, WAXING_GIBBOUS_LABEL, WAXING_GIBBOUS_EMOJI };
    else if (agePercent < 0.75) return { lunarAge, WANING_GIBBOUS_LABEL, WANING_GIBBOUS_EMOJI };
    
    return { lunarAge, WANING_CRESCENT_LABEL, WANING_CRESCENT_EMOJI };
}