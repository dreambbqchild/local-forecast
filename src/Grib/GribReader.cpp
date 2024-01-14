#include "Data/StringCache.h"
#include "GribReader.h"
#include <eccodes.h>
#include <iostream>

using namespace std;


#define GetString(fieldData, property) \
{ \
    char __localBuffer[128] = {0}; \
    size_t __length = 128; \
    CODES_CHECK(codes_get_string(h, #property, __localBuffer, &__length), "Unable to get " #property); \
    fieldData.property = GetCachedString(__localBuffer); \
}

#define GetInt(property, result)\
{ \
    long __value = 0; \
    codes_get_long(h, property, &__value); \
    result = static_cast<int32_t>(__value); \
}

GribReader::GribReader(string fileName, WeatherModel wxModel) 
    : fileName(fileName), wxModel(wxModel), rows(0), columns(0), numberOfValues(0)
{}

bool GribReader::GetFieldData(vector<FieldData>& result)
{
    codes_handle* h = nullptr;
    int err = 0, pointsPerRow = 0;
    auto file = fopen(fileName.c_str(), "rb");
    auto isFirst = true;

    if (!file)
        return false;

    GeoCoord* coordArray = nullptr;
    int32_t currentIndex = 0, currentRowIndex = 0, columnsSetInCurentRow = 0;
    while ((h = codes_handle_new_from_file(0, file, PRODUCT_GRIB, &err)) != NULL) 
    {
        FieldData fieldData = {0};

        GetString(fieldData, name)
        GetString(fieldData, level)
        GetString(fieldData, shortName)
        GetString(fieldData, stepRange)
        GetString(fieldData, typeOfLevel)
        GetInt("parameterNumber", fieldData.parameterNumber);

        if(isFirst)
        {
            GetInt("numberOfValues", numberOfValues);
            if(wxModel == WeatherModel::HRRR)
            {
                GetInt("Nx", columns);
                GetInt("Ny", rows);
            }
            else if(wxModel == WeatherModel::GFS)
            {
                GetInt("Ni", columns);
                GetInt("Nj", rows);
            }
            coordArray = new GeoCoord[numberOfValues];
        }

        currentRowIndex = rows - 1;
        currentIndex = currentRowIndex * columns;
        codes_iterator* iter = codes_grib_iterator_new(h, 0, &err);
        if (err != CODES_SUCCESS) 
            CODES_CHECK(err, 0);

        GeoCoord coord = {0};
        //Assuming default of WE:SN, Swapping to WE:NS
        while (codes_grib_iterator_next(iter, &coord.lat, &coord.lon, &fieldData.value))
        {
            fieldData.index = currentIndex;
            result.push_back(fieldData);

            if(isFirst)
                coordArray[currentIndex] = coord;

            if(++columnsSetInCurentRow == columns)
            {
                currentIndex = --currentRowIndex * columns;
                columnsSetInCurentRow = 0;
            }
            else
                currentIndex++;
        }

        codes_grib_iterator_delete(iter);

        codes_handle_delete(h);

        isFirst = false;
    }

    if(coordArray)
    {
        geoCoords = vector(coordArray, coordArray + numberOfValues);
        delete[] coordArray;
    }

    fclose(file);
    return true;
}