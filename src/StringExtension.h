#pragma once

#include <sstream>
#include <vector>

template <typename T>
inline const char* NumberSuffix(T value)
{
    if((value >= 11 && value <= 13) || value % 10 == 0 || value % 10 >= 4)
        return "th";
    else if(value % 10 == 1)
        return "st";
    else if(value % 10 == 2)
        return "ed";
    else
        return "rd";
}

inline std::string JoinWithCommasButFinishWithAmpersand(std::vector<std::string>& names, bool doSort)
{
    if(names.size() == 1)
        return names[0];

    if(doSort)
        std::sort(names.begin(), names.end());

    std::stringstream nameStream;
    for(auto itr = names.begin(); itr != names.end() - 1; itr++)
    {
        if(itr != names.begin())
            nameStream << ", ";

        nameStream << *itr;
    }

    nameStream << " & " << *(names.end() - 1);
    return nameStream.str();
}

template <typename T>
inline const char* SingularPlural(T value, const char* singular, const char* plural)
{
    return value == 1 ? singular : plural;
}
