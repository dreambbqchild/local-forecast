#ifndef DECLARATIONS_H
#define DECLARATIONS_H

#include <json-c/json.h>
#include <sys/stat.h>

#define GRIB_FILES 49
#define NEARBY_SIZE 4

#define GetString(fieldData, property) \
length = sizeof(fieldData.property) / sizeof(char); \
CODES_CHECK(codes_get_string(h, #property, fieldData.property, &length), "Unable to get " #property);

#define AddJsonArray(i, addTo, shortName) \
locations[i].shortName = json_object_new_array(); \
json_object_object_add(addTo, #shortName, locations[i].shortName);

#define ShortNameIs(value) strcmp(fieldData.shortName, value) == 0 
#define TypeOfLevelIs(value) strcmp(fieldData.typeOfLevel, value) == 0 

typedef struct HomeData {
    double lats[NEARBY_SIZE], 
        lons[NEARBY_SIZE], 
        values[NEARBY_SIZE], 
        distances[NEARBY_SIZE];
    int indexes[NEARBY_SIZE];
} HomeData;

typedef struct FieldData {
    long step;
    char shortName[20];
    char name[128];
    char level[32];
    char typeOfLevel[256];
    char stepRange[8];
} FieldData;

typedef struct WindData {
    double u[NEARBY_SIZE], 
        v[NEARBY_SIZE], 
        gust[NEARBY_SIZE],
        speed[NEARBY_SIZE];
} WindData;

typedef struct PrecipData {
    int type[NEARBY_SIZE];
    double rate[NEARBY_SIZE];
    double cumulativeTotal[NEARBY_SIZE];
    double hourTotal[NEARBY_SIZE];
} PrecipData;

typedef struct Location {
    char name[16];
    double lat, lon;
    HomeData homeData;
    WindData windData;
    PrecipData precipData;
    json_object* root;
    json_object* cape;
    json_object* dewpoint;
    json_object* lightning;
    json_object* precip;
    json_object* pressure;
    json_object* totalCloudCover;
    json_object* temperature;
    json_object* vis;
    json_object* wind;
} Location;

typedef double (*conversionFn)(double);

#endif
