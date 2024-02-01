#include "GribData.h"
#include "Error.h"

using namespace std;

const WxAtGeoCoord GribData::emptyWx = {};

vector<GeoCoord> GribData::GetGeoCoords()
{
    vector<GeoCoord> result;
    result.reserve(geoCoordLookup.size());
    
    for(auto& pair : geoCoordLookup)
        result.push_back(pair.first);

    return result;
}

void GribData::Save(std::filesystem::path path)
{
    auto f = fopen(path.c_str(), "wb");
    if(!f)
        ERR_OUT("Unable to open " << path);

    fwrite(&numberOfFiles, sizeof(size_t), 1, f);
    auto len = validIndexes.size();
    fwrite(&len, sizeof(size_t), 1, f);
    for(auto& index : validIndexes)
        fwrite(&index, sizeof(int32_t), 1, f);

    len = quadIndexes.size();
    fwrite(&len, sizeof(size_t), 1, f);
    for(auto& quad : quadIndexes)
        fwrite(&quad, sizeof(QuadIndexes), 1, f);

    len = geoCoordLookup.size();
    fwrite(&len, sizeof(size_t), 1, f);
    for(auto& pair : geoCoordLookup)
    {
        fwrite(&pair.first, sizeof(GeoCoord), 1, f);
        fwrite(&pair.second, sizeof(int32_t), 1, f);
    }

    for(auto& map : wxResults)
    {
        len = map.size();
        fwrite(&len, sizeof(size_t), 1, f);
        for(auto& pair : map)
        {
            fwrite(&pair.first, sizeof(int32_t), 1, f);
            fwrite(&pair.second, sizeof(WxAtGeoCoord), 1, f);
        }
    }

    fclose(f);
}

GribData* GribData::Load(std::filesystem::path path)
{
    auto f = fopen(path.c_str(), "rb");
    if(!f)
        ERR_OUT("Unable to open " << path);

    size_t len = 0;
    int32_t int32Value = 0;
    QuadIndexes quadIndexValue = {};
    GeoCoord geoCoordValue = {};
    WxAtGeoCoord wxAtGeoCoordValue = {};

    int32_t numberOfFiles;
    std::vector<int32_t> validIndexes;
    std::vector<QuadIndexes> quadIndexes;
    std::unordered_map<GeoCoord, int32_t> geoCoordLookup;
    std::vector<std::unordered_map<int32_t, WxAtGeoCoord>> wxResults;
    
    fread(&numberOfFiles, sizeof(size_t), 1, f);
    fread(&len, sizeof(size_t), 1, f);
    for(auto i = 0; i < len; i++)
    {
        fread(&int32Value, sizeof(int32_t), 1, f);
        validIndexes.push_back(int32Value);
    }

    fread(&len, sizeof(size_t), 1, f);
    for(auto i = 0; i < len; i++)
    {
        fread(&quadIndexValue, sizeof(QuadIndexes), 1, f);
        quadIndexes.push_back(quadIndexValue);
    }

    fread(&len, sizeof(size_t), 1, f);
    for(auto i = 0; i < len; i++)
    {
        fread(&geoCoordValue, sizeof(GeoCoord), 1, f);
        fread(&int32Value, sizeof(int32_t), 1, f);
        geoCoordLookup[geoCoordValue] = int32Value;
    }

    wxResults.resize(numberOfFiles);
    for(auto i = 0; i < numberOfFiles; i++)
    {
        auto& wxResult = wxResults[i];
        fread(&len, sizeof(size_t), 1, f);
        for(auto k = 0; k < len; k++)
        {
            fread(&int32Value, sizeof(int32_t), 1, f);
            fread(&wxAtGeoCoordValue, sizeof(WxAtGeoCoord), 1, f);        
            wxResult[int32Value] = wxAtGeoCoordValue;
        }
    }

    fclose(f);

    return new GribData(validIndexes, quadIndexes, geoCoordLookup, wxResults);
}