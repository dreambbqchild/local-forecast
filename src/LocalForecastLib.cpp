#include "Characters.h"
#include "HttpClient.h"
#include "DateTime.h"
#include "Data/ForecastRepo.h"
#include "Drawing/ForecastImages/PersonalForecasts.h"
#include "Drawing/ForecastImages/RegionalForecast.h"
#include "Drawing/ForecastImages/WeatherMaps.h"
#include "Drawing/ImageCache.h"
#include "Geography/Geo.h"
#include "Grib/GribDownloader.h"
#include "Grib/GribReader.h"
#include "NumberFormat.h"
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

class LocalForecastRunner
{
private:
    unique_ptr<IForecast> forecast;
    unique_ptr<GribData> gribData;
    
    WeatherModel weatherModel; //Is set in ProcessGribData.
    fs::path gribFilePath;
    fs::path forecastFilePath;
    fs::path videoFilePath;
    fs::path textFilePath;
    fs::path pathToRegionalForecastJson;
    fs::path pathToRegionalForecastPng;

    const SelectedRegion selectedRegion;
    GeographicCalcs geoCalcs;

    inline void ClearFlag(RenderTargets flag, RenderTargets& from) { from = (RenderTargets)(~flag & from);  }

    static ForecastData ForecastDataFromDownloader(GribDownloader &downloader)
    {
        return {
            downloader.GetFilePathTemplate(),
            downloader.GetForecastStartTime(),
            downloader.GetWeatherModel(),
            downloader.GetMaxGribIndex(),
            downloader.GetSkipToGribNumber()
        };
    }

    void ProcessGribData(const ForecastData& data, bool useCache)
    {
        unique_ptr<IForecastRepo> forecastRepo;

        this->weatherModel = data.weatherModel;
        auto forecastKey = this->weatherModel == WeatherModel::HRRR ? "hrrr" : "gfs";

        auto gribDataPath = forecastFilePath / string("gribdata.bin");
        auto forecastJsonPath = forecastFilePath / string("forecast.json");        

        if(!useCache)
        {
            if(!std::filesystem::exists(forecastFilePath))
                std::filesystem::create_directories(forecastFilePath);

            forecastRepo = unique_ptr<IForecastRepo>(InitForecastRepo(forecastKey));
            unique_ptr<IGribReader> gribReader(AllocGribReader(data.gribFileTemplate, selectedRegion, data.weatherModel, system_clock::from_time_t(data.forecastStart), data.skipToGribNumber, data.maxGribIndex, geoCalcs));
            gribReader->CollectData(forecastRepo, gribData);

            cout << "Saving " << forecastFilePath << "..." << endl;
            forecastRepo->Save(forecastJsonPath.c_str());

            cout << "Saving " << gribDataPath <<  "..." << endl;
            gribData->Save(gribDataPath);
        }
        else
        {
            cout << "Loading " << forecastJsonPath << "..." << endl;
            forecastRepo = unique_ptr<IForecastRepo>(LoadForecastRepo(forecastKey, forecastJsonPath.c_str()));

            cout << "Loading " << gribDataPath << "..." << endl;
            gribData = unique_ptr<GribData>(GribData::Load(gribDataPath));
        }

        forecast = unique_ptr<IForecast>(forecastRepo->GetForecast());
        forecast->SetNow(system_clock::to_time_t(system_clock::now()));
    }

    void RenderForecastAssets(RenderTargets renderTargets)
    {
        if(HasFlag(RegionalForecastRenderTarget, renderTargets))
            selectedRegion.RenderRegionalForecast(pathToRegionalForecastJson, pathToRegionalForecastPng);
        else
            cout << "Skipping regional forecast." << endl;

        if(HasFlag(WeatherMapsRenderTarget, renderTargets))
        {
            auto weatherMaps = unique_ptr<IWeatherMaps>(AllocWeatherMaps(forecast, gribData, geoCalcs, selectedRegion.GetMapBackgroundFileName()));
            weatherMaps->GenerateForecastMaps(forecastFilePath);
        }
        else
            cout << "Skipping Weather Maps." << endl;

        if(HasFlag(PersonalForecastsRenderTarget, renderTargets))
        {
            PersonalForecasts personalForecasts(forecast.get());
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
            textSummary->Render(textFilePath, forecast);
        }
        else
            cout << "Skipping text forecast." << endl;
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
        uniform_int_distribution<int> dist(0, globResult.gl_pathc - 1);
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

        auto now = system_clock::from_time_t(forecast->GetNow());
        auto encoder = unique_ptr<IEncoder>(AllocEncoder(videoFilePath));    
        encoder->UseAudioFile(SelectAudioFile(now));
        encoder->EncodeImagesFittingPattern(pathToRegionalForecastPng, 10);
        encoder->EncodeImagesFittingPattern(forecastFilePath / string("personal-*"), 10);

        vector<string> maps;
        maps.push_back("temperature");
        maps.push_back("precip");

        for(auto& map : maps)
        {
            cout << "Rendering " << map << " frames..." << endl;
            forecast->GetForecastsFromNow(now, UINT32_MAX, [&](system_clock::time_point& forecastTime, int32_t forecastIndex)
            {
                encoder->EncodeImagesFittingPattern(forecastFilePath / (map + "-" + ToStringWithPad(3, '0', forecastIndex) + ".png"), 2, forecastFilePath / "bg.png");
            });
        }

        encoder->Close();
    }

    static inline string WeatherModelToFilePath(WeatherModel model)
    {
        return model == WeatherModel::HRRR ? "hrrr" : "gfs";
    }

public:
    LocalForecastRunner(WeatherModel weatherModel, const SelectedRegion& selectedRegion) :
        forecast(nullptr),
        gribFilePath(fs::path("data") / WeatherModelToFilePath(weatherModel)),
        forecastFilePath(fs::path("forecasts") / selectedRegion.GetOutputFolder() / WeatherModelToFilePath(weatherModel)),
        weatherModel(weatherModel),
        selectedRegion(selectedRegion),
        geoCalcs(selectedRegion) 
        {
            videoFilePath = forecastFilePath / string("forecast.mp4");
            textFilePath = forecastFilePath / string("forecast.txt");

            selectedRegion.RenderRegionalForecastPaths(pathToRegionalForecastJson, pathToRegionalForecastPng);
        }

    inline const fs::path& GetVideoFilePath() { return videoFilePath; }

    inline const fs::path& GetTextFilePath() { return textFilePath; }

    void ProcessCachedGribData(RenderTargets& renderTargets)
    {
        GribDownloader downloader(selectedRegion, gribFilePath);
        ForecastData data = ForecastDataFromDownloader(downloader);

        ClearFlag(WeatherMapsRenderTarget, renderTargets);

        ProcessGribData(data, true); 
    }

    void ProcessGribData(RenderTargets& renderTargets, uint16_t skipToGribNumber, uint16_t maxGribIndex) 
    {
        GribDownloader downloader(selectedRegion, gribFilePath, weatherModel, maxGribIndex, skipToGribNumber);
        downloader.Download();
        ForecastData data = ForecastDataFromDownloader(downloader);
        ProcessGribData(data, false); 
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

static void HandleBuffer(const fs::path& path, char** buffer)
{
    if(buffer == nullptr)
        return;

    size_t pathLen = string(path).size();
    *buffer = (char*)calloc(pathLen + 1, sizeof(char));

    strncpy(*buffer, path.c_str(), pathLen);
}

extern "C"
{
    void LocalForecastLibInit() { InitInternal(); }

    void LocalForecastLibRenderRegionalForecast(const char* regionKey, char** pathToPngBuffer) 
    {
        fs::path pathToJson, pathToPng;
        SelectedRegion selectedRegion(WeatherModel::NoWeatherModel, regionKey);

        selectedRegion.RenderRegionalForecastPaths(pathToJson, pathToPng);

        HandleBuffer(pathToPng, pathToPngBuffer);
        
        selectedRegion.RenderRegionalForecast(pathToJson, pathToPng);
    }

    void LocalForecastLibRenderLocalForecast(const char* regionKey, enum WxModel wxModel, enum RenderTargets renderTargets, uint16_t skipToGribNumber, uint16_t maxGribIndex, char** pathToVideoBuffer, char** pathToTextBuffer)
    {
        SelectedRegion selectedRegion((WeatherModel)wxModel, regionKey);
        LocalForecastRunner runner((WeatherModel)wxModel, selectedRegion);

        HandleBuffer(runner.GetVideoFilePath(), pathToVideoBuffer);
        HandleBuffer(runner.GetTextFilePath(), pathToTextBuffer);

        runner.ProcessGribData(renderTargets, skipToGribNumber, maxGribIndex);
        runner.Run(renderTargets);

    }

    void LocalForecastLibRenderCahcedLocalForecast(const char* regionKey, enum WxModel wxModel, enum RenderTargets renderTargets, char** pathToVideoBuffer, char** pathToTextBuffer)
    {
        SelectedRegion selectedRegion((WeatherModel)wxModel, regionKey);
        LocalForecastRunner runner((WeatherModel)wxModel, selectedRegion);

        HandleBuffer(runner.GetVideoFilePath(), pathToVideoBuffer);
        HandleBuffer(runner.GetTextFilePath(), pathToTextBuffer);

        runner.ProcessCachedGribData(renderTargets);
        runner.Run(renderTargets);
    }
}