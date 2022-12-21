#include "declarations.h"
#include <string.h>

using namespace std;
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
    bool useCache;
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
    Options opts = {false, WeatherModel::HRRR, "./data/hrrr", "./forecasts/hrrr", 48, 1};
    for(auto i = 0; i < argc; i++)
    {
        if(OptIs("-useCache"))
            opts.useCache = true;
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

int main(int argc, const char* argv[])
{
    Options opts = GetOptionsFromArgs(argc, argv);

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

    ForecastArea area(data.gribFileTemplate, data.wxModel, data.forecastStart, opts.forecastPath, opts.maxGribIndex, opts.skipToGribNumber);
    area.Process();
    return 0;
}