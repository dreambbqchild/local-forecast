#pragma once

#include <stdint.h>
#include <cstddef>

enum MoonPhase : uint8_t {
    New,
    WaxingCrescent,
    FirstQuarter,
    WaxingGibbous,
    Full,
    WaningGibbous,
    LastQuarter,
    WaningCrescent
};

typedef struct {
    int16_t dewpoint;
    int16_t gust;
    int16_t lightning;
    double new_precip;
    double precip_rate;
    uint8_t precip_type;
    double pressure;
    int16_t temperature;
    int16_t total_cloud_cover;
    double total_precip;
    double total_snow;
    int16_t vis;
    int16_t wind_dir;
    int16_t wind_spd;
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
    uint8_t phase;
} Moon;

typedef struct {
    int32_t day;
    Moon moon;
} LabeledMoon;

typedef struct {
    Coords coords;
    bool is_city;
    const LabeledSun* suns;
    size_t sunsLen;
    const WxSingle* wx;
    size_t wx_len;
} LocationRust;

typedef struct {
    const char* key;
    LocationRust location;
} LabeledLocation;

typedef struct Forecast {
    uint64_t forecast_times;
    size_t forecast_times_len;
    LabeledLocation* locations;
    size_t locations_len;
    LabeledMoon* moons;
    size_t moons_len;
} Forecast;

extern "C" {
    bool forecast_repo_load_forecast(const char* forecast, const char* path);
    bool forecast_repo_save_forecast(const char* forecast, const char* path);

    void forecast_repo_init_forecast(const char* forecast, const uint64_t* forecastTimes, size_t forecastTimeLen);
    void forecast_repo_add_lunar_phase(const char* forecast, int32_t day, const Moon* moon);
    void forecast_repo_add_location(const char* forecast, const char* locationName, const LocationRust* location);

    Forecast* forecast_repo_get_forecast(const char* forecast);
    void forecast_repo_free_forecast(Forecast* forecast);
}