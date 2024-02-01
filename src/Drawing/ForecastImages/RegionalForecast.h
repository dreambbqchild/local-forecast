#pragma once

#include "Data/SelectedRegion.h"
#include "Drawing/DrawService.h"
#include "ImageBase.h"

#include <filesystem>
#include <json/json.h>

struct ForecastBase;

class RegionalForecast : public ImageBase {
private:
    static const uint32_t borderGap = 8;
    static const uint32_t renderAreaWidth = totalWidth - (borderGap * 2), renderAreaHeight = totalHeight - (borderGap * 2);
    static const uint32_t columnWidth = renderAreaWidth / 2;

    static std::function<DSRect(IDrawTextContext*)> AddLabel(DSColor color, std::string& text, double offsetLeft, double offsetTop);
    static std::function<DSRect(IDrawTextContext*)> RenderForecast(IDrawService* draw, double offsetLeft, double offsetTop, ForecastBase* forecast);

    static Json::Value LoadForecast(const SelectedRegion& selectedRegion, const std::filesystem::path& pathToJson);
    static double DrawBoxHeaderLabels(IDrawService* draw, double top, std::string& labelLeft, DSColor colorLeft, std::string& labelRight, DSColor colorRight);
    static double DrawForecastBoxes(IDrawService* draw, double top, ForecastBase* forecastLeft, ForecastBase* forecastRight);
    
public:
    void Render(const SelectedRegion& selectedRegion, const std::filesystem::path& pathToJson, const std::filesystem::path& pathToPng);
};