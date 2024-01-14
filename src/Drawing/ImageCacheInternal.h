#pragma once
#include "ImageCache.h"
#include "SafeReleases.h"

CGImageRef LoadImageFromFile(const char* pathToImageFile);

class FileSystemImage : public IImage {
private:
    int32_t width, height;
    SafeReleaseCGImageRef image;

public:
    FileSystemImage(const std::filesystem::path& path) : image(LoadImageFromFile(path.c_str()))
    {
        width = CGImageGetWidth(image);
        height = CGImageGetWidth(image);
    }

    DSSize GetSize() { return { (double)width, (double)height }; }
    CGImageRef GetImageRef() { return image; }
};

FileSystemImage* GetCachedImageInternal(const std::filesystem::path& path);