#include "declarations.h"
#include <algorithm>
#include <functional>
#include <png.h>

using namespace std;
typedef union Pixel {
    png_byte bytes[4];
    struct { png_byte r, g, b, a; };
} Pixel;

struct ImageData {
    bool isInFillMode;
    uint32_t width, height;
    FILE* file;
    png_bytep* rows;
    png_structp pPng;
    png_infop pInfo;
    unordered_map<int32_t, Pixel*> virtualPx;

    virtual ~ImageData() {
        for(auto &px : virtualPx)
            delete px.second;
    }
};

inline int32_t KeyFromXY(int16_t &x, int16_t& y) {return x << 16 | y;}

inline void GetXYInts(double dx, double dy, int16_t &x, int16_t& y)
{
    x = static_cast<int16_t>(round(dx));
    y = static_cast<int16_t>(round(dy));
}

inline Pixel* GetPixelInternal(ImageData* imageData, double dx, double dy)
{
    int16_t x, y;
    GetXYInts(dx, dy, x, y);

    if(dx < 0 || dy < 0 || dx >= imageData->width || dy >= imageData->height)
        return imageData->virtualPx[KeyFromXY(x, y)];
    else
        return (Pixel*)&imageData->rows[y][x*4];
}

Pixel* SetPixelInternal(ImageData* imageData, double dx, double dy, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    Pixel* px = nullptr;;
    if(dx < 0 || dy < 0 || dx >= imageData->width || dy >= imageData->height)
    {
        if(imageData->isInFillMode)
            return nullptr;
        
        int16_t x, y;
        GetXYInts(dx, dy, x, y);
        px = new Pixel();
        imageData->virtualPx[KeyFromXY(x, y)] = px;
    }
    else
        px = GetPixelInternal(imageData, dx, dy);

    px->r = r;
    px->g = g;
    px->b = b;
    px->a = a;

    return px;    
}

#define TestSetPixelsItr(itr) if(itr == setPixels.end())\
{\
    cerr << "Ran out of Xs. Choose a wider area to render." << endl;\
    exit(1);\
}

//Assumes setPixels sorted for x
double FindGridDistance(vector<SetPixelData> &setPixels)
{
    auto itr = setPixels.begin();
    double l = 0, r = 0;
    do
    {
        TestSetPixelsItr(itr)
        l = itr->pt.x;
        TestSetPixelsItr(++itr)
        r = itr->pt.x;
    } while (r - l < 0);

    return r - l;
}

ForecastImage::ForecastImage(string fileName, uint32_t width, uint32_t height) 
    : imageData(new ImageData())
{
    imageData->isInFillMode = false;
    imageData->width = width;
    imageData->height = height;
    imageData->file = fopen(fileName.c_str(), "wb");
    if (!imageData->file)
        ERR_OUT(fileName << " could not be opened for writing")

    imageData->pPng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!imageData->pPng)
        ERR_OUT("png_create_write_struct failed")

    imageData->pInfo = png_create_info_struct(imageData->pPng);
    if (!imageData->pInfo)
        ERR_OUT("png_create_info_struct failed")

    if (setjmp(png_jmpbuf(imageData->pPng)))
        ERR_OUT("Error during init_io")

    png_init_io(imageData->pPng, imageData->file);

    if (setjmp(png_jmpbuf(imageData->pPng)))
        ERR_OUT("Error during writing header")

    png_set_IHDR(imageData->pPng, imageData->pInfo, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(imageData->pPng, imageData->pInfo);

    imageData->rows = (png_bytep*)calloc(height, sizeof(png_bytep));
    for (auto y = 0; y < height; y++)
        imageData->rows[y] = (png_byte*) calloc(1, png_get_rowbytes(imageData->pPng, imageData->pInfo));
}

void ForecastImage::SetPixel(double dx, double dy, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    auto px = SetPixelInternal(imageData, dx, dy, r, g, b ,a);
    if(px)
    {
        int16_t x, y;
        setPixels.push_back({dx, dy, px});
    
        GetXYInts(dx, dy, x, y);
        protectedPixels.emplace(KeyFromXY(x, y));
    }
}

bool GetKnownDataPoints(int32_t pointsPerRow, vector<SetPixelData>::iterator itr, SetPixelData &topLeft, SetPixelData &topRight, SetPixelData &bottomLeft, SetPixelData &bottomRight)
{
    topLeft = *itr++;
    topRight = *itr;

    if(topLeft.pt.x > topRight.pt.x)
        return false;

    itr += pointsPerRow - 1;

    bottomLeft = *itr++;
    bottomRight = *itr;

    return true;
}

void ForecastImage::InterpolateFill(int32_t pointsPerRow)
{
    auto itr = setPixels.begin();
    auto setPixelsSize = static_cast<int32_t>(setPixels.size());
    SetPixelData topLeft = {0}, 
                 topRight = {0},
                 bottomLeft = {0},
                 bottomRight = {0};

    imageData->isInFillMode = true;   
    for(auto i = pointsPerRow; i < setPixelsSize; i++, itr++)
    {
        if(!GetKnownDataPoints(pointsPerRow, itr, topLeft, topRight, bottomLeft, bottomRight))
            continue;

        for(auto y = floor(topLeft.pt.y); y < ceil(bottomRight.pt.y); y++)
        for(auto x = floor(bottomLeft.pt.x); x < ceil(topRight.pt.x); x++)
        {
            auto ix = static_cast<int16_t>(x), 
                 iy = static_cast<int16_t>(y);

            if(protectedPixels.find(KeyFromXY(ix, iy)) != protectedPixels.end())
                continue;

            Point pt = {x, y};

            if(IsInsideTriangleOf(floor(topLeft.pt.x), floor(topLeft.pt.y), ceil(topRight.pt.x), floor(topRight.pt.y), ceil(bottomRight.pt.x), ceil(bottomRight.pt.y), x, y)
            || IsInsideTriangleOf(floor(topLeft.pt.x), floor(topLeft.pt.y), floor(bottomLeft.pt.x), ceil(bottomLeft.pt.y), ceil(bottomRight.pt.x), ceil(bottomRight.pt.y), x, y))
            {
                Pixel px = {0};

                for(auto c = 0; c < 4; c++)
                {
                    px.bytes[c] = static_cast<png_byte>(min(255.0, Blerp(
                        {topLeft.pt.x, topLeft.pt.y, static_cast<double>(topLeft.px->bytes[c])},
                        {topRight.pt.x, topRight.pt.y, static_cast<double>(topRight.px->bytes[c])},
                        {bottomLeft.pt.x, bottomLeft.pt.y, static_cast<double>(bottomLeft.px->bytes[c])},
                        {bottomRight.pt.x, bottomRight.pt.y, static_cast<double>(bottomRight.px->bytes[c])},
                        pt
                    )));
                }
                SetPixelInternal(imageData, x, y, px.r, px.g, px.b, px.a);
                protectedPixels.emplace(KeyFromXY(ix, iy));
            }
        }
    }
}

void ForecastImage::Save()
{
#ifdef DEBUG_POINTS
    for(auto &setPx : setPixels)
    {
        setPx.px->r = 255;
        setPx.px->g = 0;
        setPx.px->b = 0;
        setPx.px->a = 255;
    }
#endif

    if (setjmp(png_jmpbuf(imageData->pPng)))
        ERR_OUT("Error during writing bytes");

    png_write_image(imageData->pPng, imageData->rows);

    if (setjmp(png_jmpbuf(imageData->pPng)))
        ERR_OUT("Error during end of write");

    png_write_end(imageData->pPng, NULL);
}

ForecastImage::~ForecastImage()
{
    png_destroy_write_struct(&imageData->pPng, &imageData->pInfo);

    for (auto y = 0; y < imageData->height; y++)
        free(imageData->rows[y]);

    free(imageData->rows);
    fclose(imageData->file);
    delete imageData;
}