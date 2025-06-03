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
    virtual void UseAudioFile(const std::string& audioPath) = 0;
    virtual void EncodeImagesFittingPattern(const std::string& pattern, int32_t frameDuration, const std::string& background) = 0;
    inline void EncodeImagesFittingPattern(const std::string& pattern, const std::string& background) { EncodeImagesFittingPattern(pattern, 1, background); };
    virtual void EncodeImagesFittingPattern(const std::string& pattern, int32_t frameDuration = 1) = 0;
    virtual void Close() = 0;
    virtual ~IEncoder() = default;
};

IEncoder* AllocEncoder(std::filesystem::path path, int64_t bitRate = 150000, int32_t width = 1060, int32_t height = 1100, FPS baseFps = {1, 2});