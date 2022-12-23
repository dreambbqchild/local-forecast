#include "declarations.h"
#include <filesystem>
#include <sstream>
#include <curl/curl.h>

namespace fs = std::filesystem;
using namespace std;

GribDownloader::StaticConstructor GribDownloader::cons;

struct SaveData {
    time_t time;
    WeatherModel wxModel;
    uint16_t maxGribIndex, skipToGribNumber;
};

GribDownloader::StaticConstructor::StaticConstructor()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

time_t GetStartTimeForWeatherModelDownload(WeatherModel wxModel)
{
    auto now = time(nullptr);
    now -= secondsInHour;
    auto gmtm = gmtime(&now);
    auto fromLastDivBy6Hr = gmtm->tm_hour % 6;
    now -= secondsInHour * fromLastDivBy6Hr;
    if(wxModel == WeatherModel::HRRR && fromLastDivBy6Hr == 0 && gmtm->tm_min < 55)
        now -= secondsInHour * 6;
    else if(wxModel == WeatherModel::GFS && fromLastDivBy6Hr <= 4 && gmtm->tm_min < 55)
        now -= secondsInHour * 6;
    
    return now;
}

string GetFilePathTemplate(string outputDirectory, WeatherModel wxModel)
{
    switch(wxModel)
    {
        case WeatherModel::HRRR:
            return fs::path(outputDirectory) / "hrrr-%02d.grib2";
        case WeatherModel::GFS:
            return fs::path(outputDirectory) / "gfs-%03d.grib2";
    }
    return "";
}

string GetUrlForWeatherModel(WeatherModel wxModel, int32_t hour, int32_t forecastIndex, const char* timeStamp)
{
    stringstream urlStream;
    urlStream.precision(6);
    if(wxModel == WeatherModel::HRRR)
    {
        urlStream << "https://nomads.ncep.noaa.gov/cgi-bin/filter_hrrr_2d.pl?file=hrrr.t" <<
            setfill('0') << setw(2) << hour <<
            "z.wrfsfcf" <<
            setfill('0') << setw(2) << forecastIndex <<
            ".grib2&all_lev=on&all_var=on&subregion=&leftlon=" << fixed << leftlon << 
            "&rightlon="  << rightlon << 
            "&toplat=" << toplat <<
            "&bottomlat=" << bottomlat << 
            "&dir=%2Fhrrr." << timeStamp << "%2Fconus";
    }
    else if(wxModel == WeatherModel::GFS)
    {
        double gfsLeftlon = leftlon - 0.5,
               gfsToplat = toplat + 0.5,
               gfsRightlon = rightlon + 0.5,
               gfsBottomlat = bottomlat - 0.5;

        urlStream << "https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_0p25_1hr.pl?file=gfs.t" <<
            setfill('0') << setw(2) << hour << "z.pgrb2.0p25.f" <<
            setfill('0') << setw(3) << forecastIndex <<
            "&lev_10_m_above_ground=on&lev_20_m_above_ground=on&lev_2_m_above_ground=on&lev_entire_atmosphere=on&lev_entire_atmosphere_%5C%28considered_as_a_single_layer%5C%29=on&lev_mean_sea_level=on&lev_surface=on&lev_top_of_atmosphere=on&all_var=on&subregion=&leftlon=" << fixed << gfsLeftlon << 
            "&rightlon=" << gfsRightlon << 
            "&toplat=" << gfsToplat <<
            "&bottomlat=" << gfsBottomlat << 
            "&dir=%2Fgfs."<< timeStamp <<
            "%2F" << setfill('0') << setw(2) << hour << "%2Fatmos";
    }

    return urlStream.str();
}

void SaveDownloadInfo(string outputDirectory, WeatherModel wxModel, uint16_t maxGribIndex, uint16_t skipToGribNumber, time_t forecastStartTime)
{
    auto dlInfo = fs::path(outputDirectory) / "downloadInfo.bin";
    auto f = fopen(dlInfo.c_str(), "wb");
    if(!f)
    {
        cerr << "Unable to open " << dlInfo << endl;
        exit(1);
    }
    SaveData saveData = {forecastStartTime, wxModel, maxGribIndex, skipToGribNumber};
    fwrite(&saveData, sizeof(SaveData), 1, f);
    fclose(f);
}

GribDownloader::GribDownloader(string outputDirectory)
    : usingCachedMode(true)
{
    auto dlInfo = fs::path(outputDirectory) / "downloadInfo.bin";
    auto f = fopen(dlInfo.c_str(), "rb");
    if(!f)
    {
        cerr << "Could not open " << dlInfo << endl;
        exit(1);
    }

    SaveData saveData = {0};
    auto bytes = fread(&saveData, sizeof(SaveData), 1, f);
    if(bytes != 1)
        exit(1);
    fclose(f);
    
    forecastStartTime = saveData.time;
    maxGribIndex = saveData.maxGribIndex;
    skipToGribNumber = saveData.skipToGribNumber;
    filePathTemplate = ::GetFilePathTemplate(outputDirectory, saveData.wxModel);
    wxModel = saveData.wxModel;

    cout << "Loaded the " << (wxModel == WeatherModel::GFS ? "GFS" : "HRRR") << " from cache..." << endl;
}

GribDownloader::GribDownloader(string outputDirectory, WeatherModel wxModel, uint16_t maxGribIndex, uint16_t skipToGribNumber)
    :  maxGribIndex(maxGribIndex), skipToGribNumber(skipToGribNumber), usingCachedMode(false), wxModel(wxModel), forecastStartTime(GetStartTimeForWeatherModelDownload(wxModel)), outputDirectory(outputDirectory)
{
    filePathTemplate = ::GetFilePathTemplate(outputDirectory, wxModel);
}

void GribDownloader::Download()
{
    if(usingCachedMode)
        return;

    SaveDownloadInfo(outputDirectory, wxModel, maxGribIndex, skipToGribNumber, forecastStartTime);

    char timeStamp[9] = {0};
    tm forecastStart = *gmtime(&forecastStartTime);
    strftime(timeStamp, 9, "%Y%m%d", &forecastStart);

    cout << "Downloading the " << (wxModel == WeatherModel::GFS ? "GFS" : "HRRR") << " model with timestamp " << timeStamp << " at hour " << forecastStart.tm_hour << "..." << endl;

    #pragma omp parallel for
    for(uint32_t i = skipToGribNumber; i <= maxGribIndex; i++)
    {
        if(wxModel == WeatherModel::GFS && i > 120 && i % 3 != 0)
            continue;

        auto url = GetUrlForWeatherModel(wxModel, forecastStart.tm_hour, i, timeStamp);
        char outfilename[FILENAME_MAX] = {0};
        sprintf(outfilename, filePathTemplate.c_str(), i);
        auto curl = curl_easy_init();

        if (curl) {
            long httpCode = 0;
            auto fp = fopen(outfilename, "wb");
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            auto code = curl_easy_perform(curl);

            if(code != CURLE_OK)
            {
                cout << "Curl failed: " << code << endl;
                exit(1);
            }
            
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if(httpCode >= 300)
            {
                cout << "Http request failed: " << httpCode << endl;
                exit(1);
            }

            curl_easy_cleanup(curl);
            fclose(fp);
        }
    }

    cout << "Downloads complete!" << endl;
}