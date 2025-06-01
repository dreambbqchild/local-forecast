#pragma once

#include "Data/ForecastRepo.h"

#include <filesystem>

class ISummaryForecast
{
public:
    virtual void Render(std::filesystem::path textForecastOutputPath, const std::unique_ptr<IForecast>& forecast, int32_t maxRows = 24) = 0;
    virtual ~ISummaryForecast() = default;
};

ISummaryForecast* AllocSummaryForecast();