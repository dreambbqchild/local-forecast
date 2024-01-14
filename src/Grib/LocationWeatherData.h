#pragma once
#include "Geography/Geo.h"
#include "GribReader.h"
#include "Wx.h"

#include <unordered_map>
#include <vector>

class LocationWeatherData
{
private:
    std::vector<int32_t> validFieldDataIndexes;
    std::unordered_map<int32_t, int32_t> reverseFieldIndexLookup;
    std::vector<GeoCoord> geoCoords;
    int32_t pointsPerRow = 0, geoCoordsLength = 0;
    std::unique_ptr<std::vector<FieldData>[]> allData;
    std::unique_ptr<std::unordered_map<uint16_t, Wx>[]> wxRegistries;

public:
    LocationWeatherData(int32_t maxGribIndex)
    { 
        allData = std::unique_ptr<std::vector<FieldData>[]>(new std::vector<FieldData>[maxGribIndex + 1]);
        wxRegistries = std::unique_ptr<std::unordered_map<uint16_t, Wx>[]>(new std::unordered_map<uint16_t, Wx>[maxGribIndex + 1]);
    }

    inline std::vector<int32_t>& GetValidFieldDataIndexes(){ return validFieldDataIndexes; }
    inline int32_t GetIndexOfKnownValidFieldIndex(int32_t fieldDataIndex) { return reverseFieldIndexLookup[fieldDataIndex]; }
    inline std::vector<FieldData>& GetValidFieldDataAtKnownValidIndex(int32_t fieldDataIndex) { return allData[fieldDataIndex]; }
    inline GeoCoord& GetGeoCoordAtKnownValidIndexes(int32_t index) { return geoCoords[index]; }
    inline Wx& GetWxAtKnownValidIndexes(int32_t fieldDataIndex, int32_t coordIndex) { return wxRegistries[fieldDataIndex][coordIndex]; }
    inline std::vector<GeoCoord>& GetGeoCoords(){ return geoCoords; }

    inline int32_t GetPointsPerRow(){ return pointsPerRow; }
    inline int32_t GetGeoCoordsLength(){ return geoCoordsLength; }

    bool TryReadFieldData(GribReader& reader, int32_t index);

    inline bool HasDataAtIndex(int32_t i){ return allData == nullptr ? false : allData[i].size(); }
};