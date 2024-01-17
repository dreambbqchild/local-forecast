#pragma once
#include "Drawing/DrawService.h"
#include <string>

class IMapOverlay
{
public:
    virtual DSInt32Size GetSize() = 0;
    virtual void SetPixel(double dx, double dy, const DSColor& wxColor) = 0;
    virtual void InterpolateFill(int32_t pointsPerRow) = 0;
    virtual std::shared_ptr<IBitmapContext> GetBitmapContext() = 0;
    virtual void Save(const char* fileName) = 0;
    inline void Save(std::string fileName) { Save(fileName.c_str()); }
    virtual ~IMapOverlay() = default;
};

IMapOverlay* AllocMapOverlay(int32_t width, int32_t height);