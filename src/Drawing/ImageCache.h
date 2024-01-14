#pragma once

#include "DrawService.h"

class IImage {
public:
    virtual DSSize GetSize() = 0;
    virtual ~IImage() = default;
};

IImage* AllocImage(const std::filesystem::path& path);

class ImageCache {
public:
    static IImage* GetCachedImage(const std::filesystem::path& path);
    static void CacheImageAt(const std::filesystem::path& path);
};