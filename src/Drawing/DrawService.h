#ifndef DRAWSERVICE_H
#define DRAWSERVICE_H
#include "Calcs.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

typedef DoublePoint DSPoint;
typedef PointTemplate<uint16_t> DSUInt16Point;

typedef SizeTemplate<double> DSSize;
typedef SizeTemplate<int32_t> DSInt32Size;

struct DSRect 
{
    DSPoint origin;
    DSSize size;
};

struct DSColor
{
    uint8_t r = 0, 
    g = 0, 
    b = 0, 
    a = 255;
};

namespace PredefinedColors
{
    const DSColor white = {255, 255, 255},
        black = {0, 0, 0},
        yellow = {255, 255, 0},
        red = {255, 0, 0},
        orange = {255, 165, 0},
        cyan = {0, 255, 255},
        discordBg = {53, 56, 62};
}

enum FontOptions
{
    Regular = 0,
    Bold = (1 << 0),
    Italic = (1 << 1),
    BoldItalic = Bold | Italic
};

class IDrawTextContext {    
public:
    virtual void SetFontSize(double fontSize) = 0;
    virtual void SetLineSpacing(double lineSpacing) = 0;
    virtual void SetTextFillColor(DSColor color) = 0;
    virtual void SetTextStrokeColorWithThickness(DSColor color, double thickness) = 0;
    virtual void SetTextShadow(DSSize offset, double blur) = 0;
    virtual void SetFontOptions(FontOptions options) = 0;
    virtual void AddText(const char* text) = 0;
    inline void AddText(std::string text) { AddText(text.c_str()); };
    virtual void AddImage(const char* path) = 0;
    inline void AddImage(std::string text) { AddImage(text.c_str()); };
    virtual void AddImage(const char* path, DSRect bounds) = 0;
    inline void AddImage(std::string path, DSRect bounds) { AddImage(path.c_str(), bounds); };
    virtual DSSize Bounds() = 0;
    virtual ~IDrawTextContext() = default;
};

class IImage;

class IDrawService {
public:
    virtual DSSize GetSize() = 0;
    virtual void ClearCanvas(DSColor color) = 0;
    virtual void SetDropShadow(DSSize offset, double blur) = 0;
    virtual void ClearDropShadow() = 0;
    virtual void DrawImage(const char* pathToImageFile, DSRect drawAtWithinBounds, double alpha = 1.0) = 0;
    inline void DrawImage(std::string pathToImageFile, DSRect drawAtWithinBounds, double alpha = 1.0) { DrawImage(pathToImageFile.c_str(), drawAtWithinBounds, alpha); }
    virtual void DrawCroppedImage(std::shared_ptr<IDrawService> drawService, DSRect sourceImageBounds, DSRect drawAtWithinBounds, double alpha = 1.0) = 0;
    virtual void DrawCroppedImage(std::shared_ptr<IImage> image, DSRect sourceImageBounds, DSRect drawAtWithinBounds, double alpha = 1.0) = 0;
    virtual void DrawCroppedImage(const char* pathToImageFile, DSRect sourceImageBounds, DSRect drawAtWithinBounds, double alpha = 1.0) = 0;
    inline void DrawCroppedImage(std::string pathToImageFile, DSRect sourceImageBounds, DSRect drawAtWithinBounds, double alpha = 1.0) { DrawCroppedImage(pathToImageFile.c_str(), sourceImageBounds, drawAtWithinBounds, alpha); }
    virtual DSRect DrawText(std::function<DSRect(IDrawTextContext*)> callback) = 0;
    virtual void MoveTo(DSPoint pt) = 0;
    virtual void LineTo(DSPoint pt) = 0;
    virtual void AddRoundedRectangle(DSRect bounds, double cx, double cy) = 0;
    virtual void AddRectangle(DSRect bounds) = 0;
    virtual void SetFillColor(DSColor color) = 0;
    virtual void SetStrokeColor(DSColor color) = 0;
    virtual void FillActivePath() = 0;
    virtual void StrokeActivePath(double thickness) = 0;
    virtual void Save(const char* outfilePath) = 0;
    inline void Save(std::string outfilePath) { Save(outfilePath.c_str()); }
    virtual ~IDrawService() = default;
};

IDrawService* AllocDrawService(int32_t width, int32_t height);

class IBitmapContext {
public:
    virtual void SetPixel(DSUInt16Point pt, DSColor color) = 0;
    virtual DSColor GetPixel(DSUInt16Point pt) = 0;
    virtual uint8_t* GetPointer(DSUInt16Point pt) = 0;
    virtual void Save(const char* outfilePath) = 0;
    virtual IDrawService* ToDrawService() = 0;
    virtual ~IBitmapContext() = default;
};

extern IBitmapContext* AllocBitmapContext(int32_t width, int32_t height);

//https://www.w3.org/TR/WCAG20-TECHS/G17.html#G17-procedure
inline double RGBLuminance(const DSColor& color)
{
    double rgb[3] = {color.r / 255.0, color.g / 255.0, color.b / 255.0};
    for(auto i = 0; i < 3; i++)
        rgb[i] = rgb[i] <= 0.03928 ? rgb[i] / 12.92 : pow((rgb[i] + 0.055) / 1.055, 2.4);

    return 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2];
}

inline double ContrastRatio(const DSColor& color1, const DSColor& color2)
{
    auto lum1 = RGBLuminance(color1);
    auto lum2 = RGBLuminance(color2);
    auto lighter = std::max(lum1, lum2);
    auto darker = std::min(lum1, lum2);

    return (lighter + 0.05) / (darker + 0.05);
}

DSRect CalcRectForCenteredLocation(DSSize area, DSSize rendering, DSPoint offset = {0});

#endif