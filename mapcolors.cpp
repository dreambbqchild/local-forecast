#include "mapcolors.h"

#define TEMP_COLOR(temp, r, g, b) if(farenheight >= temp) return {r, g, b, 255}
MapColor ColorFromDegrees(double farenheight) 
{
    TEMP_COLOR(125, 238, 238, 238);
    TEMP_COLOR(120, 255, 232, 232);
    TEMP_COLOR(115, 225, 207, 207);
    TEMP_COLOR(110, 255, 181, 181);
    TEMP_COLOR(105, 255, 158, 158);
    TEMP_COLOR(100, 255, 135, 135);
    TEMP_COLOR( 95, 255, 104, 104);
    TEMP_COLOR( 90, 255,  69,  69);
    TEMP_COLOR( 85, 255,   0,   0);
    TEMP_COLOR( 80, 255,  79,   0);
    TEMP_COLOR( 75, 255, 127,   0);
    TEMP_COLOR( 70, 255, 153,   0);
    TEMP_COLOR( 65, 255, 174,   0);
    TEMP_COLOR( 60, 255, 204,   0);
    TEMP_COLOR( 55, 255, 230,   0);
    TEMP_COLOR( 50, 255, 255,   0);
    TEMP_COLOR( 45, 206, 255,   0);
    TEMP_COLOR( 40, 127, 255,   0);
    TEMP_COLOR( 35,   0, 255, 179);
    TEMP_COLOR( 30,   0, 255, 255);
    TEMP_COLOR( 25,   0, 230, 255);
    TEMP_COLOR( 20,   0, 204, 255);
    TEMP_COLOR( 15,   0, 163, 255);
    TEMP_COLOR( 10,   0, 115, 255);
    TEMP_COLOR(  5,   0,  74, 255);
    TEMP_COLOR(  0,   0,   0, 255);
    TEMP_COLOR( -5, 102,   0, 255);
    TEMP_COLOR(-10, 158,   0, 255);
    TEMP_COLOR(-15, 209,   0, 255);
    TEMP_COLOR(-20, 255,   0, 255);
    TEMP_COLOR(-25, 230,   0, 230);
    TEMP_COLOR(-30, 204,   0, 204);
    TEMP_COLOR(-35, 179,   0, 179);
    TEMP_COLOR(-40, 153,   0, 153);
    TEMP_COLOR(-45, 130,   0, 130);
    return {107, 0, 107, 255};
}

#define RATE_COLOR(p, r, g, b) if(rate >= p) return {r, g, b, 255}
MapColor ColorFromPrecipitation(PrecipitationType precipitationType, double rate)
{
    if(rate == 0)
        return {0};
    
    if((precipitationType & PrecipitationType::Snow) == PrecipitationType::Snow)
    {
        RATE_COLOR(16.0,   0,   0, 100); //65+
        RATE_COLOR(8.00,   0,   0, 100); //60
        RATE_COLOR(4.00,   0,   0, 150); //55
        RATE_COLOR(1.90,   0,   0, 150); //50
        RATE_COLOR(0.92,   0,   0, 200); //45
        RATE_COLOR(0.45,   0,   0, 200); //40
        RATE_COLOR(0.22,   0,  42, 225); //35
        RATE_COLOR(0.10,   0,  42, 225); //30
        RATE_COLOR(0.05,   0,  85, 225); //25
        RATE_COLOR(0.01,   0, 128, 225); //20
    }
    else if((precipitationType & PrecipitationType::FreezingRain) == PrecipitationType::FreezingRain)
    {
        RATE_COLOR(16.0,  68,  34,  39); //65+
        RATE_COLOR(8.00, 102,  51,  59); //60
        RATE_COLOR(4.00, 115,  54,  60); //55
        RATE_COLOR(1.90, 128,  56,  60); //50
        RATE_COLOR(0.92, 140,  59,  61); //45
        RATE_COLOR(0.45, 153,  61,  61); //40
        RATE_COLOR(0.22, 172,  66,  63); //35
        RATE_COLOR(0.10, 191,  72,  65); //30
        RATE_COLOR(0.05, 210,  77,  67); //25
        RATE_COLOR(0.01, 229,  82,  69); //20
    }
    else
    {
        RATE_COLOR(16.0, 255, 153, 255); //65+
        RATE_COLOR(8.00, 255,  80, 255); //60
        RATE_COLOR(4.00, 255,   0, 255); //55
        RATE_COLOR(1.90, 255,   0,   0); //50
        RATE_COLOR(0.92, 255, 120,   0); //45
        RATE_COLOR(0.45, 255, 200,   0); //40
        RATE_COLOR(0.22, 255, 255,   0); //35
        RATE_COLOR(0.10,   0, 125,   0); //30
        RATE_COLOR(0.05,   0, 153,   0); //25
        RATE_COLOR(0.01,   0, 170,   0); //20
    }

    return {0};
}