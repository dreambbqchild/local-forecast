#pragma once
#include "DrawService.h"
#include "SafeReleases.h"

extern CFStringRef pngIdentifier;
extern SafeReleaseInt32 imageDpi;
const size_t bitsPerComponent = 8;

inline double ToComponentDouble(uint32_t value) { return value / (double)((1 << bitsPerComponent) - 1); }

inline CGPoint ToCG(const DSPoint& value) {return {value.x, value.y}; }
inline CGSize ToCG(const DSSize& value) { return {value.width, value.height}; }
inline CGRect ToCG(const DSRect& value) { return {{value.origin.x, value.origin.y},{value.size.width, value.size.height}}; }

inline DSPoint FromCG(const CGPoint& value) { return {value.x, value.y}; }
inline DSSize FromCG(const CGSize& value) { return {value.width, value.height};}
inline DSRect FromCG(const CGRect& value) { return {{value.origin.x, value.origin.y},{value.size.width, value.size.height}}; }

class IDrawTextContextObjC : public IDrawTextContext {
public:
    virtual CGImageRef CreateImageRef() = 0;
    virtual ~IDrawTextContextObjC() = default;
};

CGImageRef RunDrawTextContext(std::function<DSRect(IDrawTextContext*)> callback, DSRect& textBounds);