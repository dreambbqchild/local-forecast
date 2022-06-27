#include "declarations.h"
#include "eccodes.h"

extern Location locations[];
extern const int locationsLength;

const int PTYPE_RAIN = 1 << 0,
    PTYPE_FREEZING_RAIN = 1 << 1,
    PTYPE_SNOW = 1 << 2;

const unsigned long flags = CODES_NEAREST_SAME_GRID | CODES_NEAREST_SAME_POINT;
const char* orderBy="step,param";

double ToFarenheight(double k) {
    return (k - 273.15) * 9/5.0 + 32.0;
}

double ToInPerHour(double mmPerSec) {
    return mmPerSec * 141.7;
}

double ToMiles(double m) {
    return m * 0.000621371;
}

double ToMPH(double mPerS) {
    return mPerS * 2.237;
}

double ToInHg(double pa) {
    return pa / 3386.0;
}

double WindDirection(double u, double v) {
    return (180 / 3.14159265358979323846) * atan2(u, v) + 180;
}

codes_fieldset* GetFieldSet() {
    int err = 0, gribFileCount = 0;
    char** gribFiles = (char**)malloc(sizeof(char*) * GRIB_FILES );

    for(gribFileCount = 0; gribFileCount < GRIB_FILES; gribFileCount++)
    {
        struct stat statBuffer;
        gribFiles[gribFileCount] = malloc(sizeof(char) * 21);
        sprintf(gribFiles[gribFileCount], "./data/hrrr-%02d.grib2", gribFileCount);
        if(stat(gribFiles[gribFileCount], &statBuffer))
        {
            free(gribFiles[gribFileCount]);
            break;
        }
    }

    codes_fieldset* set = codes_fieldset_new_from_files(0, gribFiles, gribFileCount, 0, 0, 0, orderBy, &err);
    CODES_CHECK(err, "unable to get fieldset");

    for(int i = 0; i < gribFileCount; i++)
        free(gribFiles[i]);
    free(gribFiles);

    CODES_CHECK(err, "unable to get fieldset");

    return set;
}

int AddDate(codes_handle* h, json_object* root) {
    char isoDate[25] = {0};
    long dataDate = 0, dataTime = 0;
    CODES_CHECK(codes_get_long(h, "dataDate", &dataDate), "Unable to get dataDate");
    CODES_CHECK(codes_get_long(h, "dataTime", &dataTime), "Unable to get dataTime");

    int year = dataDate / 10000;
    int month = (dataDate % 10000) / 100;
    int day = (dataDate % 100);
    int hour = dataTime / 100;

    sprintf(isoDate, "%04d-%02d-%02dT%02d:00:00.000Z", year, month, day, hour);

    json_object_object_add(root, "date", json_object_new_string(isoDate));
    return hour;
}

void PrepareHomeData(json_object* root, codes_handle* h, double lat, double lon, HomeData* homeData) {
    int err = 0;
    size_t nearbySize = NEARBY_SIZE;
    codes_nearest* nearest = codes_grib_nearest_new(h, &err);
    CODES_CHECK(codes_grib_nearest_find(nearest, h, lat, lon, flags, homeData->lats, homeData->lons, homeData->values, homeData->distances, homeData->indexes, &nearbySize), "Unable to find nearest");

    json_object* latLongs = json_object_new_array();
    json_object_object_add(root, "grid", latLongs);
    for(int i = 0; i < NEARBY_SIZE; i++) {
        json_object* latLong = json_object_new_object();
        json_object_object_add(latLong, "lat", json_object_new_double(homeData->lats[i]));
        json_object_object_add(latLong, "lon", json_object_new_double(homeData->lons[i]));
        json_object_object_add(latLong, "distance", json_object_new_double(homeData->distances[i]));
        json_object_array_add(latLongs, latLong);
    }
    codes_grib_nearest_delete(nearest);
}

void FinishStep(long step, WindData* windData, PrecipData* precipData, json_object* wind, json_object* precip) {
    json_object* windArr = json_object_new_array();
    json_object* precipArr = json_object_new_array();

    json_object_array_put_idx(wind, step, windArr);
    json_object_array_put_idx(precip, step, precipArr);

    for(int i = 0; i < NEARBY_SIZE; i++) {
        json_object* windJson = json_object_new_object();
        json_object* precipJson = json_object_new_object();
        json_object* precipTypes = json_object_new_array();

        json_object_object_add(windJson, "dir", json_object_new_double(WindDirection(windData->u[i], windData->v[i])));
        json_object_object_add(windJson, "speed", json_object_new_double(windData->speed[i]));
        json_object_object_add(windJson, "gust", json_object_new_double(windData->gust[i]));

        json_object_object_add(precipJson, "rate", json_object_new_double(precipData->rate[i]));
        json_object_object_add(precipJson, "types", precipTypes);
        if((precipData->type[i] & PTYPE_RAIN) == PTYPE_RAIN)
            json_object_array_add(precipTypes, json_object_new_string("rain"));
        if((precipData->type[i] & PTYPE_FREEZING_RAIN) == PTYPE_FREEZING_RAIN)
            json_object_array_add(precipTypes, json_object_new_string("freezing rain"));
        if((precipData->type[i] & PTYPE_SNOW) == PTYPE_SNOW)
            json_object_array_add(precipTypes, json_object_new_string("snow"));

        json_object_array_add(windArr, windJson);
        json_object_array_add(precipArr, precipJson);
    }

    memset(windData, 0x0, sizeof(WindData));
    memset(precipData, 0x0, sizeof(PrecipData));
}

int main(int argc, char* argv[])
{
    char isFirst = 1;
    int err = 0, hour = 0;
    long lastStep = 0;
    codes_handle* h = NULL;

    FieldData fieldData = {0};

    size_t length = 0;

    codes_fieldset* set = GetFieldSet();

    json_object* root = json_object_new_object();
    json_object* objLocations = json_object_new_object();
    json_object_object_add(root, "locations", objLocations);

    for(int i = 0; i < locationsLength; i++)
    {
        locations[i].root = json_object_new_object();
        json_object_object_add(objLocations, locations[i].name, locations[i].root);
        AddJsonArray(i, locations[i].root, cape)
        AddJsonArray(i, locations[i].root, dewpoint)
        AddJsonArray(i, locations[i].root, lightning)
        AddJsonArray(i, locations[i].root, precip)
        AddJsonArray(i, locations[i].root, pressure)
        AddJsonArray(i, locations[i].root, totalCloudCover)
        AddJsonArray(i, locations[i].root, temperature)
        AddJsonArray(i, locations[i].root, vis)
        AddJsonArray(i, locations[i].root, wind)
    }

    while (( h = codes_fieldset_next_handle(set, &err)) != NULL)
    {
        CODES_CHECK(codes_get_long(h, "step", &fieldData.step), "Unable to get step");
        if(fieldData.step != lastStep){
            for(int i = 0; i < locationsLength; i++)
                FinishStep(lastStep, &locations[i].windData, &locations[i].precipData, locations[i].wind, locations[i].precip);
        }
        lastStep = fieldData.step;

        if(isFirst)
            hour = AddDate(h, root);

        GetString(fieldData, shortName)
        GetString(fieldData, level)
        GetString(fieldData, typeOfLevel)

        if (isFirst) {
            for(int i = 0; i < locationsLength; i++)
                PrepareHomeData(locations[i].root, h, locations[i].lat, locations[i].lon, &locations[i].homeData);
        }

        size_t valuesLen;
        CODES_CHECK(codes_get_size(h, "values", &valuesLen),0);
        double* values = (double*)malloc(valuesLen * sizeof(double));
        CODES_CHECK(codes_get_double_array(h, "values", values, &valuesLen), "unable to get values");
        printf("%ld %s %s %s %g\n", fieldData.step, fieldData.shortName, fieldData.typeOfLevel, fieldData.level, values[locations[0].homeData.indexes[0]]);

        for(int i = 0; i < locationsLength; i++) {
            json_object* currentJson = NULL;
            double* currentDoubleArray = NULL;
            int* currentIntArray = NULL;
            conversionFn conversion = NULL;
            if(ShortNameIs("cape") && TypeOfLevelIs("surface"))
                currentJson = locations[i].cape;
            else if(ShortNameIs("2d")){
                currentJson = locations[i].dewpoint;
                conversion = ToFarenheight;
            }
            else if(ShortNameIs("ltng"))
                currentJson = locations[i].lightning;
            else if(ShortNameIs("prate")) {
                currentDoubleArray = locations[i].precipData.rate;
                conversion = ToInPerHour;
            }
            else if(ShortNameIs("crain") || ShortNameIs("cfrzr") || ShortNameIs("csnow"))
                currentIntArray = locations[i].precipData.type;
            else if(ShortNameIs("mslma")){
                currentJson = locations[i].pressure;
                conversion = ToInHg;
            }
            else if(ShortNameIs("tcc") && TypeOfLevelIs("atmosphere"))
                currentJson = locations[i].totalCloudCover;
            else if(ShortNameIs("2t")){
                currentJson = locations[i].temperature;
                conversion = ToFarenheight;
            }
            else if(ShortNameIs("vis")) {
                currentJson = locations[i].vis;
                conversion = ToMiles;
            }
            else if(ShortNameIs("10si")) {
                currentDoubleArray = locations[i].windData.speed;
                conversion = ToMPH;
            }
            else if(ShortNameIs("gust")){
                currentDoubleArray = locations[i].windData.gust;
                conversion = ToMPH;
            }
            else if(ShortNameIs("10u"))
                currentDoubleArray = locations[i].windData.u;
            else if(ShortNameIs("10v"))
                currentDoubleArray = locations[i].windData.v;

            if(currentJson || currentIntArray || currentDoubleArray)
            {
                json_object* stepArray = NULL;
                if(currentJson)
                {
                    stepArray = json_object_new_array();
                    json_object_array_put_idx(currentJson, fieldData.step, stepArray);
                }

                for(int k = 0; k < NEARBY_SIZE; k++) {
                    double dValue = values[locations[i].homeData.indexes[k]];
                    if(currentIntArray) {
                        if(ShortNameIs("crain"))
                            locations[i].precipData.type[k] |= dValue == 1 ? PTYPE_RAIN : 0;
                        else if(ShortNameIs("cfrzr"))
                            locations[i].precipData.type[k] |= dValue == 1 ? PTYPE_FREEZING_RAIN : 0;
                        else if(ShortNameIs("csnow"))
                            locations[i].precipData.type[k] |= dValue == 1 ? PTYPE_SNOW : 0;
                    } else if(currentDoubleArray)
                        currentDoubleArray[k] = conversion ? conversion(dValue) : dValue;
                    else
                        json_object_array_add(stepArray, json_object_new_double(conversion ? conversion(dValue) : dValue));
                }
            }
        }
        free(values);

        codes_handle_delete(h);
        isFirst = 0;
    }

    for(int i = 0; i < locationsLength; i++)
       FinishStep(lastStep, &locations[i].windData, &locations[i].precipData, locations[i].wind, locations[i].precip);

    codes_fieldset_delete(set);

    char fileName[16] = {0};
    sprintf(fileName, "hrrr-%02d.json", hour);
    json_object_to_file(fileName, root);

    FILE* lastRun = fopen("lastRun", "w");
    fprintf(lastRun, "%02d", hour);
    fclose(lastRun);

    json_object_put(root);
}
