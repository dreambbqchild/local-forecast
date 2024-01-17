#include "DrawServiceObjC.h"
#include "ImageCacheInternal.h"

class DrawService : public IDrawService
{
    private:
        int32_t width, height;
        SafeReleaseCGColorSpaceRef colorSpace;
        SafeReleaseCGContextRef context;

        inline void OriginToTopLeft(DSRect& rect) {rect.origin.y = height - rect.origin.y - rect.size.height;}
        inline void OriginToTopLeft(DSPoint& pt) {pt.y = height - pt.y;}

        void DrawCroppedImage(CGImageRef image, DSRect& sourceImageBounds, DSRect& drawAtWithinBounds, double alpha)
        {
            SafeReleaseCGImageRef croppedImage(CGImageCreateWithImageInRect(image, ToCG(sourceImageBounds)));

            if(alpha != 1.0)
                CGContextSetAlpha(context, alpha);

            OriginToTopLeft(drawAtWithinBounds);

            CGContextDrawImage(context, ToCG(drawAtWithinBounds), croppedImage);

            if(alpha != 1.0)
                CGContextSetAlpha(context, 1.0);
        }

    public:
        DrawService(int32_t width, int32_t height) 
            : width(width), height(height), colorSpace(CGColorSpaceCreateDeviceRGB()), context(CGBitmapContextCreate(nullptr, width, height, bitsPerComponent, 0, colorSpace, kCGImageAlphaPremultipliedLast)) {}

        DSSize GetSize() 
        {
            return { (double)width, (double)height };
        }

        void ClearCanvas(DSColor color)
        {
            CGContextSetRGBFillColor(context, ToComponentDouble(color.components.r), ToComponentDouble(color.components.g), ToComponentDouble(color.components.b), ToComponentDouble(color.components.a));
            CGContextFillRect(context, {0, 0, (double)width, (double)height});
        }

        void SetDropShadow(DSSize offset, double blur)
        {
            CGContextSetShadow(context, ToCG(offset), blur);
        }

        void ClearDropShadow()
        {
            CGContextSetShadowWithColor(context, {0, 0}, 0, nullptr);
        }

        //Internal function intended for use by the BitmapContext.
        void DrawCGImage(CGImageRef image)
        {
            CGContextDrawImage(context, {0, 0, static_cast<double>(width), static_cast<double>(height)}, image);
        }

        void DrawImage(const char* pathToImageFile, DSRect drawAtWithinBounds, double alpha = 1.0)
        {
            CGImageRef image = nullptr;
            SafeReleaseCGImageRef safeReleaseImage;            
            auto cachedImage = GetCachedImageInternal(pathToImageFile);            
            if(!cachedImage)
            {
                safeReleaseImage = LoadImageFromFile(pathToImageFile);
                image = safeReleaseImage.get();
            }
            else
                image = cachedImage->GetImageRef();

            if(alpha != 1.0)
                CGContextSetAlpha(context, alpha);

            OriginToTopLeft(drawAtWithinBounds);

            CGContextDrawImage(context, ToCG(drawAtWithinBounds), image);

            if(alpha != 1.0)
                CGContextSetAlpha(context, 1.0);
        }

        void DrawCroppedImage(std::shared_ptr<IDrawService> drawService, DSRect sourceImageBounds, DSRect drawAtWithinBounds, double alpha)
        {
            auto other = dynamic_cast<DrawService*>(drawService.get());
            SafeReleaseCGImageRef image(CGBitmapContextCreateImage(other->context));

            DrawCroppedImage(image, sourceImageBounds, drawAtWithinBounds, alpha);
        }

        void DrawCroppedImage(std::shared_ptr<IImage> image, DSRect sourceImageBounds, DSRect drawAtWithinBounds, double alpha)
        {
            auto pImage = dynamic_cast<FileSystemImage*>(image.get());
            DrawCroppedImage(pImage->GetImageRef(), sourceImageBounds, drawAtWithinBounds, alpha);
        }

        void DrawCroppedImage(const char* pathToImageFile, DSRect sourceImageBounds, DSRect drawAtWithinBounds, double alpha)
        {
            SafeReleaseCGImageRef image(LoadImageFromFile(pathToImageFile));
            DrawCroppedImage(image, sourceImageBounds, drawAtWithinBounds, alpha);
        }

        DSRect DrawText(std::function<DSRect(IDrawTextContext*)> callback)
        {
            DSRect textBounds;
            SafeReleaseCGImageRef textImage(RunDrawTextContext(callback, textBounds));
            DSRect macBounds = textBounds;

            OriginToTopLeft(macBounds);

            CGContextDrawImage(context, ToCG(macBounds), textImage);
            return textBounds;
        }

        void MoveTo(DSPoint pt)
        {
            OriginToTopLeft(pt);
            CGContextMoveToPoint(context, pt.x, pt.y);
        }

        void LineTo(DSPoint pt)
        {
            OriginToTopLeft(pt);
            CGContextAddLineToPoint(context, pt.x, pt.y); 
        }

        void AddRoundedRectangle(DSRect bounds, double cx, double cy)
        {
            OriginToTopLeft(bounds);
            SafeReleaseCGPathRef roundedRectPath(CGPathCreateWithRoundedRect(ToCG(bounds), cx, cy, nullptr));
            CGContextAddPath(context, roundedRectPath);
        }

        void AddRectangle(DSRect bounds)
        {
            OriginToTopLeft(bounds);
            SafeReleaseCGPathRef roundedRectPath(CGPathCreateWithRect(ToCG(bounds), nullptr));
            CGContextAddPath(context, roundedRectPath);
        }

        void SetFillColor(DSColor color)
        {
            CGContextSetRGBFillColor(context, ToComponentDouble(color.components.r), ToComponentDouble(color.components.g), ToComponentDouble(color.components.b), ToComponentDouble(color.components.a));
        }

        void SetStrokeColor(DSColor color)
        {
            CGContextSetRGBStrokeColor(context, ToComponentDouble(color.components.r), ToComponentDouble(color.components.g), ToComponentDouble(color.components.b), ToComponentDouble(color.components.a));
        }

        void FillActivePath()
        {
            CGContextFillPath(context);
        }

        void StrokeActivePath(double thickness)
        {
            CGContextSetLineWidth(context, thickness);
            CGContextStrokePath(context);
        }

        void Save(const char* outfilePath)
        {
            SafeReleaseCGImageRef image(CGBitmapContextCreateImage(context));
            SafeReleaseCFURLRefFileSystem url(outfilePath);    
            SafeRelease<CGImageDestinationRef> destination(CGImageDestinationCreateWithURL(url, pngIdentifier, 1, nullptr));

            const void* destinationKeys[] = {kCGImagePropertyDPIWidth, kCGImagePropertyDPIHeight};
            const void* destinationValues[] = {imageDpi.get(), imageDpi.get()};
            SafeReleaseCFDictionaryRef destinationProperties(destinationKeys, destinationValues, 2);

            CGImageDestinationAddImage(destination, image, destinationProperties);
            CGImageDestinationFinalize(destination);
        }
};

IDrawService* AllocDrawService(int32_t width, int32_t height) { return new DrawService(width, height); }
DrawService* AllocDrawServiceClass(int32_t width, int32_t height) { return new DrawService(width, height); }

class BitmapContext : public IBitmapContext
{
private:
    int32_t width, height;
    uint8_t* rows;

    CGImageRef GetAsCGImage()
    {
        SafeReleaseCGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        SafeReleaseCGContextRef context = CGBitmapContextCreate(rows, width, height, 8, 4 * width, colorSpace, kCGImageAlphaPremultipliedLast);
        return CGBitmapContextCreateImage(context);
    }

public:
    BitmapContext(int32_t width, int32_t height)
        : width(width), height(height)
    {
        rows = (uint8_t*)calloc(height, 4 * width);
    }

    void SetPixel(DSUInt16Point pt, DSColor color)
    {
        auto ptr = GetPointer(pt);
        *ptr = color.rgba;
    }

    DSColor GetPixel(DSUInt16Point pt)
    {
        auto ptr = GetPointer(pt);
        return { .rgba = *ptr };
    }

    uint8_t* GetPointer(DSUInt16Point pt)
    {
        return &rows[(width * 4 * pt.y) + (pt.x * 4)];
    };

    void Save(const char* outfilePath)
    {
        SafeReleaseCGImageRef image(GetAsCGImage());
        SafeReleaseCFURLRefFileSystem url(outfilePath);    
        SafeRelease<CGImageDestinationRef> destination(CGImageDestinationCreateWithURL(url, pngIdentifier, 1, nullptr));

        CGImageDestinationAddImage(destination, image, nullptr);
        CGImageDestinationFinalize(destination);
    }

    IDrawService* ToDrawService()
    {
        auto drawService = AllocDrawServiceClass(width, height);
        SafeReleaseCGImageRef image(GetAsCGImage());
        drawService->DrawCGImage(image);
        return drawService;
    }

    virtual ~BitmapContext()
    {
        free(rows);
    }
};

IBitmapContext* AllocBitmapContext(int32_t width, int32_t height)
{
    return new BitmapContext(width, height);
}

DSRect CalcRectForCenteredLocation(DSSize area, DSSize rendering, DSPoint offset)
{
    auto x = (area.width * 0.5) - (rendering.width * 0.5);
    auto y = (area.height * 0.5) - (rendering.height * 0.5);
    return {x + offset.x, y + offset.y, rendering.width, rendering.height};
}