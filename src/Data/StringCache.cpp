#include "StringCache.h"

#include <unordered_map>

using namespace std;

static unordered_map<string, const char*> internedStrings;

const char* GetCachedString(const char* str)
{
    auto itr = internedStrings.find(str);
    if(itr != internedStrings.end())
        return itr->second;

    auto len = strlen(str) + 1;
    auto interned = (char*)calloc(sizeof(char), len);
    strncpy(interned, str, len);
    internedStrings[str] = interned;

    return interned;
}