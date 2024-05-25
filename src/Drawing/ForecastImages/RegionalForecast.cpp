#include "Characters.h"
#include "Error.h"
#include "RegionalForecast.h"
#include "HttpClient.h"
#include "DateTime.h"
#include "Drawing/WxColors.h"
#include "NumberFormat.h"
#include <string.h>

#include <chrono>
#include <fstream>

using namespace std;
using namespace PredefinedColors;
namespace fs = std::filesystem;

namespace Labels {
    const char * now = "Now",
        * notable = "Most Notable",
        * high = "High",
        * low = "Low";
};

struct DownloadStreams {
    stringstream* memory;
    ofstream* outFile;
};

struct Description {
    string lines[2];
};

struct ForecastBase {
    bool isForecastForHour;

    string label;

    time_t time;

    int32_t code, 
        windDir,
        windVelocity,
        windGust,
        precipProbability;

    Description description;
};

struct ForecastForHour : ForecastBase {
    int32_t temperature,
        visibility;
};

struct ForecastForDay : ForecastBase {
    int32_t highTemperature,
        lowTemperature;
};

struct ForecastItems {
    ForecastForHour now, notable, high, low;
    ForecastForDay tomorrow, nextDay;
};

const char* LookupEmoji(int32_t code)
{
    switch(code)
    {
        case 200: return Emoji::cloud_with_lightning_and_rain; //thunderstorm with light rain
        case 201: return Emoji::cloud_with_lightning_and_rain; //thunderstorm with rain
        case 202: return Emoji::cloud_with_lightning_and_rain; //thunderstorm with heavy rain
        case 210: return Emoji::cloud_with_lightning_and_rain; //light thunderstorm
        case 211: return Emoji::high_voltage; //thunderstorm
        case 212: return Emoji::high_voltage; //heavy thunderstorm
        case 221: return Emoji::high_voltage; //ragged thunderstorm
        case 230: return Emoji::cloud_with_lightning_and_rain; //thunderstorm with light drizzle
        case 231: return Emoji::cloud_with_lightning_and_rain; //thunderstorm with drizzle
        case 232: return Emoji::cloud_with_lightning_and_rain; //thunderstorm with heavy drizzle
        case 300: return Emoji::cloud_with_rain; //light intensity drizzle
        case 301: return Emoji::cloud_with_rain; //drizzle
        case 302: return Emoji::cloud_with_rain; //heavy intensity drizzle
        case 310: return Emoji::cloud_with_rain; //light intensity drizzle rain
        case 311: return Emoji::cloud_with_rain; //drizzle rain
        case 312: return Emoji::cloud_with_rain; //heavy intensity drizzle rain
        case 313: return Emoji::cloud_with_rain; //shower rain and drizzle
        case 314: return Emoji::cloud_with_rain; //heavy shower rain and drizzle
        case 321: return Emoji::cloud_with_rain; //shower drizzle
        case 500: return Emoji::cloud_with_rain; //light rain
        case 501: return Emoji::cloud_with_rain; //moderate rain
        case 502: return Emoji::cloud_with_rain; //heavy intensity rain
        case 503: return Emoji::cloud_with_rain; //very heavy rain
        case 504: return Emoji::cloud_with_rain; //extreme rain
        case 511: return Emoji::ice; //freezing rain
        case 520: return Emoji::cloud_with_rain; //light intensity shower rain
        case 521: return Emoji::cloud_with_rain; //shower rain
        case 522: return Emoji::cloud_with_rain; //heavy intensity shower rain
        case 531: return Emoji::cloud_with_rain; //ragged shower rain
        case 600: return Emoji::cloud_with_snow; //light snow
        case 601: return Emoji::cloud_with_snow; //snow
        case 602: return Emoji::snowflake; //heavy snow
        case 611: return Emoji::ice; //sleet
        case 612: return Emoji::ice; //light shower sleet
        case 613: return Emoji::ice; //shower sleet
        case 615: return Emoji::ice; //light rain and snow
        case 616: return Emoji::ice; //rain and snow
        case 620: return Emoji::cloud_with_snow; //light shower snow
        case 621: return Emoji::cloud_with_snow; //shower snow
        case 622: return Emoji::cloud_with_snow; //heavy shower snow
        case 701: return Emoji::foggy; //mist
        case 711: return Emoji::foggy; //smoke
        case 721: return Emoji::foggy; //haze
        case 731: return Emoji::foggy; //sand/dust whirls
        case 741: return Emoji::fog; //fog
        case 751: return Emoji::foggy; //sand
        case 761: return Emoji::foggy; //dust
        case 762: return Emoji::foggy; //volcanic ash
        case 771: return Emoji::red_question_mark; //squalls
        case 781: return Emoji::red_question_mark; //tornado
        case 800: return Emoji::sun_with_face; //clear sky
        case 801: return Emoji::sun; //few clouds: 11-25%
        case 802: return Emoji::sun_behind_small_cloud; //scattered clouds: 25-50%
        case 803: return Emoji::sun_behind_large_cloud; //broken clouds: 51-84%
        case 804: return Emoji::cloud; //overcast clouds: 85-100%
        default: return Emoji::red_question_mark;
    }
}

int32_t InterestingRank(int32_t code)
{
    switch ((code))
    {
        case 200: return 32; //thunderstorm with light rain
        case 201: return 33; //thunderstorm with rain
        case 202: return 34; //thunderstorm with heavy rain
        case 210: return 35; //light thunderstorm
        case 211: return 36; //thunderstorm
        case 212: return 37; //heavy thunderstorm
        case 221: return 38; //ragged thunderstorm
        case 230: return 29; //thunderstorm with light drizzle
        case 231: return 30; //thunderstorm with drizzle
        case 232: return 31; //thunderstorm with heavy drizzle
        case 300: return 11; //light intensity drizzle
        case 301: return 12; //drizzle
        case 302: return 13; //heavy intensity drizzle
        case 310: return 14; //light intensity drizzle rain
        case 311: return 15; //drizzle rain
        case 312: return 24; //heavy intensity drizzle rain
        case 313: return 18; //shower rain and drizzle
        case 314: return 25; //heavy shower rain and drizzle
        case 321: return 16; //shower drizzle
        case 500: return 17; //light rain
        case 501: return 19; //moderate rain
        case 502: return 26; //heavy intensity rain
        case 503: return 27; //very heavy rain
        case 504: return 28; //extreme rain
        case 511: return 39; //freezing rain
        case 520: return 20; //light intensity shower rain
        case 521: return 21; //shower rain
        case 522: return 22; //heavy intensity shower rain
        case 531: return 23; //ragged shower rain
        case 600: return 48; //light snow
        case 601: return 49; //snow
        case 602: return 50; //heavy snow
        case 611: return 42; //sleet
        case 612: return 40; //light shower sleet
        case 613: return 41; //shower sleet
        case 615: return 43; //light rain and snow
        case 616: return 44; //rain and snow
        case 620: return 45; //light shower snow
        case 621: return 46; //shower snow
        case 622: return 47; //heavy shower snow
        case 701: return 5; //mist
        case 711: return 10; //smoke
        case 721: return 5; //haze
        case 731: return 9; //sand/dust whirls
        case 741: return 6; //fog
        case 751: return 7; //sand
        case 761: return 8; //dust
        case 762: return 998; //volcanic ash
        case 771: return 999; //squalls
        case 781: return 1000; //tornado
        case 800: return 0; //clear sky
        case 801: return 1; //few clouds: 11-25%
        case 802: return 2; //scattered clouds: 25-50%
        case 803: return 3; //broken clouds: 51-84%
        case 804: return 4; //overcast clouds: 85-100%
        default: return -1;
    }
}

Description DescriptionFromCode(int32_t code)
{
    switch (code)
    {
        case 200: return {"Thunderstorm with", "Light Rain"};
        case 201: return {"Thunderstorm with", "Rain"};
        case 202: return {"Thunderstorm with", "Heavy Rain"};
        case 210: return {"Light", "Thunderstorm"};
        case 211: return {"Thunderstorm", " "};
        case 212: return {"Heavy", "Thunderstorm"};
        case 221: return {"Ragged", "Thunderstorm"};
        case 230: return {"Thunderstorm with", "Light Drizzle"};
        case 231: return {"Thunderstorm with", "Drizzle"};
        case 232: return {"Thunderstorm with", "Heavy Drizzle"};
        case 300: return {"Light", "Intensity Drizzle"};
        case 301: return {"Drizzle", " "};
        case 302: return {"Heavy", "Intensity Drizzle"};
        case 310: return {"Light Intensity", "Drizzle Rain"};
        case 311: return {"Drizzle", "Rain"};
        case 312: return {"Heavy Intensity", "Drizzle Rain"};
        case 313: return {"Shower Rain", "and Drizzle"};
        case 314: return {"Heavy Shower", "Rain and Drizzle"};
        case 321: return {"Shower", "Drizzle"};
        case 500: return {"Light", "Rain"};
        case 501: return {"Moderate", "Rain"};
        case 502: return {"Heavy", "Intensity Rain"};
        case 503: return {"Very", "Heavy Rain"};
        case 504: return {"Extreme", "Rain"};
        case 511: return {"Freezing", "Rain"};
        case 520: return {"Light Intensity", "Shower Rain"};
        case 521: return {"Shower", "Rain"};
        case 522: return {"Heavy Intensity", "Shower Rain"};
        case 531: return {"Ragged", "Shower Rain"};
        case 600: return {"Light", "Snow"};
        case 601: return {"Snow", " "};
        case 602: return {"Heavy", "Snow"};
        case 611: return {"Sleet", " "};
        case 612: return {"Light", "Shower Sleet"};
        case 613: return {"Shower", "Sleet"};
        case 615: return {"Light Rain", "and Snow"};
        case 616: return {"Rain", "and Snow"};
        case 620: return {"Light", "Shower Snow"};
        case 621: return {"Shower", "Snow"};
        case 622: return {"Heavy", "Shower Snow"};
        case 701: return {"Mist", " "};
        case 711: return {"Smoke", " "};
        case 721: return {"Haze", " "};
        case 731: return {"Sand/Dust", "Whirls"};
        case 741: return {"Fog", " "};
        case 751: return {"Sand", " "};
        case 761: return {"Dust", " "};
        case 762: return {"Volcanic", "ash"};
        case 771: return {"Squalls", " "};
        case 781: return {"Tornado", " "};
        case 800: return {"Clear", "Sky"};
        case 801: return {"Few Clouds", "Sky Coverage 11-25%"};
        case 802: return {"Scattered Clouds", "Sky Coverage 25-50%"};
        case 803: return {"Broken Clouds", "Sky Coverage 51-84%"};
        case 804: return {"Overcast Clouds", "Sky Coverage 85-100%"};
        default: return {"Unknown", " "};
    }
}

void DeseralizeForecastBase(Json::Value& root, ForecastBase& forecastBase)
{    
    forecastBase.time = root["dt"].asInt64();
    forecastBase.code = root["weather"][0]["id"].asInt();
    forecastBase.description = DescriptionFromCode(forecastBase.code);

    forecastBase.windVelocity = static_cast<int32_t>(ToMPH(root["wind_speed"].asDouble()));
    if (root.isMember("wind_gust"))
        forecastBase.windGust = static_cast<int32_t>(ToMPH(root["wind_gust"].asDouble()));
    forecastBase.windDir = root["wind_deg"].asInt();

    if (root.isMember("pop"))
        forecastBase.precipProbability = static_cast<int32_t>(root["pop"].asDouble() * 100);
}

void DeserializeForecastForHour(Json::Value& root, ForecastForHour& hour)
{
    double visibility = root["visibility"].asDouble();
    DeseralizeForecastBase(root, hour);
    hour.isForecastForHour = true;
    hour.temperature = static_cast<int32_t>(ToFarenheight(root["temp"].asDouble()));
    hour.visibility = static_cast<int32_t>(visibility == 10000 ? 7 : static_cast<int32_t>(ToMiles(visibility)));
}

void DeserializeForecastForDay(Json::Value& root, ForecastForDay& day)
{
    DeseralizeForecastBase(root, day);
    day.highTemperature = static_cast<int32_t>(ToFarenheight(root["temp"]["max"].asDouble()));
    day.lowTemperature = static_cast<int32_t>(ToFarenheight(root["temp"]["min"].asDouble()));
    day.label = GetShortDate(day.time);
}

void FindForecastItems(Json::Value& forecastRoot, ForecastItems& result)
{
    const string space(" at ");
    result.high.temperature = INT32_MIN;
    result.low.temperature = INT32_MAX;

    DeserializeForecastForHour(forecastRoot["current"], result.now);
    result.now.label = Labels::now + space + GetShortDateTime(result.now.time);

    DeserializeForecastForDay(forecastRoot["daily"][1], result.tomorrow);
    DeserializeForecastForDay(forecastRoot["daily"][2], result.nextDay);

    auto hr = 0;
    for (Json::Value::ArrayIndex i = 0; i != forecastRoot["hourly"].size() && hr != 24; i++, hr++)
    {
        ForecastForHour current;
        DeserializeForecastForHour(forecastRoot["hourly"][i], current);
        
        if(current.temperature > result.high.temperature)
            result.high = current;

        if(current.temperature < result.low.temperature)
            result.low = current;

        if(InterestingRank(current.code) > InterestingRank(result.notable.code))
            result.notable = current;
    }

    result.high.label = Labels::high + space + GetHour(result.high.time);
    result.low.label = Labels::low + space + GetHour(result.low.time);
    result.notable.label = Labels::notable + space + GetHour(result.notable.time);
}

size_t DownloadToStream(const void *data, size_t size, size_t nmemb, void *pDownloadStreams)
{
    size_t bytesProcessed = size * nmemb;
    auto pStreams = static_cast<DownloadStreams*>(pDownloadStreams);
    auto str = (char*)calloc(bytesProcessed + 1, sizeof(char));
    memcpy(str, data, bytesProcessed);

    (*pStreams->memory) << str;
    (*pStreams->outFile) << str;

    free(str);

    return bytesProcessed;
}

Json::Value RegionalForecast::LoadForecast(const SelectedRegion& selectedRegion, const fs::path& pathToJson)
{
    Json::Value forecastRoot;
    auto appid = getenv("OPENWEATHERMAP_APPID");

    if(!appid)
    {
        cout << "Returning data cached in " << pathToJson << endl;
        if(!fs::exists(pathToJson))
            ERR_OUT(pathToJson << " not found. Nor was the OPENWEATHERMAP_APPID environment variable set. Exiting.")

        ifstream inStream;
        inStream.open(pathToJson);
        inStream >> forecastRoot;
        return forecastRoot;
    }

    cout << "Updating cache to " << pathToJson << endl;

    long httpCode = 0;
    stringstream buffer;
    ofstream outStream;
    outStream.open (pathToJson, ofstream::out | ofstream::trunc);

    DownloadStreams streams = { &buffer, &outStream };

    auto regionalCoord = selectedRegion.GetRegionalCoord();
    auto url = string("https://api.openweathermap.org/data/3.0/onecall?lat=" + to_string(regionalCoord.lat) + "&lon=" + to_string(regionalCoord.lon) + "&appid=") + appid;
    HttpClient::Get(url.c_str(), DownloadToStream, &streams);

    outStream.close();

    buffer >> forecastRoot;
    return forecastRoot;
}

function<DSRect(IDrawTextContext*)> RegionalForecast::AddLabel(DSColor color, string& text, double offsetLeft, double offsetTop)
{
    return [=](IDrawTextContext* textContext)
    {
        textContext->SetFontSize(48);
        textContext->SetTextFillColor(color);
        textContext->AddText(text.c_str());
        auto textBounds = textContext->Bounds();
        auto centeredBounds = CalcRectForCenteredLocation({columnWidth, totalHeight}, textBounds);
        centeredBounds.origin.x += borderGap + offsetLeft;
        centeredBounds.origin.y = borderGap + offsetTop;
        return centeredBounds;
    };
}

function<DSRect(IDrawTextContext*)> RegionalForecast::RenderForecast(IDrawService* draw, double offsetLeft, double offsetTop, ForecastBase* forecast)
{
    for(auto i = 0; i < 2; i++)
    {
        auto descriptionBounds = draw->DrawText([=](IDrawTextContext* textContext)
        {
            textContext->SetFontSize(38);
            textContext->SetTextFillColor({255, 255, 255});
            textContext->AddText(forecast->description.lines[i]);

            auto textBounds = textContext->Bounds();

            auto width = textBounds.width;
            auto left = columnWidth * 0.5 - textBounds.width * 0.5;
            return DSRect {offsetLeft + left + borderGap, borderGap + offsetTop, textBounds.width, textBounds.height};
        });

        offsetTop += descriptionBounds.size.height + borderGap;
    }

    return [=](IDrawTextContext* textContext)
    {
        ForecastForHour* forHour = forecast->isForecastForHour ? static_cast<ForecastForHour*>(forecast) : nullptr;
        ForecastForDay* forDay = !forecast->isForecastForHour ? static_cast<ForecastForDay*>(forecast) : nullptr;
        DSColor color = {0};

        auto AddTextWithColor = [&](string text, const DSColor& textColor) 
        {
            textContext->SetTextFillColor(textColor);
            if(ContrastRatio(textColor, discordBg) < 5)
                textContext->SetTextStrokeColorWithThickness({255, 255, 255, 200}, 1.5);

            textContext->AddText(text);
            textContext->SetTextStrokeColorWithThickness(PredefinedColors::white, 0);
            textContext->SetTextFillColor(white);
        };

        textContext->SetFontSize(36);
        //Temperature
        if(forHour)
        {
            color = ColorFromDegrees(forHour->temperature);
            AddTextWithColor(to_string(forHour->temperature), color);
        }
        else
        {
            AddTextWithColor("High ", orange);
            color = ColorFromDegrees(forDay->highTemperature);
            AddTextWithColor(ToStringWithPad(3, ' ', forDay->highTemperature), color);
            textContext->AddText(" ºF\n");
            AddTextWithColor("Low  ", cyan);
            color = ColorFromDegrees(forDay->lowTemperature);
            AddTextWithColor(ToStringWithPad(3, ' ', forDay->lowTemperature), color);            
        }

        textContext->AddText(" ºF\n");

        //Wind
        textContext->AddText("Wind " + string(Unicode::GetWindArrow(forecast->windDir)) + " at ");
        color = ColorFromWind(forecast->windVelocity);
        AddTextWithColor(to_string(forecast->windVelocity), color);
        textContext->AddText(" mph\n");

        if(forecast->windGust != 0 && (forecast->windGust - forecast->windVelocity) > 5)
        {
            textContext->AddText("Gusts     ");
            color = ColorFromWind(forecast->windGust);
            AddTextWithColor(to_string(forecast->windGust), color);
            textContext->AddText(" mph\n");
        }        
        else
            textContext->AddText("\n");

        //Visibility
        if(forHour)
        {
            textContext->AddText("Visibility ");
            if(forHour->visibility > 6)
                textContext->AddText(">6");
            else
                textContext->AddText(ToStringWithPad(2, ' ', forHour->visibility));

            textContext->AddText(" mi\n");
        }

        textContext->AddText("Precip Odds ");
        textContext->AddText(to_string(forecast->precipProbability));
        textContext->AddText("%");
        auto textBounds = textContext->Bounds();

        auto emojiForcastGap = 16;
        auto width = emojiForcastGap + textBounds.height + textBounds.width;
        auto left = borderGap + (columnWidth * 0.5 - width * 0.5);

        auto emojiPath = Emoji::Path(LookupEmoji(forecast->code));
        draw->DrawImage(emojiPath, { left + offsetLeft, borderGap + offsetTop, textBounds.height, textBounds.height });
        return DSRect {left + emojiForcastGap + offsetLeft + textBounds.height, borderGap + offsetTop, textBounds.width, textBounds.height};
    };
}

double RegionalForecast::DrawBoxHeaderLabels(IDrawService* draw, double top, string& labelLeft, DSColor colorLeft, string& labelRight, DSColor colorRight)
{
    //Label text
    auto textBounds = draw->DrawText(AddLabel(colorLeft, labelLeft, 0, top));
    draw->DrawText(AddLabel(colorRight, labelRight, columnWidth, top));

    //Draw line below the headers to make a box.
    auto bottom = textBounds.origin.y + textBounds.size.height + borderGap;
    draw->MoveTo({borderGap, bottom});
    draw->LineTo({totalWidth - borderGap, bottom});
    bottom += borderGap;
    return bottom;
}

double RegionalForecast::DrawForecastBoxes(IDrawService* draw, double top, ForecastBase* forecastLeft, ForecastBase* forecastRight)
{
    //Draw the two forecats.
    draw->DrawText(RenderForecast(draw, borderGap, top, forecastLeft));
    auto textBounds = draw->DrawText(RenderForecast(draw, columnWidth + borderGap, top, forecastRight));
    auto bottom = textBounds.origin.y + textBounds.size.height + borderGap;

    //If it's an hourly forecast, draw the line across the bottom.
    if(forecastLeft->isForecastForHour)
    {
        draw->MoveTo({borderGap, bottom});
        draw->LineTo({totalWidth - borderGap, bottom});
    }

    return bottom;
}

void RegionalForecast::Render(const SelectedRegion& selectedRegion, const fs::path& pathToJson, const fs::path& pathToPng)
{
    ForecastItems forecastItems = {0};
    auto forecastRoot = LoadForecast(selectedRegion, pathToJson);
    FindForecastItems(forecastRoot, forecastItems);

    auto draw = unique_ptr<IDrawService>(AllocDrawService(totalWidth, totalHeight));
    draw->ClearCanvas(discordBg);
    draw->SetDropShadow({5.0, -5.0}, 5.0);
    draw->AddRoundedRectangle({borderGap, borderGap, renderAreaWidth, renderAreaHeight}, 8.0, 8.0);
    draw->MoveTo({totalWidth * 0.5, borderGap});
    draw->LineTo({totalWidth * 0.5, totalHeight - borderGap});

    // Now and Interesting Labels
    auto bottom = DrawBoxHeaderLabels(draw.get(), borderGap, forecastItems.now.label, white, forecastItems.notable.label, yellow);
    
    //Render first two forecasts
    bottom = DrawForecastBoxes(draw.get(), bottom, &forecastItems.now, &forecastItems.notable);

    //Render High and Low Header
    bottom = DrawBoxHeaderLabels(draw.get(), bottom, forecastItems.high.label, orange, forecastItems.low.label, cyan);

    //Render high low forecasts
    bottom = DrawForecastBoxes(draw.get(), bottom, &forecastItems.high, &forecastItems.low);

    //Render tomorrow and next day headers
    bottom = DrawBoxHeaderLabels(draw.get(), bottom, forecastItems.tomorrow.label, white, forecastItems.nextDay.label, white);

    //Render tomorrow and next day forecasts
    bottom = DrawForecastBoxes(draw.get(), bottom, &forecastItems.tomorrow, &forecastItems.nextDay);

    draw->SetStrokeColor(white);
    draw->StrokeActivePath(2.0);

    draw->Save(pathToPng);
}