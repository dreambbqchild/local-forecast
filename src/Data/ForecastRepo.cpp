#include "DateTime.h"
#include "ForecastRepo.h"

#include <cassert>
#include <fstream>

using namespace std;
using namespace std::chrono;
namespace fs = std::filesystem;

typedef struct {
    char* key;
    Location location;
} LabeledLocation;

typedef struct {
    uint64_t now;
    uint64_t* forecastTimes;
    size_t forecastTimesLen;
    LabeledLocation* locations;
    size_t locationsLen;
    LabeledLunarPhase* phases;
    size_t phasesLen;
} RustForecast;

extern "C" {
    bool forecast_repo_load_forecast(const char* forecastKey, const char* path);
    bool forecast_repo_save_forecast(const char* forecastKey, const char* path);

    void forecast_repo_init_forecast(const char* forecastKey);
    void forecast_repo_add_forecast_start_time(const char* forecastKey, uint64_t forecastTime);
    void forecast_repo_add_lunar_phase(const char* forecastKey, int32_t day, LunarPhase phase);
    void forecast_repo_add_location(const char* forecastKey, const char* locationName, const Location* location);

    RustForecast* forecast_repo_get_forecast(const char* forecastKey);
    void forecast_repo_free_forecast(RustForecast* forecast);

    void forecast_repo_free(const char* forecastKey);
}

class LocationWrapper : public ILocation {
private:
    IForecast* forecast;
    const LabeledLocation* label;

    static const char* FindDayPart(string& shortDate)
    {
        auto ptr = shortDate.c_str();
        while(*ptr != ' ')
            ptr++;

        ptr++;

        if(*ptr == '0')
            ptr++;

        return ptr;
    }

public:
    LocationWrapper(IForecast* forecast, const LabeledLocation* label) : forecast(forecast), label(label){};

    const char* GetId() {
        return label->key;
    }

    const Coords& GetCoords() {
        return label->location.coords;
    }

    bool IsCity() {
        return label->location.isCity;
    }

    span<LabeledSun> GetSunriseSunsets()
    {
        span<LabeledSun> suns(label->location.suns, label->location.sunsLen);
        return suns;
    }

    const WxSingle& GetForecastAt(uint32_t forecastIndex)
    {
        static WxSingle empty = {};
        if(forecastIndex >= 0 && forecastIndex < label->location.wxLen)
            return label->location.wx[forecastIndex];

        return empty;
    }

    uint32_t GetForecastsFromNow(std::chrono::system_clock::time_point& now, uint32_t maxRows, function<void (std::chrono::system_clock::time_point&, const WxSingle* wx)> callback)
    {
        return forecast->GetForecastsFromNow(now, maxRows, [&](system_clock::time_point& forecastTime, uint32_t forecastIndex)
        {
            callback(forecastTime, &label->location.wx[forecastIndex]);
        });
    }

    void CollectSummaryData(SummaryData& summaryData, uint32_t maxRows)
    {
        string forecastDate;
        auto now = system_clock::from_time_t(forecast->GetNow());
        summaryData.locationName = label->key;

        const WxSingle* wxLast = nullptr;
        auto suns = GetSunriseSunsets();
        auto sunItr = suns.begin();

        GetForecastsFromNow(now, maxRows, [&](system_clock::time_point& forecastTime, const WxSingle* wx)
        {
            auto currentDate = GetShortDate(forecastTime);        
            if(currentDate != forecastDate)
            {
                forecastDate = currentDate;

                if(summaryData.sunrise < now || summaryData.sunset < now)
                {
                    auto day = atoi(FindDayPart(forecastDate));
                    while(sunItr->day != day)
                        sunItr++;

                    if(summaryData.sunrise < now)
                        summaryData.sunrise = system_clock::from_time_t(sunItr->sun.rise);
                    
                    if(summaryData.sunset < now)
                        summaryData.sunset = system_clock::from_time_t(sunItr->sun.set);
                }
            }

            int32_t temperature = wx->temperature;
            int32_t windSpd = wx->windSpeed;
            summaryData.high = max(summaryData.high, static_cast<int32_t>(wx->temperature));
            summaryData.low = min(summaryData.low, temperature);
            summaryData.wind = max(summaryData.wind, windSpd);
            
            if(wx->precipitationType == PrecipitationType::Snow)
                summaryData.snowTotal += CalcSnowPrecip(wx, wxLast);
            else if(wx->precipitationType == PrecipitationType::FreezingRain)
                summaryData.iceTotal += wx->newPrecipitation;
            else if(wx->precipitationType == PrecipitationType::Rain)
                summaryData.rainTotal += wx->newPrecipitation;

            wxLast = wx;
        });
    }
};

class Forecast : public IForecast {
private:
    RustForecast* forecast;

    static inline bool SortByLongitude(unique_ptr<ILocation>& l, unique_ptr<ILocation>& r)
    {
        return l->GetCoords().x - r->GetCoords().x < 0;
    }

public:
    Forecast(RustForecast* forecast) : forecast(forecast) {}

    void SetNow(uint64_t now) { forecast->now = now; }
    uint64_t GetNow() { return forecast->now; }

    uint64_t GetForecastTime(int32_t forecastIndex)
    {
        if(forecastIndex >= 0 && forecastIndex < forecast->forecastTimesLen)
            return forecast->forecastTimes[forecastIndex];
        
        return 0;
    }

    vector<unique_ptr<ILocation>> GetLocations(LocationMask mask)
    {
        int32_t index = 0;
        vector<unique_ptr<ILocation>> result;

        span<LabeledLocation> locations(forecast->locations, forecast->locationsLen);

        for(auto itr = locations.begin(); itr != locations.end(); itr++)
        {
            if(itr->location.isCity)
                if((mask & LocationMask::Cities) != LocationMask::Cities)
                    continue;

            if(!itr->location.isCity)
                if((mask & LocationMask::Homes) != LocationMask::Homes)
                    continue;

            auto index = itr - locations.begin();
            auto location = unique_ptr<ILocation>(new LocationWrapper(this, &forecast->locations[index]));
            result.push_back(std::move(location));
        }

        return result;
    }

    vector<unique_ptr<ILocation>> GetSortedPlaceLocations()
    {
        vector<unique_ptr<ILocation>> result = GetLocations(LocationMask::Homes);

        std::sort(result.begin(), result.end(), SortByLongitude);

        return result;
    }

    uint32_t GetForecastsFromNow(std::chrono::system_clock::time_point& now, uint32_t maxRows, function<void (std::chrono::system_clock::time_point&, uint32_t forecastIndex)> callback)
    {
        uint32_t rowsRendered = 0, 
                forecastIndex = 0;

        span<uint64_t> forecastTimes(forecast->forecastTimes, forecast->forecastTimesLen);

        auto start = forecastTimes.begin();
        auto end = forecastTimes.end();
        for(auto itr = start; itr != end && rowsRendered < maxRows; itr++, forecastIndex++)
        {
            auto forecastTime = system_clock::from_time_t(*itr);
            if(forecastTime + minutes(59) + seconds(59) < now)
                continue;

            callback(forecastTime, forecastIndex);

            rowsRendered++;
        }

        return rowsRendered;
    }

    LunarPhase GetLunarPhaseForDay(int32_t day)
    {
        span<LabeledLunarPhase> phases(forecast->phases, forecast->phasesLen);
        for(auto& phase : phases)
        {
            if(phase.day == day)
                return static_cast<LunarPhase>(phase.phase);
        }

        return LunarPhase::New;
    }

    virtual ~Forecast() {
        if(forecast) {
            forecast_repo_free_forecast(forecast);
            forecast = nullptr;
        }
    }
};

class ForecastRepo : public IForecastRepo {        
private:
    string forecastKey;

public:
    ForecastRepo(const string& forecastKey) : forecastKey(forecastKey) {}
    
    void Init()
    {
        forecast_repo_init_forecast(forecastKey.c_str());
    }

    void Load(fs::path forecastJsonPath)
    {
        forecast_repo_load_forecast(forecastKey.c_str(), forecastJsonPath.c_str());
    }

    void AddForecastStartTime(std::chrono::system_clock::time_point startTime)
    {
        forecast_repo_add_forecast_start_time(forecastKey.c_str(), system_clock::to_time_t(startTime));
    }

    void AddLunarPhase(int32_t currentDay, LunarPhase lunarPhase)
    {
        forecast_repo_add_lunar_phase(forecastKey.c_str(), currentDay, lunarPhase);
    }

    void AddLocation(const string& locationKey, const Location& location)
    {
        forecast_repo_add_location(forecastKey.c_str(), locationKey.c_str(), &location);
    }

    void Save(fs::path forecastJsonPath)
    {
        forecast_repo_save_forecast(forecastKey.c_str(), forecastJsonPath.c_str());
    }

    IForecast* GetForecast()
    {
        auto forecast = forecast_repo_get_forecast(forecastKey.c_str());
        if(forecast == nullptr)
            return nullptr;

        return new Forecast(forecast);
    }

    virtual ~ForecastRepo()
    {
        forecast_repo_free(forecastKey.c_str());
    }
};

IForecastRepo* InitForecastRepo(const string& forecastKey)
{
    auto repo = new ForecastRepo(forecastKey);
    repo->Init();
    return repo;
}

IForecastRepo* LoadForecastRepo(const std::string& forecastKey, std::filesystem::path forecastJsonPath)
{
    auto repo = new ForecastRepo(forecastKey);
    repo->Load(forecastJsonPath);
    return repo;
}