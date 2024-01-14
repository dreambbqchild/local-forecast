#pragma once

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <ImageIO/ImageIO.h>

#define SAFE_RELEASE(T, ReleaseFn) class SafeRelease ## T : public SafeReleaseBase<T, T> { \
    public: \
        SafeRelease ## T() : SafeReleaseBase<T, T>(ReleaseFn){} \
        SafeRelease ## T(T typeRef) : SafeReleaseBase<T, T>(typeRef, ReleaseFn){} \
        T operator= (const T value) { Set(value); return value; } \
};

template <typename TTypeRef, typename TReleaseType>
class SafeReleaseBase {
    private:
        TTypeRef typeRef;
        void (*Releaser)(TReleaseType);

        inline void Release(){if(typeRef != nullptr) { Releaser(typeRef); typeRef = nullptr; }}

    protected:
        inline void Set(TTypeRef value)
        {
            Release();
            typeRef = value;
        }

    public:
        SafeReleaseBase(void (*Releaser)(TReleaseType)) : typeRef(nullptr), Releaser(Releaser){}
        SafeReleaseBase(TTypeRef typeRef, void (*Releaser)(TReleaseType)) : typeRef(typeRef), Releaser(Releaser){}
        TTypeRef get(){ return typeRef; }
        operator TTypeRef() { return typeRef; }
        virtual ~SafeReleaseBase(){ Release(); }
};

template <typename TTypeRef>
class SafeRelease : public SafeReleaseBase<TTypeRef, CFTypeRef> {
    public:
        SafeRelease(TTypeRef typeRef) : SafeReleaseBase<TTypeRef, CFTypeRef>(typeRef, CFRelease){}
};

class SafeReleaseCFStringRef : public SafeRelease<CFStringRef> {
    public:
        SafeReleaseCFStringRef(const char* str) : SafeRelease<CFStringRef>(CFStringCreateWithCString(kCFAllocatorDefault, str, kCFStringEncodingUTF8)){}
};

class SafeReleaseCFURLRefFileSystem : public SafeRelease<CFURLRef> {
    private:
        static CFURLRef Alloc(const char* url) {
            SafeReleaseCFStringRef cfUrl(url);
            return CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfUrl, kCFURLPOSIXPathStyle, false);
        }

    public:
        SafeReleaseCFURLRefFileSystem(const char* url) : SafeRelease<CFURLRef>(Alloc(url)){}
};

class SafeReleaseCFDictionaryRef : public SafeRelease<CFDictionaryRef> {
    public:
        SafeReleaseCFDictionaryRef(const void** keys, const void** values, CFIndex len) : SafeRelease<CFDictionaryRef>(CFDictionaryCreate(kCFAllocatorDefault, keys, values, len, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)){}
};

class SafeReleaseInt32 : public SafeRelease<CFNumberRef> {
    public:
        SafeReleaseInt32(int32_t value) : SafeRelease<CFNumberRef>(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value)){}
};

class SafeReleaseCGColorRef : public SafeReleaseBase<CGColorRef, CGColorRef>  {
    private:
        CGColorRef Alloc(CGColorSpaceRef colorSpace, double r, double g, double b, double a)
        {
            CGFloat rgba[] = {r, g, b, a};
            return CGColorCreate(colorSpace, rgba);
        }
    public:
        SafeReleaseCGColorRef(CGColorSpaceRef colorSpace, double r, double g, double b, double a = 1.0) : SafeReleaseBase<CGColorRef, CGColorRef>(Alloc(colorSpace, r, g, b, a), CGColorRelease){};
};

SAFE_RELEASE(CGContextRef, CGContextRelease)
SAFE_RELEASE(CGColorSpaceRef, CGColorSpaceRelease)
SAFE_RELEASE(CGImageRef, CGImageRelease)
SAFE_RELEASE(CGPathRef, CGPathRelease)