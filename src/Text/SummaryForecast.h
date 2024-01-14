#pragma once

#include <filesystem>

class ISummaryForecast
{
    public:
    virtual void Render(std::filesystem::path forecastDataOutputDir, Json::Value& root, int32_t maxRows = 24) = 0;
    virtual ~ISummaryForecast() = default;
};

ISummaryForecast* AllocSummaryForecast();