#include "ImageCacheInternal.h"

IImage* AllocImage(const std::filesystem::path& path) { return new FileSystemImage(path); }

std::unordered_map<std::string, FileSystemImage*> cache;

IImage* ImageCache::GetCachedImage(const std::filesystem::path& path) { return GetCachedImageInternal(path); }

void ImageCache::CacheImageAt(const std::filesystem::path& path)
{
    if(GetCachedImage(path))
        return;

    cache[path] = new FileSystemImage(path);
}

CGImageRef LoadImageFromFile(const char* pathToImageFile)
{
    SafeReleaseCFURLRefFileSystem imageUrl(pathToImageFile);
    SafeRelease<CGImageSourceRef> imageSrc(CGImageSourceCreateWithURL(imageUrl, nullptr));
    return CGImageSourceCreateImageAtIndex(imageSrc, 0, nullptr);
}

FileSystemImage* GetCachedImageInternal(const std::filesystem::path& path)
{
    auto itr = cache.find(path);
    if(itr == cache.end())
        return nullptr;
    
    return itr->second;
}