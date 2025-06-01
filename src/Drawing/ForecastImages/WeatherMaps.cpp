#include "Characters.h"
#include "Data/ForecastRepo.h"
#include "DateTime.h"
#include "Drawing/ForecastImages/ImageBase.h"
#include "Drawing/ForecastImages/MapOverlay.h"
#include "Drawing/ImageCache.h"
#include "Drawing/WxColors.h"
#include "NumberFormat.h"
#include "WeatherMaps.h"

#include <format>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

static const DSRect mapBackgroundRect = {0, 0, 1124, 1164};

class WeatherMaps : public IWeatherMaps
{
private:
    const unique_ptr<IForecast>& forecast;
    const unique_ptr<GribData>& gribData;
    fs::path forecastDataOutputDir;
    shared_ptr<IImage> mapBackground;
    GeographicCalcs& geoCalcs;

void SetupForecastImageTextContext(IDrawTextContext* textContext)
{
    textContext->SetFontSize(48);
    textContext->SetFontOptions(FontOptions::Bold);
    textContext->SetTextFillColor(PredefinedColors::white);
    textContext->SetTextStrokeColorWithThickness(PredefinedColors::black, 3.0);
}

void FinishImage(const char* label, int32_t forecastIndex, const vector<unique_ptr<ILocation>>& locations, unique_ptr<IMapOverlay>& mapOverlay, string path)
{
    const int32_t marginOffset = 32;

    //Setup the Base Map, the Forecast Overlay, and the location forecasts.
    auto mapSize = mapOverlay->GetSize();
    auto bgSize = mapBackground->GetSize();
    auto locationDrawService = shared_ptr<IDrawService>(AllocDrawService(mapSize.width, mapSize.height));
    locationDrawService->SetDropShadow({4.0, -4.0}, 0.0);
    for(auto& location : locations)
    {
        auto coords = location->GetCoords();
        auto wx = location->GetForecastAt(forecastIndex);        

        locationDrawService->DrawText([&](IDrawTextContext* textContext)
        {
            SetupForecastImageTextContext(textContext);
            textContext->AddText(to_string(wx.temperature) + "ºF");
            textContext->AddImage(Emoji::GetSkyEmojiPath(wx.totalCloudCover, wx.lightning != 0, static_cast<PrecipitationType>(wx.precipitationType), wx.precipitationRate));
            auto textBounds = textContext->Bounds();

            return DSRect {coords.x - textBounds.width * 0.5, static_cast<double>(coords.y), textBounds.width, textBounds.height};
        });
    }  

    auto drawService = unique_ptr<IDrawService>(AllocDrawService(defaultImageWidth, defaultImageHeight));
    auto mapOverlayDraw = shared_ptr<IDrawService>(mapOverlay->GetBitmapContext()->ToDrawService());
    DSRect croppingBounds = { marginOffset, marginOffset, defaultImageWidth, defaultImageHeight };
    DSRect targetBounds = {0, 0, defaultImageWidth, defaultImageHeight};
    drawService->DrawCroppedImage(mapBackground, croppingBounds, targetBounds);
    drawService->DrawCroppedImage(mapOverlayDraw, croppingBounds, targetBounds, 0.75);
    drawService->DrawCroppedImage(locationDrawService, croppingBounds, targetBounds);

    //Add the final text to the rendered image;
    auto forecastTime = forecast->GetForecastTime(forecastIndex);
    drawService->SetDropShadow({4.0, -4.0}, 0.0);    
    drawService->DrawText([&](IDrawTextContext* textContext)
    {
        SetupForecastImageTextContext(textContext);
        textContext->AddText(GetLongDateTime(forecastTime));
        auto textBounds = textContext->Bounds();

        auto renderBounds = CalcRectForCenteredLocation({defaultImageWidth, defaultImageHeight}, textBounds);
        renderBounds.origin.y = defaultImageHeight - textBounds.height - 8;
        return renderBounds;
    });

    drawService->DrawText([&](IDrawTextContext* textContext)
    {
        SetupForecastImageTextContext(textContext);
        textContext->AddText(label);
        auto textBounds = textContext->Bounds();

        auto renderBounds = CalcRectForCenteredLocation({defaultImageWidth, defaultImageHeight}, textBounds);
        renderBounds.origin.y = 8;
        return renderBounds;
    });

    drawService->Save(forecastDataOutputDir / path);
}

public:
    WeatherMaps(const unique_ptr<IForecast>& forecast, const unique_ptr<GribData>& gribData, GeographicCalcs& geoCalcs, const string& mapBackgroundFile)
        : forecast(forecast), gribData(gribData), geoCalcs(geoCalcs)
    {
        mapBackground = shared_ptr<IImage>(AllocImage(fs::path("media") / string("images") / mapBackgroundFile));
    }

   void GenerateForecastMaps(fs::path forecastDataOutputDir)
   {   
        cout << "Rendering Forecast Maps..." << endl;

        this->forecastDataOutputDir = forecastDataOutputDir;

        auto overlayBounds = geoCalcs.Bounds();
        auto locations = forecast->GetLocations(LocationMask::Cities);

        #pragma omp parallel for
        for(auto forecastIndex = 0; forecastIndex < gribData->GetNumberOfFiles(); forecastIndex++)
        {
            auto forecastQuads = gribData->GetQuadIteratorForFileIndex(forecastIndex);
            auto imgSuffix = ToStringWithPad(3, '0', forecastIndex);
            string temperatureFileName = "temperature-" + imgSuffix + ".png",
                   precipFileName = "precip-" + imgSuffix + ".png";

            auto temperatureImg = unique_ptr<IMapOverlay>(AllocMapOverlay(overlayBounds.width, overlayBounds.height));
            auto precipImg = unique_ptr<IMapOverlay>(AllocMapOverlay(overlayBounds.width, overlayBounds.height));
            for(auto& quad : forecastQuads)
            {
                MapOverlayPixel topLeft = {
                    .pt = geoCalcs.FindXY(quad.topLeft.coord),
                    .px = ColorFromDegrees(quad.topLeft.wx.temperature)
                },
                topRight = {
                    .pt = geoCalcs.FindXY(quad.topRight.coord),
                    .px = ColorFromDegrees(quad.topRight.wx.temperature)
                },
                bottomLeft = {
                    .pt = geoCalcs.FindXY(quad.bottomLeft.coord),
                    .px = ColorFromDegrees(quad.bottomLeft.wx.temperature)
                },
                bottomRight = {
                    .pt = geoCalcs.FindXY(quad.bottomRight.coord),
                    .px = ColorFromDegrees(quad.bottomRight.wx.temperature)
                }; 

                temperatureImg->InterpolateFill(topLeft, topRight, bottomLeft, bottomRight);

                #define SET_PRECIP_PX(var) var.px = quad.var.wx.type == PrecipitationType::NoPrecipitation ? PredefinedColors::transparent : ColorFromPrecipitation(quad.var.wx.type, ScaledValueForTypeAndTemp(quad.var.wx.type, quad.var.wx.precipitationRate, quad.var.wx.temperature))
                SET_PRECIP_PX(topLeft);
                SET_PRECIP_PX(topRight);
                SET_PRECIP_PX(bottomLeft);
                SET_PRECIP_PX(bottomRight);
                #undef SET_PRECIP_PX

                precipImg->InterpolateFill(topLeft, topRight, bottomLeft, bottomRight);
            }

            FinishImage("Temperature", forecastIndex, locations, temperatureImg, temperatureFileName);
            FinishImage("Precipitation", forecastIndex, locations, precipImg, precipFileName);
        }
    }

    virtual ~WeatherMaps() = default;
};

IWeatherMaps* AllocWeatherMaps(const unique_ptr<IForecast>& forecast, const unique_ptr<GribData>& gribData, GeographicCalcs& geoCalcs, const string& mapBackgroundFile)
{
    return new WeatherMaps(forecast, gribData, geoCalcs, mapBackgroundFile);
}