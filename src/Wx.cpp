#include "Wx.h"
#include <stdint.h>

double ScaledValueForTypeAndTemp(PrecipitationType type, double value, double temperature)
{
    if((type & PrecipitationType::Snow) != PrecipitationType::Snow)
        return value;

    int32_t scaleFactor = 1;
    if(temperature < -20)
        scaleFactor = 100;
    else if(temperature <= -1)
        scaleFactor = 50;
    else if(temperature <= 9)
        scaleFactor = 40;
    else if(temperature <= 14)
        scaleFactor = 30;
    else if(temperature <= 19)
        scaleFactor = 20;
    else if(temperature <= 27)
        scaleFactor = 15;
    else if(temperature <= 34)
        scaleFactor = 10;

    return scaleFactor * value;
}