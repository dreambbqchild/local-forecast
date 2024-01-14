#pragma once

#include "Grib.h"

#include <string>

class GribDownloader {
private:
    uint16_t maxGribIndex, skipToGribNumber;
    bool usingCachedMode;
    WeatherModel wxModel;
    time_t forecastStartTime;
    std::string filePathTemplate, outputDirectory;

    void Init(void* vSaveData);

public:
    GribDownloader(std::string outputDirectory);
    GribDownloader(std::string outputDirectory, WeatherModel wxmodel, uint16_t maxGribIndex, uint16_t skipToGribNumber = 0);

    std::string GetFilePathTemplate() {return filePathTemplate;}
    uint16_t GetMaxGribIndex() {return maxGribIndex;}
    uint16_t GetSkipToGribNumber() {return skipToGribNumber;}
    WeatherModel GetWeatherModel() {return wxModel;}
    time_t GetForecastStartTime() {return forecastStartTime;}

    void Download();
};