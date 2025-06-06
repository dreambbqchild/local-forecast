#include "Astronomy/Astronomy.h"
#include "BarycentricCoordinates.h"
#include "Characters.h"
#include "DateTime.h"
#include "Data/StringCache.h"
#include "Error.h"
#include "GribData.h"
#include "GribReader.h"
#include "NumberFormat.h"

#include <eccodes.h>
#include <filesystem>
#include <iostream>
#include <omp.h>
#include <stack>

using namespace std;
using namespace chrono;
namespace fs = std::filesystem;

const Wx emptyWx = {PrecipitationType::NoPrecipitation};

struct LocalForecastPayload {
    const Vector2d bounds[4];
    const double values[4];
};

#define GetString(fieldData, property) \
{ \
    char __localBuffer[128] = {0}; \
    size_t __length = 128; \
    CODES_CHECK(codes_get_string(h, #property, __localBuffer, &__length), "Unable to get " #property); \
    fieldData.property = GetCachedString(__localBuffer); \
}

inline int32_t GetInt(codes_handle* h, const char* property)
{
    long value = 0; \
    codes_get_long(h, property, &value); \
    return static_cast<int32_t>(value); \
}

#define GetInt(property, result) result = GetInt(h, property)

inline double LocalForecast(const Vector2d& v, const LocalForecastPayload& payload)
{
    double resultWeights[4] = {0};
    if(!BarycentricCoordinatesForCWTetrahedron(v, payload.bounds, resultWeights))
        ERR_OUT("Forecast edge hit. Please choose another point.");

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

#define ShortNameIs(value) !strcmp(fieldData.shortName, value)
#define TypeOfLevelIs(value) !strcmp(fieldData.typeOfLevel, value)
#define LevelIs(value) !strcmp(fieldData.level, value)
#define FOR_FORECASTS_IN_RANGE(i) for(auto i = skipToGribNumber; i <= maxGribIndex; i++)

class GribReader : public IGribReader
{
private:
    uint16_t skipToGribNumber, maxGribIndex, totalDays;
    int32_t forecastTimeIndex = 0;
    string gribPathTemplate;
    system_clock::time_point forecastStartTime;
    vector<tuple<string, Location>> locations;
    GeographicCalcs geoCalcs;

    WeatherModel wxModel;
    vector<int32_t> validIndexes;
    
    unordered_map<int32_t, GeoCoord> geoCoords;
    unordered_map<GeoCoord, int32_t> geoCoordLookup;
    vector<system_clock::time_point> localForecastTimes;
    vector<QuadIndexes> quads;
    vector<unordered_map<int32_t, vector<FieldData>>> rawFieldData;

    GeoBounds geoBounds;

    inline bool IsInArea(const GeoBounds& geoBounds, const GeoCoord& coord) { return IsBetween(geoBounds.bottomLat, coord.lat, geoBounds.topLat) && IsBetween(geoBounds.leftLon, coord.lon, geoBounds.rightLon); }

    inline bool CheckGeoCoordsIndex(int32_t index) { return geoCoords.find(index) != geoCoords.end();}

    inline void PopPointsFromStackToVector(vector<int32_t>& validIndexes, stack<int32_t>& forNorthToSouth)
    {
        while(!forNorthToSouth.empty())
        {
            validIndexes.insert(validIndexes.begin(), forNorthToSouth.top());
            forNorthToSouth.pop();
        }
    }

    FILE* OpenFile(const string& fileName)
    {
        auto file = fopen(fileName.c_str(), "rb");

        if (!file)
            ERR_OUT(fileName << " not found.");

        return file;
    }

    bool ReadGribFile(const string& fileName)
    {    
        int32_t err = 0;
        codes_handle* h = nullptr;

        if(!fs::exists(fileName))
            return false;

        rawFieldData.resize(rawFieldData.size() + 1);
        auto& currentFieldData = rawFieldData[rawFieldData.size() - 1];

        if(!geoCoords.size())
            PrepareParallelDataBasedOffOfFile(fileName);

        auto file = OpenFile(fileName);
        int32_t overallIndex = 0;
        while ((h = codes_handle_new_from_file(nullptr, file, PRODUCT_GRIB, &err)) != nullptr)
        {
            int32_t numberOfPoints = 0;
            FieldData fieldData = {0};

            GetString(fieldData, name)
            GetString(fieldData, level)
            GetString(fieldData, shortName)
            GetString(fieldData, stepRange)
            GetString(fieldData, typeOfLevel)
            GetInt("parameterNumber", fieldData.parameterNumber);
            GetInt("numberOfPoints", numberOfPoints);

            unique_ptr<double[]> lats(new double[numberOfPoints]);
            unique_ptr<double[]> lons(new double[numberOfPoints]);
            unique_ptr<double[]> values(new double[numberOfPoints]);
            codes_grib_get_data(h, lats.get(), lons.get(), values.get());
            
            for(auto& index : validIndexes)
            {
                fieldData.value = values[index];
                currentFieldData[index].push_back(fieldData);
            }

            codes_handle_delete(h);
        }

        fclose(file);
        return true;
    }

    void CollectGeoCoordsInBounds(FILE* file, int32_t& columns, vector<int32_t>& validIndexes)
    {
        int32_t err = 0, overallIndex = 0;
        stack<int32_t> forNorthToSouth;
        codes_handle* h = codes_handle_new_from_file(nullptr, file, PRODUCT_GRIB, &err);
        if (err != CODES_SUCCESS) 
            CODES_CHECK(err, 0);
            
        codes_iterator* iter = codes_grib_iterator_new(h, 0, &err);
        if (err != CODES_SUCCESS) 
            CODES_CHECK(err, 0);
        
        if(wxModel == WeatherModel::HRRR)
            GetInt("Nx", columns);
        else
            GetInt("Ni", columns);

        double value = 0;
        GeoCoord coord = {0}, lastCoord = { 361.0, 361.0 };
        while (codes_grib_iterator_next(iter, &coord.lat, &coord.lon, &value))
        {
            auto currentOverallIndex = overallIndex++;
            if(!IsInArea(geoBounds, coord))
                continue;

            //Assuming default of WE:SN coming out of the GRIB, but doing a thing to work with the data as WE:NS
            if(coord.lon < lastCoord.lon) 
                PopPointsFromStackToVector(validIndexes, forNorthToSouth);

            geoCoords[currentOverallIndex] = coord;
            forNorthToSouth.emplace(currentOverallIndex);
            lastCoord = coord;
        }

        PopPointsFromStackToVector(validIndexes, forNorthToSouth);

        codes_grib_iterator_delete(iter);
        codes_handle_delete(h);
        h = nullptr;
    }    

    void BuildQuads(int32_t columns, vector<int32_t>& validIndexes)
    {
        for(auto& validIndex : validIndexes)
        {
            QuadIndexes quad = {
                .topLeft = validIndex,
                .topRight = validIndex + 1,
                .bottomLeft = validIndex - columns,
                .bottomRight = validIndex - columns + 1 
            };

            geoCoordLookup[geoCoords[validIndex]] = validIndex;

            if(CheckGeoCoordsIndex(quad.topLeft) && CheckGeoCoordsIndex(quad.topRight) && CheckGeoCoordsIndex(quad.bottomLeft) && CheckGeoCoordsIndex(quad.bottomRight))
            {
                auto topLeft = geoCoords[quad.topLeft], topRight = geoCoords[quad.topRight];
                if(topLeft.lon < topRight.lon) //Protect against wrap around.
                    quads.push_back(quad);
            }
        }
    }

    void PrepareParallelDataBasedOffOfFile(const string& fileName)
    {
        int32_t columns = 0;    
        auto file = OpenFile(fileName);
        CollectGeoCoordsInBounds(file, columns, validIndexes);
        fclose(file);

        BuildQuads(columns, validIndexes);
    }

    void Initalize(unique_ptr<IForecastRepo>& forecastRepo)
    {
        int32_t lastDay = INT32_MAX;
        cout << "Reading in grib files..." << endl;
        //Read in files one at a time because that's the way it has to be due to eccodes.
        FOR_FORECASTS_IN_RANGE(i)
        {
            string path(gribPathTemplate.length() + 2, '\0');
            snprintf((char*)path.c_str(), gribPathTemplate.length() + 2, gribPathTemplate.c_str(), i);
            
            if(!ReadGribFile(path))
                continue;

            auto forecastAtPoint = forecastStartTime + hours(i);
            localForecastTimes.push_back(forecastAtPoint);

            forecastRepo->AddForecastStartTime(forecastAtPoint);

            auto currentDay = ToLocalTm(forecastAtPoint).tm_mday;
            if(currentDay != lastDay)
            {
                cout << "Adding Lunar Information for " << GetShortDate(forecastAtPoint) << "..." << endl;
                auto lunarPhase = Astronomy::GetLunarPhase(forecastAtPoint);
                forecastRepo->AddLunarPhase(currentDay, lunarPhase);
                lastDay = currentDay;
                totalDays++;
            }
        }
    }

    GribData* GetCompiledGribData()
    {
        vector<unordered_map<int32_t, WxAtGeoCoord>> wxResults;
        wxResults.resize(rawFieldData.size());

        cout << "Collecting data..." << endl;        
        #pragma omp parallel for
        for(auto itr = rawFieldData.begin(); itr != rawFieldData.end(); itr++)
        {            
            auto i = itr - rawFieldData.begin();
            auto& wxResult = wxResults[i];
            const auto& fieldDatums = *itr;
            for(const auto& kvp : fieldDatums)
            {
                Wx wx = {};
                for (const auto &fieldData : kvp.second)
                {                    
                    if(ShortNameIs("crain") && fieldData.value)
                        wx.type |= PrecipitationType::Rain;
                    else if(ShortNameIs("cfrzr") && fieldData.value)
                        wx.type |= PrecipitationType::FreezingRain;
                    else if(ShortNameIs("csnow") && fieldData.value)
                        wx.type |= PrecipitationType::Snow;
                    else if(ShortNameIs("prate"))
                        wx.precipitationRate = ToInPerHour(fieldData.value);
                    else if(ShortNameIs("tp"))
                    {
                        if(fieldData.stepRange[0] == '0' && fieldData.stepRange[1] == '-')
                            wx.totalPrecipitation = ToInchesFromKgPerSquareMeter(fieldData.value);
                        else
                            wx.newPrecipitation = ToInchesFromKgPerSquareMeter(fieldData.value);
                    }
                    else if(ShortNameIs("sde"))
                        wx.snowDepth = ToInchesFromMeters(fieldData.value);
                    else if(ShortNameIs("asnow") || (wxModel == WeatherModel::HRRR && fieldData.parameterNumber == 29))
                        wx.totalSnow = ToInchesFromMeters(fieldData.value);
                    else if(ShortNameIs("2d"))
                        wx.dewpoint = ToFarenheight(fieldData.value);
                    else if(wxModel == WeatherModel::HRRR && ShortNameIs("2t"))
                        wx.temperature = ToFarenheight(fieldData.value);
                    else if(wxModel == WeatherModel::GFS && ShortNameIs("t"))
                        wx.temperature = ToFarenheight(fieldData.value);
                    else if(ShortNameIs("tcc") && TypeOfLevelIs("atmosphere"))
                        wx.totalCloudCover = fieldData.value;
                    else if(ShortNameIs("vis") && (wxModel == WeatherModel::HRRR || TypeOfLevelIs("surface")))
                        wx.visibility = ToMiles(fieldData.value);
                    else if(wxModel == WeatherModel::HRRR && ShortNameIs("10si"))
                        wx.windSpeed = ToMPH(fieldData.value);
                    else if(ShortNameIs("gust"))
                        wx.gust = ToMPH(fieldData.value);
                    else if(ShortNameIs("10u"))
                        wx.windU = fieldData.value;
                    else if(ShortNameIs("10v"))
                        wx.windV = fieldData.value;
                    else if(wxModel == WeatherModel::HRRR && ShortNameIs("mslma"))
                        wx.pressure = ToInHg(fieldData.value);
                    else if(wxModel == WeatherModel::GFS && ShortNameIs("mslet"))
                        wx.pressure = ToInHg(fieldData.value);
                    else if(ShortNameIs("ltng"))
                        wx.lightning = fieldData.value;
                }

                wxResult[kvp.first] = {
                    .coord = geoCoords[kvp.first],
                    .wx = wx
                };
            }
        }

        if(wxModel == WeatherModel::GFS)
        {
            for(auto fileItr = wxResults.begin() + 1; fileItr != wxResults.end(); fileItr++)
            {
                auto& currentMap = *fileItr;
                auto& previousMap = *(fileItr - 1);

                #pragma omp parallel for
                for(auto& index : validIndexes)
                {
                    auto& pWx = currentMap[index].wx;
                    auto& pPreviousWx = previousMap[index].wx;

                    pWx.totalSnow = pPreviousWx.totalSnow;
                    if(pWx.snowDepth > pPreviousWx.snowDepth)
                        pWx.totalSnow += (pWx.snowDepth - pPreviousWx.snowDepth);
                }
            }
        }

        return new GribData(validIndexes, quads, geoCoordLookup, wxResults);
    }

    void GenerateForecast(unique_ptr<GribData>& gribData, unique_ptr<IForecastRepo>& forecastRepo)
    {
        auto geoCoords = gribData->GetGeoCoords();
        auto pointSet = unique_ptr<IGeoPointSet>(AllocGeoPointSet(geoCoords, geoCalcs));        
        cout << "Compiling JSON..." << endl;

        {
            auto forecastIndex = 0;
            for(int32_t i = 0; i < gribData->GetNumberOfFiles(); i++)
            {
                auto imgSuffix = ToStringWithPad(3, '0', i);
            }
        }

        #pragma omp parallel for
        for(auto& kvp : locations)
        {
            auto locationKey = get<0>(kvp);
            auto location = get<1>(kvp);

            GeoCoordPoint nearPoints[4] = {0};
            Wx lastResult = {};

            auto homeCoords = geoCalcs.FindXY({location.coords.lat, location.coords.lon});
            location.coords.x = static_cast<uint16_t>(round(homeCoords.x)); 
            location.coords.y = static_cast<uint16_t>(round(homeCoords.y));

            pointSet->GetBoundingBox(location, nearPoints);
            location.wxLen = gribData->GetNumberOfFiles();
            location.wx = static_cast<WxSingle*>(calloc(location.wxLen, sizeof(WxSingle)));
            location.sunsLen = totalDays;
            location.suns = static_cast<LabeledSun*>(calloc(location.sunsLen, sizeof(LabeledSun)));

            auto sunIndex = -1;
            for(int32_t forecastIndex = 0; forecastIndex < location.wxLen; forecastIndex++)
            {
                auto forecastTime = localForecastTimes[forecastIndex];
                auto currentDay = ToLocalTm(forecastTime).tm_mday;
                if(sunIndex == -1 || location.suns[sunIndex].day != currentDay)
                {
                    #pragma omp critical
                    cout << "Adding Sunrise/Sunset info for " << locationKey << " on " << GetShortDate(forecastTime) << endl;        

                    auto labeledSun = &location.suns[++sunIndex];
                    labeledSun->day = currentDay;                                
                    auto sunriseSunset = Astronomy::GetSunRiseSunset(location.coords.lat, location.coords.lon, forecastTime);
                    labeledSun->sun.rise = system_clock::to_time_t(sunriseSunset.rise);
                    labeledSun->sun.set = system_clock::to_time_t(sunriseSunset.set);
                }

                Wx boundsWx[4], result;
                PrecipitationType typesSeen = PrecipitationType::NoPrecipitation;
                for(auto k = 0; k < 4; k++)
                {
                    boundsWx[k] = gribData->GetWxAtGeoCoord(nearPoints[k], forecastIndex).wx;
                    typesSeen |= boundsWx[k].type;
                }
                
                if((typesSeen & PrecipitationType::FreezingRain) == PrecipitationType::FreezingRain)
                    result.type = PrecipitationType::FreezingRain;
                else if((typesSeen & PrecipitationType::Snow) == PrecipitationType::Snow)
                    result.type = PrecipitationType::Snow;
                else if((typesSeen & PrecipitationType::Rain) == PrecipitationType::Rain)
                    result.type = PrecipitationType::Rain;
                else
                    result.type = PrecipitationType::NoPrecipitation;

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

                location.wx[forecastIndex] = WxSingle {
                    .dewpoint = static_cast<int16_t>(result.dewpoint),
                    .gust = static_cast<uint16_t>(result.gust),
                    .lightning = static_cast<uint16_t>(result.lightning),
                    .newPrecipitation = max(0.0, result.newPrecipitation),
                    .precipitationRate = max(0.0, ScaledValueForTypeAndTemp(result.type, result.precipitationRate, result.temperature)),
                    .precipitationType = result.type,
                    .pressure = result.pressure,
                    .temperature = static_cast<int16_t>(result.temperature),
                    .totalCloudCover = static_cast<uint16_t>(result.totalCloudCover),
                    .totalPrecipitation = max(0.0, result.totalPrecipitation),
                    .totalSnow = result.totalSnow = max(lastResult.totalSnow, result.totalSnow),
                    .visibility = static_cast<uint16_t>(result.visibility),
                    .windDirection = static_cast<uint16_t>(result.WindDirection()),
                    .windSpeed = static_cast<uint16_t>(wxModel == WeatherModel::HRRR ? result.windSpeed : result.WindSpeed())
                };

                lastResult = result;
            }
            
            forecastRepo->AddLocation(locationKey, location);
            free(location.wx);
            free(location.suns);
        }
    }

public:
    GribReader(string gribPathTemplate, const SelectedRegion& selectedRegion, WeatherModel wxModel, system_clock::time_point forecastStartTime, uint16_t skipToGribNumber, uint16_t maxGribIndex, GeographicCalcs& geoCalcs) 
        : gribPathTemplate(gribPathTemplate), geoBounds(selectedRegion.GetRegionBoundsWithOverflow()), locations(selectedRegion.GetAllLocations()), wxModel(wxModel), forecastStartTime(forecastStartTime), skipToGribNumber(skipToGribNumber), maxGribIndex(maxGribIndex), totalDays(0), geoCalcs(geoCalcs)
    {
        if(this->geoBounds.leftLon < 0)
            this->geoBounds.leftLon += 360;

        if(this->geoBounds.rightLon < 0)
            this->geoBounds.rightLon += 360;
    }    

    void CollectData(unique_ptr<IForecastRepo>& forecastRepo, std::unique_ptr<GribData>& gribData)
    {
        Initalize(forecastRepo);
        gribData = unique_ptr<GribData>(GetCompiledGribData());
        GenerateForecast(gribData, forecastRepo);

        cout << "Done collecting Grib data!" << endl;
    }
};

IGribReader* AllocGribReader(string gribPathTemplate, const SelectedRegion& selectedRegion, WeatherModel wxModel, system_clock::time_point forecastStartTime, uint16_t skipToGribNumber, uint16_t maxGribIndex, GeographicCalcs& geoCalcs)
{
    return new GribReader(gribPathTemplate, selectedRegion, wxModel, forecastStartTime, skipToGribNumber, maxGribIndex, geoCalcs);
}