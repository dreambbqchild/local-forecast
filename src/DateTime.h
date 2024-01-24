#pragma once

#include <chrono>
#include <string>

const uint16_t secondsInHour = 60 * 60;
const uint32_t secondsInDay = secondsInHour * 24;

inline time_t mkgmtime(tm* tm) { return mktime(tm) - timezone; }

inline int32_t GetHourFromTime(time_t t) {return (t / secondsInHour) % 24;}

#define ToYearMonthDay(t, yearVar, monthVar, dayVar) std::chrono::year yearVar(t.tm_year + 1900); auto monthVar = std::chrono::month(t.tm_mon + 1); auto dayVar = std::chrono::day(t.tm_mday);

extern tm ToLocalTm(std::chrono::system_clock::time_point timePoint);
extern tm ToUTCTm(std::chrono::system_clock::time_point timePoint);

extern std::string GetShortDayOfWeek(std::chrono::system_clock::time_point timePoint);
inline std::string GetShortDayOfWeek(time_t time){ return GetShortDayOfWeek(std::chrono::system_clock::from_time_t(time)); }

extern std::string GetShortDateTime(std::chrono::system_clock::time_point timePoint);
inline std::string GetShortDateTime(time_t time){ return GetShortDateTime(std::chrono::system_clock::from_time_t(time)); }

extern std::string GetShortDate(std::chrono::system_clock::time_point timePoint);
inline std::string GetShortDate(time_t time){ return GetShortDate(std::chrono::system_clock::from_time_t(time)); }

extern std::string GetHoursAndMinutes(std::chrono::system_clock::time_point timePoint);
inline std::string GetHoursAndMinutes(time_t time){ return GetHoursAndMinutes(std::chrono::system_clock::from_time_t(time)); }

extern std::string GetHour(std::chrono::system_clock::time_point timePoint);
inline std::string GetHour(time_t time){ return GetHour(std::chrono::system_clock::from_time_t(time)); }

extern std::string GetShortHour(std::chrono::system_clock::time_point timePoint);
inline std::string GetShortHour(time_t time){ return GetShortHour(std::chrono::system_clock::from_time_t(time)); }

extern std::string GetLongDateTime(std::chrono::system_clock::time_point timePoint);
inline std::string GetLongDateTime(time_t time){ return GetLongDateTime(std::chrono::system_clock::from_time_t(time)); }