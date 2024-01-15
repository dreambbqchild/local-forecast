#ifndef LOCAL_FORECAST_LIB
#define LOCAL_FORECAST_LIB

#include <stdint.h>

enum RenderTargets {
    NoRenderTarget = 0,
    RegionalForecastRenderTarget = (1 << 0),
    PersonalForecastsRenderTarget = (1 << 1),
    WeatherMapsRenderTarget = (1 << 2),
    TextForecastRenderTarget = (1 << 3),
    VideoRenderTarget = (1 << 4),
    All = RegionalForecastRenderTarget | PersonalForecastsRenderTarget | WeatherMapsRenderTarget | TextForecastRenderTarget | VideoRenderTarget
};

enum WxModel{ HRRRWxModel, GFSWxModel };

void LocalForecastLibInit();
void LocalForecastLibRenderRegionalForecast();
void LocalForecastLibRenderLocalForecast(enum WxModel wxModel, const char* gribFilePath, const char* forecastFilePath, enum RenderTargets renderTargets, uint16_t skipToGribNumber, uint16_t maxGribIndex);
void LocalForecastLibRenderCahcedLocalForecast(const char* gribFilePath, const char* forecastFilePath, enum RenderTargets renderTargets);
#endif