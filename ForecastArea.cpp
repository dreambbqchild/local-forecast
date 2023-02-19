#include "declarations.h"
#include "calcs.h"
#include "mapcolors.h"
#include <omp.h>
#include <string.h>
#include <unordered_map>
#include <fstream>
#include <filesystem>

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/Constants.hpp>
#include <GeographicLib/NearestNeighbor.hpp>
#include <GeographicLib/TransverseMercator.hpp>

#include <jsoncpp/json/json.h>

using namespace std;
using namespace GeographicLib;
namespace fs = std::filesystem;

#define ShortNameIs(value) !strcmp(fieldData.shortName, value)
#define TypeOfLevelIs(value) !strcmp(fieldData.typeOfLevel, value)
#define LevelIs(value) !strcmp(fieldData.level, value)
#define FinishImage(img) img.InterpolateFill(pointsPerRow); img.Save()
#define LocalForecast(property) result.property = Blerp( \
    {nearPoints[0].x, nearPoints[0].y, boundsWx[0].property}, \
    {nearPoints[1].x, nearPoints[1].y, boundsWx[1].property}, \
    {nearPoints[2].x, nearPoints[2].y, boundsWx[2].property}, \
    {nearPoints[3].x, nearPoints[3].y, boundsWx[3].property}, \
    homeCoords \
)

extern Location locations[];
extern const int locationsLength;
const Wx emptyWx = {PrecipitationType::None};

Geodesic geod(Constants::WGS84_a(), Constants::WGS84_f());

class DistanceCalculator {
public:
  double operator() (const GeoCoord& a, const GeoCoord& b) const {
    double d;
    geod.Inverse(a.lat, a.lon, b.lat, b.lon, d);
    if ( !(d >= 0) )
      throw GeographicErr("distance doesn't satisfy d >= 0");
    return d;
  }
} distanceCalc;

class MinnesotaUTM {
private:
    TransverseMercator tm;
    double centralLon, falseeasting, leftX, rightX, topY, bottomY, width, height, xScale, yScale;
    
    void ForwardInternal(double lat, double lon, double& x, double& y)
    {
        tm.Forward(centralLon, lat, lon, x, y);
        x += falseeasting; 
    }

public:
  MinnesotaUTM(uint32_t imageWidth = 0, uint32_t imageHeight = 0)
    : tm(Constants::WGS84_a(), 1/297.0, Constants::UTM_k0()), centralLon(6 * 15 - 183), falseeasting(5e5), leftX(0), topY(0), rightX(0), bottomY(0), width(0), height(0)
    {
        if(imageWidth == 0 || imageHeight == 0)
            return;

        ForwardInternal(toplat, leftlon, leftX, topY);
        ForwardInternal(bottomlat, rightlon, rightX, bottomY);
        width = rightX - leftX;
        height = topY - bottomY;
        xScale = imageWidth / width;
        yScale = imageHeight / height;
    }

    void Forward(double lat, double lon, double &x, double& y)
    {
        ForwardInternal(lat, lon, x, y);
        x = x - leftX;
        y = y - bottomY;
        y = height - y; //Remember: inverted because bigger values are more north.
        x *= xScale;
        y *= yScale;
    }
} minnesotaUtm;

bool operator==(const Point& lhs, const Point& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

string GetIsoDateTime(time_t forecastStartTime, uint16_t hrOffset)
{
    char isoDate[25] = {0};
    forecastStartTime += secondsInHour * hrOffset;
    tm forecastStart = *gmtime(&forecastStartTime);

    strftime(isoDate, 25, "%Y-%m-%dT%H:00:00.000Z", &forecastStart);
    return isoDate;
}

void GetBoundingBox(uint32_t loc, NearestNeighbor<double, GeoCoord, DistanceCalculator>& pointset, vector<GeoCoord> geoCoords, function<Point(double, double)> FindXY, uint32_t pointsPerRow, IndexedPoint resultIndexes[4])
{
    vector<int32_t> indexes;
    vector<IndexedPoint> nearPoints;
    pointset.Search(geoCoords, distanceCalc, { locations[loc].lat, locations[loc].lon}, indexes, 6);
    for(auto &i : indexes)
    {
        auto geoCoord = geoCoords[i];
        auto pt = FindXY(geoCoord.lat, geoCoord.lon);
        nearPoints.push_back({pt.x, pt.y, static_cast<uint16_t>(i)});
    }
    
    std::sort(nearPoints.begin(), nearPoints.end(), [](IndexedPoint l, IndexedPoint r){return l.index < r.index;});

    resultIndexes[0] = nearPoints.at(0);
    for(auto i = 1; i < 6; i++)
    {
        auto pt = nearPoints.at(i);
        if(pt.index == resultIndexes[0].index + 1)
            resultIndexes[1] = pt;
        else if(pt.index == resultIndexes[0].index + pointsPerRow)
            resultIndexes[2] = pt;
        else if(pt.index == resultIndexes[0].index + pointsPerRow + 1)
            resultIndexes[3] = pt;
    }
}

ForecastArea::ForecastArea(string gribPathTemplate, WeatherModel wxModel, time_t forecastStartTime, string forecastDataOutputDir, uint16_t maxGribIndex, uint16_t skipToGribNumber) 
    : gribPathTemplate(gribPathTemplate), wxModel(wxModel), forecastStartTime(forecastStartTime), forecastDataOutputDir(forecastDataOutputDir), maxGribIndex(maxGribIndex), skipToGribNumber(skipToGribNumber), imageHeight(1164)
{
    auto line = geod.InverseLine(toplat, leftlon, toplat, rightlon);
    metersWidth = line.Distance();

    line = geod.InverseLine(toplat, leftlon, bottomlat, leftlon);
    metersHeight = line.Distance();
    imageWidth = static_cast<uint32_t>(ceil((metersWidth / metersHeight) * imageHeight));

    minnesotaUtm = MinnesotaUTM(imageWidth, imageHeight);
}

Point ForecastArea::FindXY(double lat, double lon)
{
    Point pt = {0};
    minnesotaUtm.Forward(lat, lon, pt.x, pt.y);
    return pt;
}

void ForecastArea::Process()
{
    auto allData = new vector<FieldData>[maxGribIndex + 1];
    auto wxRegistries = new unordered_map<uint16_t, Wx>[maxGribIndex + 1];
    Json::Value forecastTimes;
    vector<GeoCoord> geoCoords;
    int32_t pointsPerRow = 0, geoCoordsLength = 0, forecastTimeIndex = 0;

    cout << "Reading in grib files..." << endl;
    //Read in files one at a time because that's the way it has to be due to eccodes.
    for(auto i = skipToGribNumber; i <= maxGribIndex; i++)
    {
        string path(gribPathTemplate.length() + 2, '\0');
        sprintf((char*)path.c_str(), gribPathTemplate.c_str(), i);

        GribReader reader(path, wxModel);
        allData[i] = reader.GetFieldData();
        if(!allData[i].size())
            continue;

        if(i == skipToGribNumber)
        {
            geoCoords = reader.GetGeoCoords();
            pointsPerRow = reader.GetColumns();
            geoCoordsLength = reader.GetNumberOfValues();
        }

        forecastTimes[forecastTimeIndex++] = GetIsoDateTime(forecastStartTime, i);
    }

    cout << "Collecting data & generating images..." << endl;
    //Generate Base Data
    #pragma omp parallel for
    for(auto i = skipToGribNumber; i <= maxGribIndex; i++)
    {
        if(!allData[i].size())
            continue;

        //Normalize
        for (auto &fieldData : allData[i]) 
        {
            auto pWx = &wxRegistries[i][fieldData.index];
            
            if(ShortNameIs("crain") && fieldData.value)
                pWx->type |= PrecipitationType::Rain;
            else if(ShortNameIs("cfrzr") && fieldData.value)
                pWx->type |= PrecipitationType::FreezingRain;
            else if(ShortNameIs("csnow") && fieldData.value)
                pWx->type |= PrecipitationType::Snow;
            else if(ShortNameIs("prate"))
                pWx->precipitationRate = ToInPerHour(fieldData.value);
            else if(ShortNameIs("tp"))
            {
                if(fieldData.stepRange[0] == '0' && fieldData.stepRange[1] == '-')
                    pWx->totalPrecipitation = ToInchesFromKgPerSquareMeter(fieldData.value);
                else
                    pWx->newPrecipitation = ToInchesFromKgPerSquareMeter(fieldData.value);
            }
            else if(ShortNameIs("sde"))
                pWx->snowDepth = ToInchesFromMeters(fieldData.value);
            else if(ShortNameIs("asnow"))
                pWx->totalSnow = ToInchesFromMeters(fieldData.value);
            else if(ShortNameIs("2d"))
                pWx->dewpoint = ToFarenheight(fieldData.value);
            else if(ShortNameIs("2t"))
                pWx->temperature = ToFarenheight(fieldData.value);
            else if(ShortNameIs("tcc") && TypeOfLevelIs("atmosphere"))
                pWx->totalCloudCover = fieldData.value;
            else if(ShortNameIs("vis") && (wxModel == WeatherModel::HRRR || TypeOfLevelIs("surface")))
                pWx->visibility = ToMiles(fieldData.value);
            else if(wxModel == WeatherModel::HRRR && ShortNameIs("10si"))
                pWx->windSpeed = ToMPH(fieldData.value);
            else if(ShortNameIs("gust"))
                pWx->gust = ToMPH(fieldData.value);
            else if(wxModel == WeatherModel::HRRR && ShortNameIs("10u"))
                pWx->windU = fieldData.value;
            else if(wxModel == WeatherModel::GFS && ShortNameIs("u") && TypeOfLevelIs("heightAboveGround") && LevelIs("20"))
                pWx->windU = fieldData.value;
            else if(wxModel == WeatherModel::HRRR && ShortNameIs("10v"))
                pWx->windV = fieldData.value;
            else if(wxModel == WeatherModel::GFS && ShortNameIs("v") && TypeOfLevelIs("heightAboveGround") && LevelIs("20"))
                pWx->windV = fieldData.value;
            else if(wxModel == WeatherModel::HRRR && ShortNameIs("mslma"))
                pWx->pressure = ToInHg(fieldData.value);
            else if(wxModel == WeatherModel::GFS && ShortNameIs("mslet"))
                pWx->pressure = ToInHg(fieldData.value);
            else if(ShortNameIs("ltng"))
                pWx->lightning = fieldData.value;
        }

        //Generate Output Images
        char temperatureFileName[32] = {0}, 
             precipFileName[32] = {0};

        sprintf(temperatureFileName, "temperature-%03d.png", i);
        sprintf(precipFileName, "precip-%03d.png", i);

        ForecastImage temperatureImg(fs::path(forecastDataOutputDir) / temperatureFileName, imageWidth, imageHeight);
        ForecastImage precipImg(fs::path(forecastDataOutputDir) / precipFileName, imageWidth, imageHeight);
        for(auto k = 0; k < geoCoordsLength; k++)
        {
            GeoCoord coords = geoCoords[k];
            Point pt = FindXY(coords.lat, coords.lon);
            Wx wx = wxRegistries[i][k];

            MapColor c = ColorFromDegrees(wx.temperature);
            temperatureImg.SetPixel(pt.x, pt.y, c.r, c.g, c.b);

            c = ColorFromPrecipitation(wx.type, ScaledValueForTypeAndTemp(wx.type, wx.precipitationRate, wx.temperature));
            precipImg.SetPixel(pt.x, pt.y, c.r, c.g, c.b, c.a);
        }
        FinishImage(temperatureImg);
        FinishImage(precipImg);
    }

    if(wxModel == WeatherModel::GFS)
    {
        auto lastI = skipToGribNumber;
        for(auto i = skipToGribNumber + 1; i <= maxGribIndex; i++)
        {
            if(!allData[i].size())
                continue;

            #pragma omp parallel for
            for(auto k = 0; k < geoCoordsLength; k++)
            {
                auto pWx = &wxRegistries[i][k];
                auto pLastWx = &wxRegistries[lastI][k];

                pWx->totalSnow = pLastWx->totalSnow;
                if(pWx->snowDepth > pLastWx->snowDepth)
                    pWx->totalSnow += (pWx->snowDepth - pLastWx->snowDepth);
            }
            lastI = i;
        }
    }

    cout << "Compiling forecast.json..." << endl;
    //Now for our local forecasts...
    NearestNeighbor<double, GeoCoord, DistanceCalculator> pointset;
    pointset.Initialize(geoCoords, distanceCalc);
    Json::Value root, jLocations;

    #pragma omp parallel for
    for(auto loc = 0; loc < locationsLength; loc++)
    {
        IndexedPoint nearPoints[4] = {0};        
        Json::Value jLocation, jCoords, jWx, dewpoint, precipitationRate, precipitationType, gust, lightning, newPrecipitation, pressure, temperature, totalCloudCover, totalPrecipitation, totalSnow, visibility, windDirection, windSpeed;
        Wx lastResult = {};
                
        auto homeCoords = FindXY(locations[loc].lat, locations[loc].lon);
        jCoords["x"] = static_cast<uint16_t>(round(homeCoords.x)); 
        jCoords["y"] = static_cast<uint16_t>(round(homeCoords.y));
        jCoords["lat"] = locations[loc].lat;
        jCoords["lon"] = locations[loc].lon;

        GetBoundingBox(loc, pointset, geoCoords, [this](double dx, double dy) {return this->FindXY(dx, dy);}, pointsPerRow, nearPoints);

        for(uint32_t i = skipToGribNumber, j = 0; i <= maxGribIndex; i++, j++)
        {
            if(!allData[i].size()) {
                j--;
                continue;
            }

            Wx boundsWx[4], result;
            PrecipitationType typesSeen = PrecipitationType::None;
            for(auto k = 0; k < 4; k++)
            {
                boundsWx[k] = wxRegistries[i][nearPoints[k].index];
                typesSeen |= boundsWx[k].type;
            }
            
            if((typesSeen & PrecipitationType::FreezingRain) == PrecipitationType::FreezingRain)
            {
                result.type = PrecipitationType::FreezingRain;
                precipitationType[j] = "ice";
            }
            else if((typesSeen & PrecipitationType::Snow) == PrecipitationType::Snow)
            {
                result.type = PrecipitationType::Snow;
                precipitationType[j] = "snow";
            }
            else if((typesSeen & PrecipitationType::Rain) == PrecipitationType::Rain)
            {
                result.type = PrecipitationType::Rain;
                precipitationType[j] = "rain";
            }
            else
            {
                result.type = PrecipitationType::None;
                precipitationType[j] = "";
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

            dewpoint[j] = static_cast<int16_t>(result.dewpoint);
            gust[j] = static_cast<uint16_t>(result.gust);
            lightning[j] = static_cast<uint16_t>(result.lightning);
            newPrecipitation[j] = max(0.0, result.newPrecipitation);
            precipitationRate[j] = max(0.0, ScaledValueForTypeAndTemp(result.type, result.precipitationRate, result.temperature));
            pressure[j] = result.pressure;
            temperature[j] = static_cast<int16_t>(result.temperature);
            totalCloudCover[j] = static_cast<uint16_t>(result.totalCloudCover);
            totalPrecipitation[j] = max(0.0, result.totalPrecipitation);
            totalSnow[j] = result.totalSnow = max(lastResult.totalSnow, result.totalSnow);
            visibility[j] = static_cast<uint16_t>(result.visibility);
            windDirection[j] = static_cast<uint16_t>(result.WindDirection());
            windSpeed[j] = static_cast<uint16_t>(wxModel == WeatherModel::HRRR ? result.windSpeed : result.WindSpeed());

            lastResult = result;
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
        jLocation["wx"] = jWx;
        jLocation["isCity"] = locations[loc].isCity;

        #pragma omp critical
        jLocations[locations[loc].name] = jLocation;
    }
    root["locations"] = jLocations;
    root["forecastTimes"] = forecastTimes;

    cout << "Saving forecast.json..." << endl;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());    
    fstream forecastJson;
    forecastJson.open(fs::path(forecastDataOutputDir) / "forecast.json", fstream::out | fstream::trunc);
    writer->write(root, &forecastJson);
    forecastJson.close();

    delete[] allData;
    delete[] wxRegistries;
    cout << "Done!" << endl;
}
