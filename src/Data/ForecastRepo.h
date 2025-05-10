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

extern "C" {
    void forecast_repo_init_forecast(const char *forecast);
    void forecast_repo_add_forecast_start_time(const char *forecast, uint64_t forecastTime);
    void forecast_repo_add_lunar_phase(const char *forecast, int32_t day, double age, const char *name, const char *emoji);
    void forecast_repo_init_location(const char *forecast, const char *location, bool isCity, const Coords *coords);
    void forecast_repo_add_sun_for_location(const char *forecast, const char *location, int32_t day, const Sun *sun);
    void forecast_repo_add_weather_for_location(const char *forecast, const char *location, const WxSingle *wx, size_t len);
}