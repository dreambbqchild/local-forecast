extern "C"
{
    #include "LocalForecastLib.h"
}

#include <iostream>
#include <sstream>

using namespace std;

#define OptIs(s) !strcmp(argv[i], s)
#define NextI() NextICheck(i, argc)

struct Options {
    bool useCache;
    RenderTargets renderTargets;
    string locationKey;
    WxModel wxModel;
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

#define DisableRenderTarget(f) (RenderTargets)(~f & opts.renderTargets)
Options GetOptionsFromArgs(int argc, const char* argv[])
{
    Options opts = {
        .useCache = false, 
        .renderTargets = RenderTargets::AllRenderTargets,
        .wxModel = WxModel::HRRRWxModel, 
        .maxGribIndex = 48, 
        .skipToGribNumber = 1
    };

    for(auto i = 0; i < argc; i++)
    {
        if(OptIs("-useCache"))
            opts.useCache = true;
        else if(OptIs("-suppressWeatherMaps"))
            opts.renderTargets = DisableRenderTarget(WeatherMapsRenderTarget);
        else if(OptIs("-suppressRegionalForecast"))
            opts.renderTargets = DisableRenderTarget(RegionalForecastRenderTarget);
        else if(OptIs("-suppressPersonalForecasts"))
            opts.renderTargets = DisableRenderTarget(PersonalForecastsRenderTarget);
        else if(OptIs("-suppressText"))
            opts.renderTargets = DisableRenderTarget(TextForecastRenderTarget);
        else if(OptIs("-suppressVideo"))
            opts.renderTargets = DisableRenderTarget(VideoRenderTarget);
        else if(OptIs("-model") && NextI())
        {
            i++;
            if(OptIs("HRRR"))
                opts.wxModel = WxModel::HRRRWxModel;
            else if(OptIs("GFS"))
                opts.wxModel = WxModel::GFSWxModel;
            else   
            {
                cout << "Unknown Model." << endl;
                exit(1);
            }
        }
        else if(OptIs("-locationKey") && NextI())
            opts.locationKey = argv[++i];
        else if(OptIs("-maxGribIndex") && NextI())
            opts.maxGribIndex = static_cast<uint16_t>(atoi(argv[++i]));
        else if(OptIs("-skipToGribNumber") && NextI())
            opts.skipToGribNumber = static_cast<uint16_t>(atoi(argv[++i]));
    }

    return opts;
}
#undef DisableRenderTarget

int main(int argc, const char* argv[])
{   
    LocalForecastLibInit();    
    Options opts = GetOptionsFromArgs(argc, argv);
    char* outFile = nullptr;

    if(opts.useCache)
        LocalForecastLibRenderCahcedLocalForecast(opts.locationKey.c_str(), opts.wxModel, opts.renderTargets, &outFile);
    else
        LocalForecastLibRenderLocalForecast(opts.locationKey.c_str(), opts.wxModel, opts.renderTargets, opts.skipToGribNumber, opts.maxGribIndex, &outFile);

    cout << "Done! Rendered to " << outFile << endl;
    free(outFile);
    
    return 0;
}