#include "Characters.h"
#include "Data/ForecastJsonExtension.h"
#include "DateTime.h"
#include "Drawing/ForecastImages/ImageBase.h"
#include "Drawing/ForecastImages/MapOverlay.h"
#include "Drawing/ImageCache.h"
#include "Drawing/WxColors.h"
#include "WeatherMaps.h"

#include <iostream>

using namespace std;
namespace fs = std::filesystem;

static const DSRect mapBackgroundRect = {0, 0, 1124, 1164};

class WeatherMaps : public IWeatherMaps
{
private:
    Json::Value& root;
    LocationWeatherData& locationWeatherData;
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

void FinishImage(const char* label, int32_t forecastIndex, unique_ptr<IMapOverlay>& mapOverlay, string path)
{
    const int32_t marginOffset = 32;

    //Setup the Base Map, the Forecast Overlay, and the location forecasts.
    mapOverlay->InterpolateFill(locationWeatherData.GetPointsPerRow());
    auto mapSize = mapOverlay->GetSize();
    auto bgSize = mapBackground->GetSize();
    auto locationDrawService = shared_ptr<IDrawService>(AllocDrawService(mapSize.width, mapSize.height));
    locationDrawService->SetDropShadow({4.0, -4.0}, 0.0);
    for(auto itr = root["locations"].begin(); itr != root["locations"].end(); itr++)
    {
        if(!(*itr)["isCity"].asBool())
            continue;

        auto x = (*itr)["coords"]["x"].asInt();
        auto y = (*itr)["coords"]["y"].asInt();
        auto wx = (*itr)["wx"];
        auto temperature =wx["temperature"][forecastIndex].asInt();

        locationDrawService->DrawText([&](IDrawTextContext* textContext)
        {
            SetupForecastImageTextContext(textContext);
            textContext->AddText(to_string(temperature) + "ºF");
            textContext->AddImage(GetSkyEmojiPathFromWxJson(wx, forecastIndex));
            auto textBounds = textContext->Bounds();

            return DSRect {x - textBounds.width * 0.5, static_cast<double>(y), textBounds.width, textBounds.height};
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
    auto forecastTime = root["forecastTimes"][forecastIndex].asInt64();
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
    WeatherMaps(Json::Value& root, LocationWeatherData& locationWeatherData, GeographicCalcs& geoCalcs)
        : root(root), locationWeatherData(locationWeatherData), geoCalcs(geoCalcs)
    {
        mapBackground = shared_ptr<IImage>(AllocImage(fs::path("media") / string("images") / string("basemap.png")));
    }

   void GenerateForecastMaps(fs::path forecastDataOutputDir)
   {   
        cout << "Rendering Forecast Maps..." << endl;

        this->forecastDataOutputDir = forecastDataOutputDir;

        auto overlayBounds = geoCalcs.Bounds();

        #pragma omp parallel for
        for(auto i : locationWeatherData.GetValidFieldDataIndexes())
        {
            auto forecastIndex = locationWeatherData.GetIndexOfKnownValidFieldIndex(i);
            auto imgSuffix = root["forecastSuffixes"][forecastIndex].asString();
            string temperatureFileName = "temperature-" + imgSuffix + ".png",
                   precipFileName = "precip-" + imgSuffix + ".png";

            auto temperatureImg = unique_ptr<IMapOverlay>(AllocMapOverlay(overlayBounds.width, overlayBounds.height));
            auto precipImg = unique_ptr<IMapOverlay>(AllocMapOverlay(overlayBounds.width, overlayBounds.height));
            for(auto k = 0; k < locationWeatherData.GetGeoCoordsLength(); k++)
            {
                auto& coords = locationWeatherData.GetGeoCoordAtKnownValidIndexes(k);
                auto pt = geoCalcs.FindXY(coords.lat, coords.lon);
                auto& wx = locationWeatherData.GetWxAtKnownValidIndexes(i, k);

                WxColor c = ColorFromDegrees(wx.temperature);
                temperatureImg->SetPixel(pt.x, pt.y, c.r, c.g, c.b, c.a);

                if(wx.type == PrecipitationType::None)
                {
                    precipImg->SetPixel(pt.x, pt.y, 0, 0, 0, 0);
                    continue;
                }

                c = ColorFromPrecipitation(wx.type, ScaledValueForTypeAndTemp(wx.type, wx.precipitationRate, wx.temperature));
                precipImg->SetPixel(pt.x, pt.y, c.r, c.g, c.b, c.a);
            }

            FinishImage("Temperature", forecastIndex, temperatureImg, temperatureFileName);
            FinishImage("Precipitation", forecastIndex, precipImg, precipFileName);
        }
    }

    virtual ~WeatherMaps() = default;
};

IWeatherMaps* AllocWeatherMaps(Json::Value& root, LocationWeatherData& locationWeatherData, GeographicCalcs& geoCalcs)
{
    return new WeatherMaps(root, locationWeatherData, geoCalcs);
}