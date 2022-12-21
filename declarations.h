#ifndef DECLARATIONS_H
#define DECLARATIONS_H
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include "calcs.h"

#define ERR_OUT(streamCommands) {\
    std::cerr << streamCommands << std::endl; \
    exit(1);\
}

//const double leftlon = -93.550505, rightlon = -92.852469, toplat = 45.184381, bottomlat = 44.468505;
const double leftlon = -93.63166870117188, rightlon = -92.85987915039063, toplat = 45.183375349113845, bottomlat = 44.61724084030939;
const uint16_t secondsInHour = 60 * 60;

struct GeoCoord 
{
    double lat, lon;
};

struct FieldData 
{
    uint16_t index;
    char name[128], level[32], shortName[32], stepRange[16], typeOfLevel[128];
    double value;
};

struct Location {
    char name[32];
    double lat, lon;
    bool isCity;
};

enum WeatherModel {HRRR, GFS};

//---
class ForecastArea
{
private:
    WeatherModel wxModel;
    uint16_t maxGribIndex, skipToGribNumber;
    uint32_t imageWidth, imageHeight;
    double metersWidth, metersHeight;
    time_t forecastStartTime;
    std::string gribPathTemplate, forecastDataOutputDir;

    Point FindXY(double lat, double lon);

public:
    ForecastArea(std::string gribPathTemplate, WeatherModel wxModel, time_t forecastStartTime, std::string forecastDataOutputDir, uint16_t maxGribIndex, uint16_t skipToGribNumber = 0);
    void Process();
};

//---
union Pixel;
struct ImageData;
struct SetPixelData
{
    Point pt;
    Pixel* px;
};

class ForecastImage 
{
private:
    std::vector<SetPixelData> setPixels;
    std::unordered_set<uint32_t> protectedPixels;
    ImageData* imageData;
    
public:
    ForecastImage(std::string fileName, uint32_t width, uint32_t height);
    void SetPixel(double dx, double dy, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void InterpolateFill(int32_t pointsPerRow);
    void Save();
    virtual ~ForecastImage();
};

//---
class GribReader 
{
private:
    WeatherModel wxModel;
    int32_t rows, columns, numberOfValues;
    std::string fileName;
    std::vector<GeoCoord> geoCoords;

public:
    GribReader(std::string fileName, WeatherModel wxModel);
    std::vector<FieldData> GetFieldData();
    std::vector<GeoCoord> GetGeoCoords(){ return geoCoords; }
    int32_t GetRows(){return rows;}
    int32_t GetColumns(){return columns;}
    int32_t GetNumberOfValues(){return numberOfValues;}
};


//---
class GribDownloader {
private:
    struct StaticConstructor{
        StaticConstructor();
    };
    static StaticConstructor cons;

    uint16_t maxGribIndex, skipToGribNumber;
    bool usingCachedMode;
    WeatherModel wxModel;
    time_t forecastStartTime;
    std::string filePathTemplate, outputDirectory;

public:
    GribDownloader(std::string outputDirectory);
    GribDownloader(std::string outputDirectory, WeatherModel wxmodel, uint16_t maxGribIndex, uint16_t skipToGribNumber = 0);

    std::string GetFilePathTemplate() {return filePathTemplate;}
    uint16_t GetMaxGribIndex() {return maxGribIndex;}
    uint16_t GetSkipToGribNumber() {return skipToGribNumber;}
    WeatherModel GetWeatherModel() {return wxModel;}
    time_t GetForecastStartTime() {return forecastStartTime;}

    void Download();
};


#endif