#include "Characters.h"
#include "HttpClient.h"
#include "Data/ForecastJsonExtension.h"
#include "DateTime.h"
#include "Drawing/ForecastImages/PersonalForecasts.h"
#include "Drawing/ForecastImages/RegionalForecast.h"
#include "Drawing/ForecastImages/WeatherMaps.h"
#include "Drawing/ImageCache.h"
#include "Grib/GribDownloader.h"
#include "Grib/GribProcessor.h"
#include "Text/SummaryForecast.h"
#include "Video/Encoder.h"
#include "Wx.h"

#include <glob.h>
#include <string.h>

#include <fstream>
#include <random>
#include <sstream>

extern "C" {
    #include "LocalForecastLib.h"
}

using namespace std;
using namespace chrono;
namespace fs = std::filesystem;

struct ForecastData {
    string gribFileTemplate;
    time_t forecastStart;
    WeatherModel weatherModel;
    uint16_t maxGribIndex, skipToGribNumber;
};

#define HasFlag(f, t) ((f & t) == f)

void RenderRegionalForecast()
{
    RegionalForecast regionalForecast;
    regionalForecast.Render("forecasts/openweather.json");
}

ForecastData ForecastDataFromDownloader(GribDownloader &downloader)
{
    return {
        downloader.GetFilePathTemplate(),
        downloader.GetForecastStartTime(),
        downloader.GetWeatherModel(),
        downloader.GetMaxGribIndex(),
        downloader.GetSkipToGribNumber()
    };
}

class LocalForecastRunner
{
private:
    Json::Value root;
    WeatherModel weatherModel; //Is set in ProcessGribData.
    fs::path gribFilePath;
    fs::path forecastFilePath;
    unique_ptr<LocationWeatherData> locationWeatherData;
    GeographicCalcs geoCalcs;

    void ProcessGribData(const ForecastData& data)
    {
        this->weatherModel = data.weatherModel;

        locationWeatherData = unique_ptr<LocationWeatherData>(new LocationWeatherData(data.maxGribIndex));
        GribProcessor processor(data.gribFileTemplate, data.weatherModel, system_clock::from_time_t(data.forecastStart), data.maxGribIndex, geoCalcs, data.skipToGribNumber);
        processor.Process(root, *locationWeatherData);

        cout << "Saving forecast.json..." << endl;
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());    

        fstream forecastJson;
        forecastJson.open(fs::path(forecastFilePath) / string("forecast.json"), fstream::out | fstream::trunc);
        writer->write(root, &forecastJson);
        forecastJson.close();
    }

    void RenderForecastAssets(RenderTargets renderTargets)
    {
        if(HasFlag(RegionalForecastRenderTarget, renderTargets))
            RenderRegionalForecast();

        if(HasFlag(WeatherMapsRenderTarget, renderTargets))
        {
            auto weatherMaps = unique_ptr<IWeatherMaps>(AllocWeatherMaps(root, *locationWeatherData, geoCalcs));
            weatherMaps->GenerateForecastMaps(forecastFilePath);
        }

        if(HasFlag(PersonalForecastsRenderTarget, renderTargets))
        {
            PersonalForecasts personalForecasts(root);
            personalForecasts.RenderAll(forecastFilePath, weatherModel == WeatherModel::GFS ? INT32_MAX : 24);
        }

        if(weatherModel == WeatherModel::GFS)
        {
            cout << "Text forecast not supported yet for GFS." << endl;
            return;
        }

        if(HasFlag(TextForecastRenderTarget, renderTargets))
        {
            auto textSummary = unique_ptr<ISummaryForecast>(AllocSummaryForecast());
            textSummary->Render(forecastFilePath, root);
        }
    }

    string SelectAudioFile(system_clock::time_point now)
    {
        auto localTm = ToLocalTm(now);
        auto rootPath = fs::path("media") / fs::path("music");
        stringstream holidayCheck;
        holidayCheck << std::setw(2) << std::setfill('0') << (localTm.tm_mon + 1) << "-" << localTm.tm_mday << ".m4a";
        auto holidayPath = rootPath / fs::path("holiday") / holidayCheck.str();
        if(fs::exists(holidayPath))
            return holidayPath;

        glob_t globResult = {0};
        if(glob((rootPath / fs::path("*.m4a")).c_str(), GLOB_TILDE, nullptr, &globResult) != 0)
        {
            globfree(&globResult);
            exit(1);
        }

        random_device rando;
        std::mt19937 gen(rando());
        uniform_int_distribution<int> dist(0, globResult.gl_pathc);
        auto index = dist(gen);
        string audioFile = globResult.gl_pathv[index];
        globfree(&globResult);
        return audioFile;
    }

    void EncodeVideo(RenderTargets renderTargets)
    {
        if(!HasFlag(VideoRenderTarget, renderTargets))
            return;

        if(weatherModel == WeatherModel::GFS)
        {
            cout << "Video not supported yet for GFS." << endl;
            return;
        }

        auto now = system_clock::from_time_t(root["now"].asInt64());
        fs::path outputFolder(forecastFilePath);
        auto encoder = unique_ptr<IEncoder>(AllocEncoder(forecastFilePath));    
        encoder->UseAudioFile(SelectAudioFile(now));
        encoder->EncodeImagesFittingPattern("forecasts/openweather.png", 10);
        encoder->EncodeImagesFittingPattern(outputFolder / string("personal-*"), 10);

        vector<string> maps;
        maps.push_back("temperature");
        maps.push_back("precip");

        for(auto& map : maps)
        {
            cout << "Rendering " << map <<" frames..." << endl;
            GetForecastsFromNow(root, now, INT32_MAX, [&](system_clock::time_point& forecastTime, int32_t forecastIndex)
            {
                encoder->EncodeImagesFittingPattern(outputFolder / (map + string("-") + root["forecastSuffixes"][forecastIndex].asString() + ".png"));
            });
        }

        encoder->Close();
    }

public:
    LocalForecastRunner(fs::path gribFilePath, fs::path forecastFilePath) :
        gribFilePath(gribFilePath), 
        forecastFilePath(forecastFilePath), 
        weatherModel(WeatherModel::HRRR), //Only to initalize. Is set in Process Grib data becasue it could come from the cache, or as a caller set parameter.
        geoCalcs(toplat, leftlon, bottomlat, rightlon) {}

    void ProcessCachedGribData() 
    {
        GribDownloader downloader(gribFilePath);
        ForecastData data = ForecastDataFromDownloader(downloader);

        ProcessGribData(data); 
    }

    void ProcessGribData(WeatherModel weatherModel, uint16_t skipToGribNumber, uint16_t maxGribIndex) 
    {
        GribDownloader downloader(gribFilePath, weatherModel, maxGribIndex, skipToGribNumber);
        downloader.Download();
        ForecastData data = ForecastDataFromDownloader(downloader);

        ProcessGribData(data); 
    }

    void Run(RenderTargets renderTargets)
    {
        RenderForecastAssets(renderTargets);
        EncodeVideo(renderTargets);
    }
};

static bool hasBeenInitalized = false;
static void InitInternal()
{
    if(hasBeenInitalized)
        return;

    tzset();
    HttpClient::Init();

    for(auto i = 0; i < Emoji::EmojiCount; i++)
        ImageCache::CacheImageAt(Emoji::Path(Emoji::All[i]));

    hasBeenInitalized = true;
}

extern "C"
{
    void LocalForecastLibInit() { InitInternal(); }
    void LocalForecastLibRenderRegionalForecast() { RenderRegionalForecast(); }
        
    void LocalForecastLibRenderLocalForecast(enum WxModel wxModel, const char* gribFilePath, const char* forecastFilePath, enum RenderTargets renderTargets, uint16_t skipToGribNumber, uint16_t maxGribIndex)
    {
        LocalForecastRunner runner(gribFilePath, forecastFilePath);
        runner.ProcessGribData((WeatherModel)wxModel, skipToGribNumber, maxGribIndex);
        runner.Run(renderTargets);
    }

    void LocalForecastLibRenderCahcedLocalForecast(const char* gribFilePath, const char* forecastFilePath, enum RenderTargets renderTargets)
    {
        LocalForecastRunner runner(gribFilePath, forecastFilePath);
        runner.ProcessCachedGribData();
        runner.Run(renderTargets);
    }
}