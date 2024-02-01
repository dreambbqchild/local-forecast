#ifndef LOCAL_FORECAST_LIB_H
#define LOCAL_FORECAST_LIB_H

#include <stdint.h>

enum RenderTargets {
    NoRenderTarget = 0,
    RegionalForecastRenderTarget = (1 << 0),
    PersonalForecastsRenderTarget = (1 << 1),
    WeatherMapsRenderTarget = (1 << 2),
    TextForecastRenderTarget = (1 << 3),
    VideoRenderTarget = (1 << 4),
    AllRenderTargets = RegionalForecastRenderTarget | PersonalForecastsRenderTarget | WeatherMapsRenderTarget | TextForecastRenderTarget | VideoRenderTarget
};

enum WxModel{ NoModel, HRRRWxModel, GFSWxModel };

void LocalForecastLibInit();
void LocalForecastLibRenderRegionalForecast(const char* regionKey, char** pathToPngBuffer);
void LocalForecastLibRenderLocalForecast(const char* regionKey, enum WxModel wxModel, enum RenderTargets renderTargets, uint16_t skipToGribNumber, uint16_t maxGribIndex, char** pathToVideoBuffer, char** pathToTextBuffer);
void LocalForecastLibRenderCahcedLocalForecast(const char* regionKey, enum WxModel wxModel, enum RenderTargets renderTargets, char** pathToVideoBuffer, char** pathToTextBuffer);
#endif