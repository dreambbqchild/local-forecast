#pragma once

#include "Data/ForecastRepo.h"
#include "Drawing/DrawService.h"
#include "ImageBase.h"

#include <filesystem>

class PersonalForecasts : public ImageBase {
private:
    IForecast* forecast;

public:
    PersonalForecasts(IForecast* forecast) : forecast(forecast) {}
    void RenderAll(std::filesystem::path forecastDataOutputDir, int32_t maxRows = 24);
};