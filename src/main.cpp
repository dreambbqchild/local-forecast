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

using namespace std;
using namespace chrono;
namespace fs = std::filesystem;

#define OptIs(s) !strcmp(argv[i], s)
#define NextI() NextICheck(i, argc)

static void usage(const char* prog)
{
    printf("Usage: %s grib_file\n", prog);
    exit(1);
}

struct ForecastData {
    string gribFileTemplate;
    time_t forecastStart;
    WeatherModel wxModel;
    uint16_t maxGribIndex, skipToGribNumber;
};

struct Options {
    bool useCache, renderWeatherMaps, renderRegionalForecast, renderPersonalForecasts, renderText, renderVideo;
    WeatherModel wxModel;
    string gribFilePath, forecastPath;
    uint16_t maxGribIndex, skipToGribNumber;
};

inline bool NextICheck(int& i, int& argc)
{
    if(i + 1 < argc)
        return true;

    cerr << "Option " << i << " Out of bounds" << endl;
    exit(1);
    return false;
}

inline ForecastData ForecastDataFromDownloader(GribDownloader &downloader)
{
    return {
        downloader.GetFilePathTemplate(),
        downloader.GetForecastStartTime(),
        downloader.GetWeatherModel(),
        downloader.GetMaxGribIndex(),
        downloader.GetSkipToGribNumber()
    };
}

Options GetOptionsFromArgs(int argc, const char* argv[])
{
    Options opts = {false, true, true, true, true, true, WeatherModel::HRRR, "./data/hrrr", "./forecasts/hrrr", 48, 1};
    for(auto i = 0; i < argc; i++)
    {
        if(OptIs("-useCache"))
            opts.useCache = true;
        else if(OptIs("-suppressWeatherMaps"))
            opts.renderWeatherMaps = false;
        else if(OptIs("-suppressRegionalForecast"))
            opts.renderRegionalForecast = false;
        else if(OptIs("-suppressPersonalForecasts"))
            opts.renderPersonalForecasts = false;
        else if(OptIs("-suppressText"))
            opts.renderText = false;
        else if(OptIs("-suppressVideo"))
            opts.renderVideo = false;
        else if(OptIs("-model") && NextI())
        {
            i++;
            if(OptIs("HRRR"))
                opts.wxModel = WeatherModel::HRRR;
            else if(OptIs("GFS"))
                opts.wxModel = WeatherModel::GFS;
            else   
            {
                cout << "Unknown Model." << endl;
                exit(1);
            }
        }
        else if(OptIs("-gribFilePath") && NextI())
            opts.gribFilePath = argv[++i];
        else if(OptIs("-forecastPath") && NextI())
            opts.forecastPath = argv[++i];
        else if(OptIs("-maxGribIndex") && NextI())
            opts.maxGribIndex = static_cast<uint16_t>(atoi(argv[++i]));
        else if(OptIs("-skipToGribNumber") && NextI())
            opts.skipToGribNumber = static_cast<uint16_t>(atoi(argv[++i]));
    }

    return opts;
}

void ProcessGribData(const Options& opts, Json::Value& root, LocationWeatherData& locationWeatherData, GeographicCalcs& geoCalcs)
{
    if(!opts.renderPersonalForecasts && !opts.renderWeatherMaps && !opts.renderText && !opts.renderVideo)
        return;

    ForecastData data;

    if(opts.useCache)
    {
        GribDownloader downloader(opts.gribFilePath);
        data = ForecastDataFromDownloader(downloader);    
    }
    else
    {
        GribDownloader downloader(opts.gribFilePath, opts.wxModel, opts.maxGribIndex, opts.skipToGribNumber);
        downloader.Download();
        data = ForecastDataFromDownloader(downloader);
    }

    GribProcessor processor(data.gribFileTemplate, data.wxModel, system_clock::from_time_t(data.forecastStart), opts.maxGribIndex, geoCalcs, opts.skipToGribNumber);
    processor.Process(root, locationWeatherData);

    cout << "Saving forecast.json..." << endl;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());    

    fstream forecastJson;
    forecastJson.open(fs::path(opts.forecastPath) / string("forecast.json"), fstream::out | fstream::trunc);
    writer->write(root, &forecastJson);
    forecastJson.close();
}

void RenderForecastAssets(const Options& opts, Json::Value& root, LocationWeatherData& locationWeatherData, GeographicCalcs& geoCalcs)
{
    for(auto i = 0; i < Emoji::EmojiCount; i++)
        ImageCache::CacheImageAt(Emoji::Path(Emoji::All[i]));

    if(opts.renderWeatherMaps)
    {
        auto weatherMaps = unique_ptr<IWeatherMaps>(AllocWeatherMaps(root, locationWeatherData, geoCalcs));
        weatherMaps->GenerateForecastMaps(opts.forecastPath);
    }

    if(opts.renderRegionalForecast)
    {
        RegionalForecast regionalForecast;
        regionalForecast.Render("forecasts/openweather.json", opts.useCache);
    }

    if(opts.renderPersonalForecasts)
    {
        PersonalForecasts personalForecasts(root);
        personalForecasts.RenderAll(opts.forecastPath, opts.wxModel == WeatherModel::GFS ? INT32_MAX : 24);
    }

    if(opts.wxModel == WeatherModel::GFS)
    {
        cout << "Text forecast not supported yet for GFS." << endl;
        return;
    }

    if(opts.renderText)
    {
        auto textSummary = unique_ptr<ISummaryForecast>(AllocSummaryForecast());
        textSummary->Render(opts.forecastPath, root);
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

void EncodeVideo(const Options& opts, Json::Value& root)
{
    if(!opts.renderVideo)
        return;

    if(opts.wxModel == WeatherModel::GFS)
    {
        cout << "Video not supported yet for GFS." << endl;
        return;
    }

    auto now = system_clock::from_time_t(root["now"].asInt64());
    fs::path outputFolder(opts.forecastPath);
    auto encoder = unique_ptr<IEncoder>(AllocEncoder(opts.forecastPath));    
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

int main(int argc, const char* argv[])
{   
    tzset();
    HttpClient::Init();
    
    Options opts = GetOptionsFromArgs(argc, argv);    

    Json::Value root;
    LocationWeatherData locationWeatherData(opts.maxGribIndex);
    GeographicCalcs geoCalcs(toplat, leftlon, bottomlat, rightlon);
    ProcessGribData(opts, root, locationWeatherData, geoCalcs);
    RenderForecastAssets(opts, root, locationWeatherData, geoCalcs);

    EncodeVideo(opts, root);

    cout << "Done!" << endl;
    return 0;
}