#include "Calcs.h"
#include "DateTime.h"
#include "StringExtension.h"
#include "SummaryForecast.h"

#include <boost/algorithm/string/trim.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;
using namespace chrono;
namespace fs = std::filesystem;

static const char* emptyString = "";

class SummaryForecast : public ISummaryForecast
{
private:
    vector<string> highNames, lowNames, windNames, snowNamesAll, iceNamesAll, rainNamesAll, snowNamesExtreme, iceNamesExtreme, rainNamesExtreme;

    template <typename T>
    void ManageExtremes(vector<string>& names, T& currentExtremeValue, const T& currentValue, const string& locationName, bool seekingMax = true)
    {
        if(seekingMax ? currentValue > currentExtremeValue : currentValue < currentExtremeValue)
        {
            names.clear();
            currentExtremeValue = currentValue;
            names.push_back(locationName);
        }
        else if(currentValue == currentExtremeValue)
            names.push_back(locationName);
    }

    void ManagePrecipExtremes(vector<string>& names, vector<string>& extremeNames, double& currentExtremeValue, double currentValue, const string& locationName, int32_t locationMax)
    {
        if(!TestDouble(currentValue))
            return;

        names.push_back(locationName);

        if(names.size() == locationMax)
        {
            names.clear();
            names.push_back("everyone");
        }

        if(currentValue > currentExtremeValue)
        {
            extremeNames.clear();
            currentExtremeValue = currentValue;        
            extremeNames.push_back(locationName);
        }
        else if(currentValue == currentExtremeValue)
            extremeNames.push_back(locationName);
    }

    inline string JoinNames(vector<string> names) { return JoinWithCommasButFinishWithAmpersand(names, true); }

    inline const char* Snide(const char* remark, bool makeIt)
    {
        if(makeIt)
            return remark;

        return emptyString;   
    }

    void AppendHRRRModelTextIce(stringstream& result, SummaryData& extremes)
    {
        if(!iceNamesAll.size())
            return;
        
        result << "● Be careful out there " << JoinNames(iceNamesAll) << ". You're forecast to get some ice. " << JoinNames(iceNamesExtreme) << " " << SingularPlural(iceNamesExtreme.size(), "is", "are") << " forecast to see the most with " << fixed << setprecision(2) << extremes.iceTotal << R"("!)" << endl;
    }

    void AppendHRRRModelTextSnow(stringstream& result, SummaryData& extremes)
    {
        if(!snowNamesAll.size())
            return;

        result << "● ";
        if(extremes.snowTotal >= 12)
            result << "Looks like you're going to be getting hit with a blizzard";
        else if(extremes.snowTotal >= 6)
            result << "Get out the shovels";
        else if(extremes.snowTotal >= 3)
            result << "There may be some treacherous snowy driving out your front door";
        else
            result << "Got some incoming snowy slop to deal with";

        result << " " << JoinNames(snowNamesAll) << ". " << JoinNames(snowNamesExtreme) << " " << SingularPlural(snowNamesExtreme.size(), "is", "are") << " forecast to see the most with " << fixed << setprecision(2) << extremes.snowTotal << R"(")" << (extremes.snowTotal >= 6 ? "!" : ".") << endl;
    }

    void AppendHRRRModelTextRain(stringstream& result, SummaryData& extremes)
    {
        if(!rainNamesAll.size())
            return;
            
        if(extremes.rainTotal < 0.1)
        {
            result << "• Going to see a little rain or mist today " << JoinNames(rainNamesAll) << ". " << JoinNames(rainNamesExtreme) << " " << SingularPlural(rainNamesExtreme.size(), "is", "are") << " forecast to get the most with " << fixed << setprecision(2) << extremes.rainTotal << R"(".)" << endl;
            return;
        }

        result << "• Going to be a rainy day for ya " << JoinNames(rainNamesAll) << ". " << JoinNames(rainNamesExtreme) << " " << SingularPlural(rainNamesExtreme.size(), "is", "are") << " forecast to get the most with " << fixed << setprecision(2) << extremes.rainTotal << R"(".)" << endl;
    }

    string GetTextSummary(const char* moonEmoji, const char*  moonPhase, vector<SummaryData>& summaryDatum, string& lastDate)
    {
        SummaryData extremes;
        for(auto& summaryData : summaryDatum)
        {
            ManageExtremes(highNames, extremes.high, summaryData.high, summaryData.locationName);
            ManageExtremes(lowNames, extremes.low, summaryData.low, summaryData.locationName, false);
            ManageExtremes(windNames, extremes.wind, summaryData.wind, summaryData.locationName);
            ManagePrecipExtremes(snowNamesAll, snowNamesExtreme, extremes.snowTotal, summaryData.snowTotal, summaryData.locationName, summaryDatum.size());
            ManagePrecipExtremes(iceNamesAll, iceNamesExtreme, extremes.iceTotal, summaryData.iceTotal, summaryData.locationName, summaryDatum.size());
            ManagePrecipExtremes(rainNamesAll, rainNamesExtreme, extremes.rainTotal, summaryData.rainTotal, summaryData.locationName, summaryDatum.size());
        }

        stringstream result;
        result << moonEmoji << " Today the moon will be in the " << moonPhase << " phase "<< endl
            << endl
            << "Between now and " << lastDate << ":" << endl
            << "• " << JoinNames(highNames) << " can expect the highest high of " << extremes.high << "ºF." << Snide(R"( (Going to have to turn on the AC))", extremes.high <= 0) << endl
            << "• " << JoinNames(lowNames) << " should see the lowest low of " << extremes.low << "ºF." << Snide(R"( (Yeah. That's a real "Low" there eh?))", extremes.low >= 70) << endl
            << "• " << JoinNames(windNames) << " " << SingularPlural(windNames.size(), "has", "have") << " the best chance to experience the highest sustained wind at " << extremes.wind << "mph." << Snide(R"( (All together now: "🦊It WIMDY!!🦊"))", extremes.wind >= 30) << endl;
        
        result << endl;
        if(!TestDouble(extremes.iceTotal) && !TestDouble(extremes.snowTotal) && !TestDouble(extremes.rainTotal))
        {
            result << "• No one is expected to see any precipitation.";
            return result.str();
        }

        result << "Precipitation:" << endl;
        AppendHRRRModelTextIce(result, extremes);
        AppendHRRRModelTextSnow(result, extremes);
        AppendHRRRModelTextRain(result, extremes);
        return result.str();
    }

    size_t CountPlaceLocations(vector<unique_ptr<ILocation>>& locations)
    {
        size_t placeLocationCount = 0;

        for(auto& location : locations)
        {
            if(location->IsCity())
                continue;

            placeLocationCount++;
        }

        return placeLocationCount;
    }

public:
    void Render(fs::path textForecastOutputPath, const unique_ptr<IForecast>& forecast, int32_t maxRows)
    {
        string lastDate;
        auto index = 0;
        vector<SummaryData> summaryDatum;
        auto now = system_clock::from_time_t(forecast->GetNow());
        auto locations = forecast->GetLocations(LocationMask::Homes);
        size_t placeLocationCount = CountPlaceLocations(locations);
        
        summaryDatum.resize(static_cast<size_t>(placeLocationCount));

        for(auto& location : locations)
        {
            if(location->IsCity())
                continue;

            auto& summaryData = summaryDatum[index];
            location->CollectSummaryData(summaryData, maxRows);
            index++;
        }

        forecast->GetForecastsFromNow(now, maxRows, [&](system_clock::time_point& forecastTime, int32_t forecastIndex)
        {
            lastDate = GetLongDateTime(forecastTime + 1h); //We want to the END of the resulting hour.
        });

        cout << "Rendering text forecast..." << endl;
        auto nowDay = ToLocalTm(now).tm_mday;
        auto lunarPhase = forecast->GetLunarPhaseForDay(nowDay);
        auto textForecast = GetTextSummary(Astronomy::GetLunarPhaseEmoji(lunarPhase), Astronomy::GetLunarPhaseLabel(lunarPhase), summaryDatum, lastDate);
        boost::algorithm::trim(textForecast);
        ofstream outStream(textForecastOutputPath);
        outStream << textForecast;
        outStream.close();
    }
};

ISummaryForecast* AllocSummaryForecast()
{
    return new SummaryForecast();
}