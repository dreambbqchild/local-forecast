#pragma once

#include "Grib.h"
#include "Data/SelectedLocation.h"

#include <string>

class GribDownloader {
private:
    uint16_t maxGribIndex, skipToGribNumber;
    bool usingCachedMode;
    WeatherModel weatherModel;
    time_t forecastStartTime;
    std::string filePathTemplate, outputDirectory;
    const SelectedLocation& selectedLocation;

    void Init(void* vSaveData);

public:
    GribDownloader(const SelectedLocation& selectedLocation, std::string outputDirectory);
    GribDownloader(const SelectedLocation& selectedLocation, std::string outputDirectory, WeatherModel weatherModel, uint16_t maxGribIndex, uint16_t skipToGribNumber = 0);

    std::string GetFilePathTemplate() {return filePathTemplate;}
    uint16_t GetMaxGribIndex() {return maxGribIndex;}
    uint16_t GetSkipToGribNumber() {return skipToGribNumber;}
    WeatherModel GetWeatherModel() {return weatherModel;}
    time_t GetForecastStartTime() {return forecastStartTime;}

    void Download();
};