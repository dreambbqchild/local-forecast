#pragma once

#include <stdint.h>
#include <cstddef>

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
    double age;
    const char* name;
    const char* emoji;
} Moon;

typedef struct {
    const Coords* coords;
    bool is_city;
    const LabeledSun* suns;
    size_t sunsLen;
    const WxSingle* wx;
    size_t wx_len;
} Location;

extern "C" {
    bool forecast_repo_load_forecast(const char* forecast, const char* path);
    bool forecast_repo_save_forecast(const char* forecast, const char* path);

    void forecast_repo_init_forecast(const char* forecast);
    void forecast_repo_add_forecast_start_time(const char* forecast, uint64_t forecastTime);
    void forecast_repo_add_lunar_phase(const char* forecast, int32_t day, const Moon* moon);
    void forecast_repo_add_location(const char* forecast, const char* locationName, const Location* location);

    size_t forecast_repo_get_forecast_length(const char* forecast);
    uint64_t forecast_repo_get_forecast_time_at(const char* forecast, size_t index);

    typedef void (*MoonCallback)(uint32_t day, Moon moon);
    void forecast_repo_forecast_moon_on_day(const char* forecast, uint32_t day, MoonCallback callback); 

    typedef void (*LocationCallback)(const char* key, Location location);
    void forecast_repo_get_forecast_location(const char* forecast, const char* location, LocationCallback callback); 
}