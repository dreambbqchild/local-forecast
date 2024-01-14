#include "LocationWeatherData.h"

bool LocationWeatherData::TryReadFieldData(GribReader& reader, int32_t index)
{
    if(!reader.GetFieldData(allData[index]))
        return false;

    if(!validFieldDataIndexes.size())
    {
        geoCoords = reader.GetGeoCoords();
        pointsPerRow = reader.GetColumns();
        geoCoordsLength = reader.GetNumberOfValues();
    }

    reverseFieldIndexLookup[index] = reverseFieldIndexLookup.size();
    validFieldDataIndexes.push_back(index);
    return true;
}