#include "Astronomy/Astronomy.h"
#include "BarycentricCoordinates.h"
#include "Calcs.h"
#include "Characters.h"
#include "DateTime.h"
#include "GribProcessor.h"
#include "Locations.h"
#include "NumberFormat.h"

#include <omp.h>
#include <string.h>

#include <fstream>
#include <iostream>

#include <json/json.h>

using namespace std;
using namespace chrono;
namespace fs = std::filesystem;

#define ShortNameIs(value) !strcmp(fieldData.shortName, value)
#define TypeOfLevelIs(value) !strcmp(fieldData.typeOfLevel, value)
#define LevelIs(value) !strcmp(fieldData.level, value)

#define FOR_FORECASTS_IN_RANGE(i) for(auto i = parent->skipToGribNumber; i <= parent->maxGribIndex; i++)

const Wx emptyWx = {PrecipitationType::None};

struct LocalForecastPayload {
    const Vector2d bounds[4];
    const double values[4];
};

inline double LocalForecast(const Vector2d& v, const LocalForecastPayload& payload)
{
    double resultWeights[4] = {0};
    BarycentricCoordinatesForCWTetrahedron(v, payload.bounds, resultWeights);

    return WeightValues4(payload.values, resultWeights);
}

#define LocalForecast(property) result.property = LocalForecast(Vector2d { homeCoords.x, homeCoords.y }, LocalForecastPayload { \
        { \
            { nearPoints[0].x, nearPoints[0].y }, \
            { nearPoints[1].x, nearPoints[1].y }, \
            { nearPoints[2].x, nearPoints[2].y }, \
            { nearPoints[3].x, nearPoints[3].y }  \
        }, { \
            boundsWx[0].property, \
            boundsWx[1].property, \
            boundsWx[2].property, \
            boundsWx[3].property  \
        } \
    } \
)

class GribProcessor::ParallelProcessor
{
private:
    GribProcessor* parent;
    LocationWeatherData& locationWeatherData;
    
    vector<system_clock::time_point> localForecastTimes;
    
    int32_t forecastTimeIndex = 0;

    Json::Value jForecastTimes;
    Json::Value jLunarPhase;

public:
    ParallelProcessor(GribProcessor* parent, LocationWeatherData& locationWeatherData)
        :parent(parent), locationWeatherData(locationWeatherData) {}

    void Initalize()
    {
        int32_t lastDay = INT32_MAX;
        cout << "Reading in grib files..." << endl;
        //Read in files one at a time because that's the way it has to be due to eccodes.
        FOR_FORECASTS_IN_RANGE(i)
        {
            string path(parent->gribPathTemplate.length() + 2, '\0');
            snprintf((char*)path.c_str(), parent->gribPathTemplate.length() + 2, parent->gribPathTemplate.c_str(), i);

            GribReader reader(path, parent->wxModel);
            
            if(!locationWeatherData.TryReadFieldData(reader, i))
                continue;

            auto forecastAtPoint = parent->forecastStartTime + hours(i);
            jForecastTimes[forecastTimeIndex++] = Json::Value::Int64(chrono::system_clock::to_time_t(forecastAtPoint));
            localForecastTimes.push_back(forecastAtPoint);

            auto currentDay = ToLocalTm(forecastAtPoint).tm_mday;
            if(currentDay != lastDay)
            {
                cout << "Adding Lunar Information for " << GetShortDate(forecastAtPoint) << "..." << endl;
                Json::Value jPhase;
                auto lunarPhase = Astronomy::GetLunarPhase(forecastAtPoint);
                jPhase["age"] = lunarPhase.age;
                jPhase["name"] = lunarPhase.name;
                jPhase["emoji"] = lunarPhase.emoji;
                lastDay = currentDay;
                jLunarPhase[to_string(lastDay)] = jPhase;
            }
        }
    }

    void CollectData()
    {
        cout << "Collecting data..." << endl;
        #pragma omp parallel for
        for(auto i : locationWeatherData.GetValidFieldDataIndexes())
        {
            //Normalize
            for (auto &fieldData : locationWeatherData.GetValidFieldDataAtKnownValidIndex(i))
            {
                auto& pWx = locationWeatherData.GetWxAtKnownValidIndexes(i, fieldData.index);
                
                if(ShortNameIs("crain") && fieldData.value)
                    pWx.type |= PrecipitationType::Rain;
                else if(ShortNameIs("cfrzr") && fieldData.value)
                    pWx.type |= PrecipitationType::FreezingRain;
                else if(ShortNameIs("csnow") && fieldData.value)
                    pWx.type |= PrecipitationType::Snow;
                else if(ShortNameIs("prate"))
                    pWx.precipitationRate = ToInPerHour(fieldData.value);
                else if(ShortNameIs("tp"))
                {
                    if(fieldData.stepRange[0] == '0' && fieldData.stepRange[1] == '-')
                        pWx.totalPrecipitation = ToInchesFromKgPerSquareMeter(fieldData.value);
                    else
                        pWx.newPrecipitation = ToInchesFromKgPerSquareMeter(fieldData.value);
                }
                else if(ShortNameIs("sde"))
                    pWx.snowDepth = ToInchesFromMeters(fieldData.value);
                else if(ShortNameIs("asnow") || (parent->wxModel == WeatherModel::HRRR && fieldData.parameterNumber == 29))
                    pWx.totalSnow = ToInchesFromMeters(fieldData.value);
                else if(ShortNameIs("2d"))
                    pWx.dewpoint = ToFarenheight(fieldData.value);
                else if(ShortNameIs("2t"))
                    pWx.temperature = ToFarenheight(fieldData.value);
                else if(ShortNameIs("tcc") && TypeOfLevelIs("atmosphere"))
                    pWx.totalCloudCover = fieldData.value;
                else if(ShortNameIs("vis") && (parent->wxModel == WeatherModel::HRRR || TypeOfLevelIs("surface")))
                    pWx.visibility = ToMiles(fieldData.value);
                else if(parent->wxModel == WeatherModel::HRRR && ShortNameIs("10si"))
                    pWx.windSpeed = ToMPH(fieldData.value);
                else if(ShortNameIs("gust"))
                    pWx.gust = ToMPH(fieldData.value);
                else if(parent->wxModel == WeatherModel::HRRR && ShortNameIs("10u"))
                    pWx.windU = fieldData.value;
                else if(parent->wxModel == WeatherModel::GFS && ShortNameIs("u") && TypeOfLevelIs("heightAboveGround") && LevelIs("20"))
                    pWx.windU = fieldData.value;
                else if(parent->wxModel == WeatherModel::HRRR && ShortNameIs("10v"))
                    pWx.windV = fieldData.value;
                else if(parent->wxModel == WeatherModel::GFS && ShortNameIs("v") && TypeOfLevelIs("heightAboveGround") && LevelIs("20"))
                    pWx.windV = fieldData.value;
                else if(parent->wxModel == WeatherModel::HRRR && ShortNameIs("mslma"))
                    pWx.pressure = ToInHg(fieldData.value);
                else if(parent->wxModel == WeatherModel::GFS && ShortNameIs("mslet"))
                    pWx.pressure = ToInHg(fieldData.value);
                else if(ShortNameIs("ltng"))
                    pWx.lightning = fieldData.value;
            }
        }

        if(parent->wxModel == WeatherModel::GFS)
        {
            auto lastI = -1;
            for(auto i : locationWeatherData.GetValidFieldDataIndexes())
            {
                if(lastI != -1)
                {
                    #pragma omp parallel for
                    for(auto k = 0; k < locationWeatherData.GetGeoCoordsLength(); k++)
                    {
                        auto& pWx = locationWeatherData.GetWxAtKnownValidIndexes(i, k);
                        auto& pLastWx =locationWeatherData.GetWxAtKnownValidIndexes(lastI, k);

                        pWx.totalSnow = pLastWx.totalSnow;
                        if(pWx.snowDepth > pLastWx.snowDepth)
                            pWx.totalSnow += (pWx.snowDepth - pLastWx.snowDepth);
                    }
                }
                lastI = i;
            }
        }
    }

    void GenerateForecastJson(Json::Value& root)
    {
        Json::Value jLocations;
        auto pointSet = unique_ptr<IGeoPointSet>(AllocGeoPointSet(locationWeatherData.GetGeoCoords(), parent->geoCalcs));        
        cout << "Compiling JSON..." << endl;

        if(!root.isMember("now"))
        {
            auto now = system_clock::now();
            root["now"] = Json::Value::Int64(system_clock::to_time_t(now));
        }

        {
            Json::Value forecastSuffixes;
            auto forecastIndex = 0;
            for(auto i : locationWeatherData.GetValidFieldDataIndexes())
            {
                auto imgSuffix = ToStringWithPad(3, '0', i);
                forecastSuffixes[forecastIndex++] = imgSuffix;
            }

            root["forecastSuffixes"] = forecastSuffixes;
        }

        #pragma omp parallel for
        for(auto loc = 0; loc < locationsLength; loc++)
        {
            DetailedGeoCoord nearPoints[4] = {0};        
            Json::Value jLocation, jCoords, jWx, jSunriseSunset, dewpoint, precipitationRate, precipitationType, gust, lightning, newPrecipitation, pressure, temperature, totalCloudCover, totalPrecipitation, totalSnow, visibility, windDirection, windSpeed;
            Wx lastResult = {};
                    
            auto homeCoords = parent->geoCalcs.FindXY(locations[loc].lat, locations[loc].lon);
            jCoords["x"] = static_cast<uint16_t>(round(homeCoords.x)); 
            jCoords["y"] = static_cast<uint16_t>(round(homeCoords.y));
            jCoords["lat"] = locations[loc].lat;
            jCoords["lon"] = locations[loc].lon;

            pointSet->GetBoundingBox(loc, locationWeatherData.GetPointsPerRow(), nearPoints);

            for(auto i : locationWeatherData.GetValidFieldDataIndexes())
            {
                auto forecastIndex = locationWeatherData.GetIndexOfKnownValidFieldIndex(i);
                auto forecastTime = localForecastTimes[forecastIndex];
                auto currentDay = ToLocalTm(forecastTime).tm_mday;
                if(!jSunriseSunset[to_string(currentDay)])
                {
                    Json::Value jRiseSetValues;
                    #pragma omp critical
                    cout << "Adding Sunrise/Sunset info for " << locations[loc].name << " on " << GetShortDate(forecastTime) << endl;
                    auto sunriseSunset = Astronomy::GetSunRiseSunset(locations[loc].lat, locations[loc].lon, forecastTime);
                    jRiseSetValues["rise"] = Json::Value::Int64(system_clock::to_time_t(sunriseSunset.rise));
                    jRiseSetValues["set"] = Json::Value::Int64(system_clock::to_time_t(sunriseSunset.set));

                    jSunriseSunset[to_string(currentDay)] = jRiseSetValues;
                }

                Wx boundsWx[4], result;
                PrecipitationType typesSeen = PrecipitationType::None;
                for(auto k = 0; k < 4; k++)
                {
                    boundsWx[k] = locationWeatherData.GetWxAtKnownValidIndexes(i, nearPoints[k].index);
                    typesSeen |= boundsWx[k].type;
                }
                
                if((typesSeen & PrecipitationType::FreezingRain) == PrecipitationType::FreezingRain)
                {
                    result.type = PrecipitationType::FreezingRain;
                    precipitationType[forecastIndex] = "ice";
                }
                else if((typesSeen & PrecipitationType::Snow) == PrecipitationType::Snow)
                {
                    result.type = PrecipitationType::Snow;
                    precipitationType[forecastIndex] = "snow";
                }
                else if((typesSeen & PrecipitationType::Rain) == PrecipitationType::Rain)
                {
                    result.type = PrecipitationType::Rain;
                    precipitationType[forecastIndex] = "rain";
                }
                else
                {
                    result.type = PrecipitationType::None;
                    precipitationType[forecastIndex] = "";
                }

                LocalForecast(dewpoint);
                LocalForecast(precipitationRate);
                LocalForecast(gust);
                LocalForecast(lightning);
                LocalForecast(newPrecipitation);
                LocalForecast(pressure);
                LocalForecast(temperature);
                LocalForecast(totalCloudCover);
                LocalForecast(totalPrecipitation);
                LocalForecast(totalSnow);
                LocalForecast(visibility);
                LocalForecast(windU);
                LocalForecast(windV);
                LocalForecast(windSpeed);

                dewpoint[forecastIndex] = static_cast<int16_t>(result.dewpoint);
                gust[forecastIndex] = static_cast<uint16_t>(result.gust);
                lightning[forecastIndex] = static_cast<uint16_t>(result.lightning);
                newPrecipitation[forecastIndex] = max(0.0, result.newPrecipitation);
                precipitationRate[forecastIndex] = max(0.0, ScaledValueForTypeAndTemp(result.type, result.precipitationRate, result.temperature));
                pressure[forecastIndex] = result.pressure;
                temperature[forecastIndex] = static_cast<int16_t>(result.temperature);
                totalCloudCover[forecastIndex] = static_cast<uint16_t>(result.totalCloudCover);
                totalPrecipitation[forecastIndex] = max(0.0, result.totalPrecipitation);
                totalSnow[forecastIndex] = result.totalSnow = max(lastResult.totalSnow, result.totalSnow);
                visibility[forecastIndex] = static_cast<uint16_t>(result.visibility);
                windDirection[forecastIndex] = static_cast<uint16_t>(result.WindDirection());
                windSpeed[forecastIndex] = static_cast<uint16_t>(parent->wxModel == WeatherModel::HRRR ? result.windSpeed : result.WindSpeed());

                lastResult = result;

                forecastIndex++;
            }

            jWx["dewpoint"] = dewpoint;
            jWx["precipRate"] = precipitationRate; 
            jWx["precipType"] = precipitationType; 
            jWx["gust"] = gust; 
            jWx["lightning"] = lightning; 
            jWx["newPrecip"] = newPrecipitation;
            jWx["pressure"] = pressure; 
            jWx["temperature"] = temperature; 
            jWx["totalCloudCover"] = totalCloudCover; 
            jWx["totalPrecip"] = totalPrecipitation;
            jWx["totalSnow"] = totalSnow;
            jWx["vis"] = visibility;
            jWx["windDir"] = windDirection;
            jWx["windSpd"] = windSpeed;

            jLocation["coords"] = jCoords;
            jLocation["sun"] = jSunriseSunset;
            jLocation["wx"] = jWx;
            jLocation["isCity"] = locations[loc].isCity;

            #pragma omp critical
            jLocations[locations[loc].name] = jLocation;
        }
        root["locations"] = jLocations;
        root["moon"] = jLunarPhase;
        root["forecastTimes"] = jForecastTimes;
    }
};

GribProcessor::GribProcessor(string gribPathTemplate, WeatherModel wxModel, std::chrono::system_clock::time_point forecastStartTime, uint16_t maxGribIndex, GeographicCalcs& geoCalcs, uint16_t skipToGribNumber) 
    : gribPathTemplate(gribPathTemplate), 
    wxModel(wxModel), 
    forecastStartTime(forecastStartTime), 
    maxGribIndex(maxGribIndex), 
    skipToGribNumber(skipToGribNumber), 
    geoCalcs(geoCalcs) {}

void GribProcessor::Process(Json::Value& root, LocationWeatherData& locationWeatherData)
{    
    ParallelProcessor processor(this, locationWeatherData);
    processor.Initalize();
    processor.CollectData();
    processor.GenerateForecastJson(root);

    cout << "Done with Grib Data." << endl;
}
