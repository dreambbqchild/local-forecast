#pragma once

#include "Data/ForecastRepo.h"
#include "Geography/Geo.h"
#include "Grib.h"
#include "GribData.h"
#include "Wx.h"

#include <chrono>
#include <string>
#include <vector>

class IGribReader
{
public:
    virtual void CollectData(std::unique_ptr<IForecastRepo>& forecastRepo, std::unique_ptr<GribData>& gribData) = 0;
    virtual ~IGribReader() = default;
};

IGribReader* AllocGribReader(std::string gribPathTemplate, const SelectedRegion& selectedRegion, WeatherModel wxModel, std::chrono::system_clock::time_point forecastStartTime, uint16_t skipToGribNumber, uint16_t maxGribIndex, GeographicCalcs& geoCalcs);