#pragma once

#include "Drawing/DrawService.h"
#include "ImageBase.h"
#include <json/json.h>

#include <filesystem>

class PersonalForecasts : public ImageBase {
private:
    Json::Value& root;

public:
    PersonalForecasts(Json::Value& root) : root(root) {}
    void RenderAll(std::filesystem::path forecastDataOutputDir, int32_t maxRows = 24);
};