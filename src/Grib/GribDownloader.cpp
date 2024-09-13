#include "DateTime.h"
#include "Error.h"
#include "Geography/Geo.h"
#include "GribDownloader.h"
#include "HttpClient.h"
#include <filesystem>
#include <omp.h>
#include <sstream>

namespace fs = std::filesystem;
using namespace std;

struct SaveData {
    time_t time;
    WeatherModel weatherModel;
    uint16_t maxGribIndex, skipToGribNumber;
};

time_t GetStartTimeForWeatherModelDownload(WeatherModel weatherModel)
{   
    auto now = time(nullptr);
    auto sinceHourDivisibleBy6 = now % (secondsInHour * 6);
    
    now -= sinceHourDivisibleBy6;

    if(weatherModel == WeatherModel::HRRR && sinceHourDivisibleBy6 < (secondsInHour + (55 * secondsInMinute)))
        now -= secondsInHour * 6;
    else if(weatherModel == WeatherModel::GFS && sinceHourDivisibleBy6 < ((secondsInHour * 5) + (55 * secondsInMinute)))
        now -= secondsInHour * 6;

    return now;
}

string GetFilePathTemplate(string outputDirectory, WeatherModel weatherModel)
{
    switch(weatherModel)
    {
        case WeatherModel::HRRR:
            return fs::path(outputDirectory) / string("hrrr-%02d.grib2");
        case WeatherModel::GFS:
            return fs::path(outputDirectory) / string("gfs-%03d.grib2");
        default:
            ERR_OUT("Unsupported WeatherModel")
    }
}

string GetUrlForWeatherModel(const GeoBounds& geoBounds, WeatherModel weatherModel, int32_t hour, int32_t forecastIndex, const char* timeStamp)
{
    stringstream urlStream;
    urlStream.precision(6);
    if(weatherModel == WeatherModel::HRRR)
    {
        urlStream << "https://nomads.ncep.noaa.gov/cgi-bin/filter_hrrr_2d.pl?file=hrrr.t" <<
            setfill('0') << setw(2) << hour <<
            "z.wrfsfcf" <<
            setfill('0') << setw(2) << forecastIndex <<
            ".grib2&all_lev=on&all_var=on&subregion=&leftlon=" << fixed << geoBounds.leftLon << 
            "&rightlon="  << geoBounds.rightLon << 
            "&toplat=" << geoBounds.topLat <<
            "&bottomlat=" << geoBounds.bottomLat << 
            "&dir=%2Fhrrr." << timeStamp << "%2Fconus";
    }
    else if(weatherModel == WeatherModel::GFS)
    {
        urlStream << "https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_0p25_1hr.pl?file=gfs.t" <<
            setfill('0') << setw(2) << hour << "z.pgrb2.0p25.f" <<
            setfill('0') << setw(3) << forecastIndex <<
            "&lev_10_m_above_ground=on&lev_20_m_above_ground=on&lev_2_m_above_ground=on&lev_entire_atmosphere=on&lev_entire_atmosphere_%5C%28considered_as_a_single_layer%5C%29=on&lev_mean_sea_level=on&lev_surface=on&lev_top_of_atmosphere=on&all_var=on&subregion=&leftlon=" << fixed << geoBounds.leftLon << 
            "&rightlon=" << geoBounds.rightLon << 
            "&toplat=" << geoBounds.topLat <<
            "&bottomlat=" << geoBounds.bottomLat << 
            "&dir=%2Fgfs."<< timeStamp <<
            "%2F" << setfill('0') << setw(2) << hour << "%2Fatmos";
    }

    return urlStream.str();
}

void SaveDownloadInfo(string outputDirectory, WeatherModel weatherModel, uint16_t maxGribIndex, uint16_t skipToGribNumber, time_t forecastStartTime)
{
    auto dlInfo = fs::path(outputDirectory) / string("downloadInfo.bin");
    auto f = fopen(dlInfo.c_str(), "wb");
    if(!f)
        ERR_OUT("Unable to open " << dlInfo)

    SaveData saveData = {forecastStartTime, weatherModel, maxGribIndex, skipToGribNumber};
    fwrite(&saveData, sizeof(SaveData), 1, f);
    fclose(f);
}

bool LoadDownloadInfo(fs::path outputDirectory, SaveData& saveData, bool exitOnNotFound)
{
    auto dlInfo = outputDirectory / string("downloadInfo.bin");
    auto f = fopen(dlInfo.c_str(), "rb");
    if(!f)
    {
        if(exitOnNotFound)
            ERR_OUT("Could not open " << dlInfo)
        
        return false;
    }

    auto bytes = fread(&saveData, sizeof(SaveData), 1, f);
    if(bytes != 1)
    {
        if(exitOnNotFound)
            exit(1);

        return false;
    }
    fclose(f);
    return true;
}

GribDownloader::GribDownloader(const SelectedRegion& selectedRegion, string outputDirectory)
    : selectedRegion(selectedRegion), usingCachedMode(true)
{
    SaveData saveData = {0};
    LoadDownloadInfo(outputDirectory, saveData, true);
    Init(static_cast<void*>(&saveData));

    filePathTemplate = ::GetFilePathTemplate(outputDirectory, saveData.weatherModel);
}

GribDownloader::GribDownloader(const SelectedRegion& selectedRegion, string outputDirectory, WeatherModel weatherModel, uint16_t maxGribIndex, uint16_t skipToGribNumber)
    :  selectedRegion(selectedRegion), maxGribIndex(maxGribIndex), skipToGribNumber(skipToGribNumber), usingCachedMode(false), weatherModel(weatherModel), forecastStartTime(GetStartTimeForWeatherModelDownload(weatherModel)), outputDirectory(outputDirectory)
{
    filePathTemplate = ::GetFilePathTemplate(outputDirectory, weatherModel);
    
    if(!std::filesystem::exists(outputDirectory))
        std::filesystem::create_directories(outputDirectory);
}

void GribDownloader::Init(void* vSaveData)
{
    auto saveData = static_cast<SaveData*>(vSaveData);
    forecastStartTime = saveData->time;
    maxGribIndex = saveData->maxGribIndex;
    skipToGribNumber = saveData->skipToGribNumber;
    weatherModel = saveData->weatherModel;

    cout << "Loaded the " << (weatherModel == WeatherModel::GFS ? "GFS" : "HRRR") << " from cache..." << endl;
}

void GribDownloader::Download()
{
    if(usingCachedMode)
        return;

    SaveData saveData = {0};
    if(LoadDownloadInfo(outputDirectory, saveData, false) && saveData.time == forecastStartTime)
    {
        cout << "Would download the same data already cached. Loading that, and continuing..." << endl;
        Init(static_cast<void*>(&saveData));
        usingCachedMode = true;
        return;
    }

    char timeStamp[9] = {0};
    tm forecastStart = {0};
    gmtime_r(&forecastStartTime, &forecastStart);
    strftime(timeStamp, 9, "%Y%m%d", &forecastStart);

    cout << "Downloading the " << (weatherModel == WeatherModel::GFS ? "GFS" : "HRRR") << " model with timestamp " << timeStamp << " at hour " << forecastStart.tm_hour << "..." << endl;

    auto geoBounds = selectedRegion.GetForecastAreaBounds();
    #pragma omp parallel for num_threads(3)
    for(uint32_t i = skipToGribNumber; i <= maxGribIndex; i++)
    {
        if(weatherModel == WeatherModel::GFS && i > 120 && i % 3 != 0)
            continue;

        auto url = GetUrlForWeatherModel(geoBounds, weatherModel, forecastStart.tm_hour, i, timeStamp);
        cout << "Thread " << omp_get_thread_num() << ": Downloading " << url << endl;
        char outfilename[FILENAME_MAX] = {0};
        snprintf(outfilename, FILENAME_MAX, filePathTemplate.c_str(), i);
        auto fp = fopen(outfilename, "wb");
        if(!fp)
            ERR_OUT("Unable to to open " << outfilename);

        HttpClient::Get(url.c_str(), fwrite, fp);

        fclose(fp);
    }
    
    SaveDownloadInfo(outputDirectory, weatherModel, maxGribIndex, skipToGribNumber, forecastStartTime);

    cout << "Downloads complete!" << endl;
}