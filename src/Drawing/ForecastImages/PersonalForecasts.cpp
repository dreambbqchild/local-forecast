#include "PersonalForecasts.h"
#include "Characters.h"
#include "DateTime.h"
#include "Drawing/WxColors.h"
#include "NumberFormat.h"
#include "StringExtension.h"

#include <format>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;
using namespace chrono;
using namespace PredefinedColors;
namespace fs = std::filesystem;

#define FONT_SIZE 34
#define ROW_HEIGHT 42.5
#define TABLE_TOP FONT_SIZE

struct DayLabelInfo
{
    string label;
    DSRect bounds;
};

typedef DSColor (*ColorFn)(double farenheight);

const char* GetTimeOfDayEmoji(time_t forecastTime, time_t sunrise, time_t sunset)
{
    auto forecastHour = GetHourFromTime(forecastTime);
    if(forecastTime <= sunrise)
    {
        auto sunriseHour = GetHourFromTime(sunrise);
        if(forecastHour == sunriseHour)
            return Emoji::sunset;
        
        return Emoji::night_with_stars;
    }

    auto sunsetHour = GetHourFromTime(sunset);
    if(forecastHour == sunsetHour)
        return Emoji::sunset;
    else if(forecastTime < sunset)
        return Emoji::cityscape;

    return Emoji::night_with_stars;
}

const char* GetVisibilityEmoji(int32_t visibility, time_t forecastTime, time_t sunrise, time_t sunset)
{
    if(visibility <= 3)
        return Emoji::fog;
    else if (visibility <= 6)
        return Emoji::foggy;

    return GetTimeOfDayEmoji(forecastTime, sunrise, sunset);
}

inline const char* GetVisibilityEmoji(int32_t visibility, system_clock::time_point forecastTime, system_clock::time_point sunrise, system_clock::time_point sunset)
{
    return GetVisibilityEmoji(visibility, system_clock::to_time_t(forecastTime), system_clock::to_time_t(sunrise), system_clock::to_time_t(sunset));
}

void DivideColumn(IDrawTextContext* textContext, bool isMeasurePass, vector<double>& columnXs, int32_t& columns)
{
    auto boundsBefore = textContext->Bounds();
    textContext->AddText("  ");
 
    if(!isMeasurePass || columnXs.size() != columns)
        return;
 
    auto boundsAfter = textContext->Bounds();

    auto gapWidth = boundsAfter.width - boundsBefore.width;
    columnXs.push_back(boundsBefore.width + gapWidth * 0.5);

    columns++;
}

void LayoutColumns(unique_ptr<IDrawService>& draw, double xOffset, vector<DayLabelInfo>& dayLabels, vector<double> columnXs)
{
    const char* columnLabels[] = 
    {
        "Time",
        "Vis",
        "Temp/Dew Pt",
        "Sky",
        "Precip",
        "Wind",
        "Press"
    };

    auto left = xOffset;
    auto labelIndex = -1;
    auto imageSize = draw->GetSize();

    columnXs.insert(columnXs.begin(), xOffset);
    columnXs.push_back(imageSize.width);

    for(auto x : columnXs)
    {
        auto top = 0.0;
        for(auto& dayLabel : dayLabels)
        {
            draw->MoveTo({x, top});
            draw->LineTo({x, dayLabel.bounds.origin.y});

            draw->MoveTo({xOffset, dayLabel.bounds.origin.y});
            draw->LineTo({imageSize.width, dayLabel.bounds.origin.y});

            top = dayLabel.bounds.origin.y + dayLabel.bounds.size.height;
            draw->MoveTo({xOffset, top});
            draw->LineTo({imageSize.width, top});
        }

        draw->MoveTo({x, top});
        draw->LineTo({x, imageSize.height});

        if(labelIndex >= 0)
        {
            draw->DrawText([&](IDrawTextContext* textContext)
            {
                textContext->SetFontSize(FONT_SIZE);
                textContext->SetTextFillColor(white);
                textContext->AddText(columnLabels[labelIndex++]);
                auto textBounds = textContext->Bounds();
                return CalcRectForCenteredLocation({x - left, textBounds.height}, textBounds, {left, 0});
            });
        }
        else
            labelIndex++;

        left = x;
    }

    draw->SetStrokeColor(white);
    draw->StrokeActivePath(2.0);

    for(auto& dayLabel : dayLabels)
    {
        draw->DrawText([&](IDrawTextContext* textContext)
        {
            textContext->SetFontSize(FONT_SIZE);
            textContext->SetTextFillColor(white);
            textContext->AddText(dayLabel.label);

            auto textBounds = textContext->Bounds();
            return CalcRectForCenteredLocation(dayLabel.bounds.size, textBounds, {xOffset, dayLabel.bounds.origin.y});
        });
    }
}

string GenerateFileName(int32_t forecastIndex)
{
    stringstream ss;
    ss << "personal-" << ToStringWithPad(2, '0', forecastIndex) << ".png";
    return ss.str();
}

void DrawWithColor(IDrawTextContext* textContext, int32_t iValue, string strValue, ColorFn colorCallback)
{
    auto color = colorCallback(iValue);
    textContext->SetTextFillColor(color);
    if(ContrastRatio(color, discordBg) < 5)
        textContext->SetTextStrokeColorWithThickness({255, 255, 255, 200}, 1.5);
    
    textContext->AddText(strValue);

    textContext->SetTextStrokeColorWithThickness(PredefinedColors::white, 0);
    textContext->SetTextFillColor(white);
}

void DrawWithColor(IDrawTextContext* textContext, double dValue, string strValue, PrecipitationType precipitationType)
{
    if(TestDouble(dValue))
    {
        if(precipitationType != PrecipitationType::NoPrecipitation)
        {
            auto color = ColorFromPrecipitation(precipitationType, dValue);
            textContext->SetTextFillColor(color);
            if(ContrastRatio(color, discordBg) < 5)
                textContext->SetTextStrokeColorWithThickness({255, 255, 255, 200}, 1.5);
        }
    }
    else if(strValue == "0.01\"")
        strValue = "Trace";

    if (strValue.length() > 5)
        strValue = " >9\" ";

    textContext->AddText(strValue);
    textContext->SetTextFillColor(white);
    textContext->SetTextStrokeColorWithThickness(PredefinedColors::white, 0);
}

DSSize RenderHourlyForecastForLocation(bool isMeasurePass, unique_ptr<IDrawService>& draw, unique_ptr<ILocation>& location, double xOffset, double areaWidth, system_clock::time_point& now, vector<double>& columnXs, int32_t maxRows)
{
    int32_t dayCounter = 0;
    system_clock::time_point sunrise, sunset;
    vector<DayLabelInfo> dayLabels;
    DSSize forecastTableSize = {0};
    string forecastDate;

    draw->ClearCanvas(discordBg);
    draw->SetDropShadow({5.0, -5.0}, 5.0);
    
    const WxSingle* wxLast = nullptr;
    auto suns = location->GetSunriseSunsets();
    auto sunItr = suns.begin();
    auto rowsRendered = location->GetForecastsFromNow(now, maxRows, [&](system_clock::time_point& forecastTime, const WxSingle* wxCurrent)
    {
        if((system_clock::to_time_t(forecastTime) + timezone) % 86400 == 0)
            dayCounter++;

        if(isMeasurePass && forecastTableSize.height)
            return;

        auto textBounds = draw->DrawText([&](IDrawTextContext* textContext)
        {
            auto columnCountForMeasure = 0;            
            auto currentDate = GetShortDate(forecastTime);

            textContext->SetFontSize(FONT_SIZE);
            textContext->SetTextFillColor(white);

            if(!forecastDate.length())
            {
                auto tm = ToLocalTm(forecastTime);
                forecastDate = GetShortDate(forecastTime);

                while(sunItr->day != tm.tm_mday)
                    sunItr++;

                sunrise = system_clock::from_time_t(sunItr->sun.rise);
                sunset = system_clock::from_time_t(sunItr->sun.set);
            }

            if(!isMeasurePass && currentDate != forecastDate)
            {
                forecastDate = currentDate;
                
                sunItr++;
                sunrise = system_clock::from_time_t(sunItr->sun.rise);
                sunset = system_clock::from_time_t(sunItr->sun.set);
                
                dayLabels.push_back({ GetShortDayOfWeek(forecastTime) + " " + forecastDate, {xOffset, TABLE_TOP + forecastTableSize.height, areaWidth, ROW_HEIGHT }});
                forecastTableSize.height += ROW_HEIGHT;
            }               

            auto shortHour = GetShortHour(forecastTime);
            while(shortHour.length() < 4)
                shortHour = " " + shortHour;

            auto hourTotal = CalcPrecipAmount(wxCurrent, wxLast);
            
            //Time
            textContext->AddText(shortHour);

            //Day/Night/Visibility
            DivideColumn(textContext, isMeasurePass, columnXs, columnCountForMeasure);
            textContext->AddImage(Emoji::Path(GetVisibilityEmoji(wxCurrent->visibility, forecastTime, sunrise, sunset)));
            
            //Temperature Dewpoint
            DivideColumn(textContext, isMeasurePass, columnXs, columnCountForMeasure);
            auto iValue = wxCurrent->temperature;
            DrawWithColor(textContext, iValue, ToStringWithPad(3, ' ', iValue), ColorFromDegrees);        
            textContext->AddText("/");
            iValue = wxCurrent->dewpoint;
            DrawWithColor(textContext, iValue, ToStringWithPad(3, ' ', iValue), ColorFromDegrees);
            textContext->AddText(" ºF");

            //Sky Condition
            DivideColumn(textContext, isMeasurePass, columnXs, columnCountForMeasure);
            textContext->AddImage(Emoji::GetSkyEmojiPath(wxCurrent->totalCloudCover, wxCurrent->lightning, static_cast<PrecipitationType>(wxCurrent->precipitationType), wxCurrent->precipitationRate));
            
            //Precip
            DivideColumn(textContext, isMeasurePass, columnXs, columnCountForMeasure);
            DrawWithColor(textContext, hourTotal, ToStringWithPrecision(2, hourTotal) + "\"", static_cast<PrecipitationType>(wxCurrent->precipitationType));

            //Wind + Gust
            DivideColumn(textContext, isMeasurePass, columnXs, columnCountForMeasure);
            textContext->AddText(Unicode::GetWindArrow(wxCurrent->windDirection) + string(" "));
            iValue = wxCurrent->windSpeed;
            DrawWithColor(textContext, iValue, ToStringWithPad(2, ' ', iValue), ColorFromWind);

            if(max(0, wxCurrent->gust - wxCurrent->windSpeed) > 5)
            {
                textContext->AddText(" ");
                iValue = wxCurrent->gust;
                textContext->SetFontOptions(FontOptions::Bold);
                DrawWithColor(textContext, iValue, ToStringWithPad(2, ' ', iValue), ColorFromWind);
                textContext->SetFontOptions(FontOptions::Regular);
            }
            else
                textContext->AddText("   ");

            textContext->AddText(" mph");

            //Barometric Pressure
            DivideColumn(textContext, isMeasurePass, columnXs, columnCountForMeasure);
            textContext->AddText(ToStringWithPrecision(2, wxCurrent->pressure) + string("\" "));

            auto size = textContext->Bounds();
            wxLast = wxCurrent;
            return CalcRectForCenteredLocation(size, {size.width, ROW_HEIGHT}, {xOffset, TABLE_TOP + forecastTableSize.height});
        });

        forecastTableSize.height += textBounds.size.height;
        forecastTableSize.width = max(textBounds.size.width, forecastTableSize.width);
    });

    if(!isMeasurePass)
        LayoutColumns(draw, xOffset, dayLabels, columnXs);

    return {
        .width = forecastTableSize.width, 
        .height = forecastTableSize.height * (rowsRendered + dayCounter) + TABLE_TOP
    };  //rows + header + Label Rows.
}

function<DSRect(IDrawTextContext*)> RenderCenteredSummaryLine(std::string text, DSRect bounds, double fontSize = FONT_SIZE, DSColor textColor = PredefinedColors::white)
{
    return [=](IDrawTextContext* textContext)
    {
        textContext->SetFontSize(fontSize);
        textContext->SetTextFillColor(textColor);

        if(ContrastRatio(textColor, discordBg) < 5)
            textContext->SetTextStrokeColorWithThickness({255, 255, 255, 200}, 1.5);

        textContext->AddText(text);

        auto textBounds = textContext->Bounds();
        return CalcRectForCenteredLocation({bounds.size.width, textBounds.height}, textBounds, {0, bounds.origin.y});
    };
}

#define PrecipSummary(label, precipType, value)\
if(TestDouble(value))\
{\
        AddSummaryLine("\n" label);\
        auto c = ColorFromPrecipitation(precipType, value);\
        auto s = ToStringWithPrecision(2, value) + "\"";\
        if(s == "0.00\"")\
        {\
            s = "Trace";\
            c = white;\
        }\
        AddSummaryLine(s, FONT_SIZE * 1.5, c);\
}\

void RenderSummaryImageForLocation(unique_ptr<IDrawService>& draw, const SummaryData& summaryData, system_clock::time_point& now, const DSSize& size, int32_t maxRows)
{
    DSRect bounds = {0, 0, size.width, size.height};

    auto AddSummaryLine = [&](string text, double fontSize = FONT_SIZE, DSColor color = white) { bounds.origin.y += draw->DrawText(RenderCenteredSummaryLine(text, bounds, fontSize, color)).size.height; };

    AddSummaryLine(GetShortDayOfWeek(now));
    AddSummaryLine(GetShortDate(now));
    AddSummaryLine(summaryData.locationName);

    if(summaryData.sunrise < summaryData.sunset)
    {
        AddSummaryLine("\nSunrise");
        AddSummaryLine(GetHoursAndMinutes(summaryData.sunrise));

        AddSummaryLine("\nSunset");
        AddSummaryLine(GetHoursAndMinutes(summaryData.sunset));
    }
    else
    {
        AddSummaryLine("\nSunset");
        AddSummaryLine(GetHoursAndMinutes(summaryData.sunset));

        AddSummaryLine("\nSunrise");
        AddSummaryLine(GetHoursAndMinutes(summaryData.sunrise));
    }

    auto color = ColorFromDegrees(summaryData.high);
    AddSummaryLine("\nHigh", FONT_SIZE, orange);
    AddSummaryLine(to_string(summaryData.high) + "ºF", FONT_SIZE * 1.5, color);

    color = ColorFromDegrees(summaryData.low);
    AddSummaryLine("\nLow", FONT_SIZE, cyan);
    AddSummaryLine(to_string(summaryData.low) + "ºF", FONT_SIZE * 1.5, color);

    PrecipSummary("Snow", PrecipitationType::Snow, summaryData.snowTotal)
    PrecipSummary("Ice", PrecipitationType::FreezingRain, summaryData.iceTotal)
    PrecipSummary("Rain", PrecipitationType::Rain, summaryData.rainTotal)    
}

#undef PrecipSummary

void PersonalForecasts::RenderAll(fs::path forecastDataOutputDir, int32_t maxRows)
{
    vector<double> columnXs;
    vector<SummaryData> summaryDatum;
    auto xOffset = 0.0, forecastAreaWidth = 0.0;
    auto now = system_clock::from_time_t(forecast->GetNow());
    uint32_t imageHeight = INT16_MAX;

    auto locations = forecast->GetSortedPlaceLocations();
    summaryDatum.resize(locations.size());

    //Measure Pass
    {
        auto draw = unique_ptr<IDrawService>(AllocDrawService(totalWidth, imageHeight));
        auto size = RenderHourlyForecastForLocation(true, draw, locations.at(0), 0, 0, now, columnXs, maxRows);
        xOffset = totalWidth - size.width;
        forecastAreaWidth = size.width;
        imageHeight = max(defaultImageHeight, static_cast<uint32_t>(ceil(size.height)));
    }

    for(double& d : columnXs)
        d += xOffset;

    //Arrange Pass
    auto index = 0;
    for(auto& location : locations)
    {
        auto draw = unique_ptr<IDrawService>(AllocDrawService(totalWidth, imageHeight));        
        
        cout << "Rendering Personal Forecast for: " << location->GetId() << " (" << index << ")" << endl;
        
        RenderHourlyForecastForLocation(false, draw, location, xOffset, forecastAreaWidth, now, columnXs, maxRows);
        
        location->CollectSummaryData(summaryDatum[index], maxRows);
        RenderSummaryImageForLocation(draw, summaryDatum[index], now, { xOffset, static_cast<double>(imageHeight) }, maxRows);

        draw->Save(forecastDataOutputDir / GenerateFileName(index));

        index++;
    }
}