#pragma once

#include <string>

const char* GetCachedString(const char* str);
inline const char* GetCachedString(std::string str) { return GetCachedString(str.c_str()); }