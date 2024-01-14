#pragma once

#include <stdint.h>

#include <filesystem>

struct FPS 
{ 
    int32_t numerator, denominator;
};

class IEncoder 
{
public:
    virtual void UseAudioFile(const char* audioPath) = 0;
    inline void UseAudioFile(std::string audioPath) { UseAudioFile(audioPath.c_str());}
    virtual void EncodeImagesFittingPattern(const char* pattern, int32_t frameDuration = 1) = 0;
    inline void EncodeImagesFittingPattern(std::string pattern, int32_t frameDuration = 1) { EncodeImagesFittingPattern(pattern.c_str(), frameDuration); }
    virtual void Close() = 0;
    virtual ~IEncoder() = default;
};

IEncoder* AllocEncoder(std::filesystem::path path, int64_t bitRate = 150000, int32_t width = 1060, int32_t height = 1100, FPS baseFps = {1, 2});