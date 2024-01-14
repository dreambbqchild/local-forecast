#ifndef NUMBERFORMAT_H
#define NUMBERFORMAT_H

#include <stdint.h>
#include <iomanip> 
#include <sstream>

template <class TNum>
inline std::string ToStringWithPad(int32_t width, char fill, TNum value)
{
    std::stringstream ss;
    ss << std::setw(width) << std::setfill(fill) << value;
    return ss.str();
}

inline std::string ToStringWithPrecision(int32_t precision, double value)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

#endif