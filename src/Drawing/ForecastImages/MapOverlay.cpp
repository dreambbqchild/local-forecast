#include "BarycentricCoordinates.h"
#include "MapOverlay.h"
#include "Calcs.h"
#include "Error.h"


#include <algorithm>
#include <functional>
#include <iostream>
#include <unordered_set>
#include <vector>

using namespace std;

#define TestSetPixelsItr(itr) if(itr == setPixels.end())\
{\
    cerr << "Ran out of Xs. Choose a wider area to render." << endl;\
    exit(1);\
}

class MapOverlay : public IMapOverlay
{
    private:
        int32_t width, height;
        shared_ptr<IBitmapContext> bitmapContext;

        DSInt32Size GetSize() { return { width, height }; }

        inline bool RenderableDataPoint(const MapOverlayPixel& data) { return (data.pt.x >= 0 && data.pt.y >= 0) || (data.pt.x <= width && data.pt.y <= height); }

        inline uint32_t* GetPixelInternal(DoublePoint pt)
        {
            if(pt.x < 0 || pt.y < 0 || pt.x >= width || pt.y >= height)
                return nullptr;
            else
                return reinterpret_cast<uint32_t*>(bitmapContext->GetPointer({static_cast<uint16_t>(pt.x), static_cast<uint16_t>(pt.y)}));
        }

        inline uint32_t* GetPixelInternal(double dx, double dy) { return GetPixelInternal({dx, dy}); }

        inline void SetPixelInternal(DoublePoint pt, const DSColor& color)
        {
            uint32_t* px = GetPixelInternal(pt);
            if(!px)
                return;

            *px = color.rgba;
        }

        inline void SetPixelInternal(double dx, double dy, const DSColor& color) { SetPixelInternal({dx, dy}, color); }

        inline void SetPxColorWithWeights(Vector2d& pt, DSColor values[4], double resultWeights[4])
        {
            uint8_t u8Values[4][4] {
                {values[0].components.r, values[1].components.r, values[2].components.r, values[3].components.r},
                {values[0].components.g, values[1].components.g, values[2].components.g, values[3].components.g},
                {values[0].components.b, values[1].components.b, values[2].components.b, values[3].components.b},
                {values[0].components.a, values[1].components.a, values[2].components.a, values[3].components.a}
            };

            DSColor color {
                .components {
                    .r = WeightValues4(u8Values[0], resultWeights),
                    .g = WeightValues4(u8Values[1], resultWeights),
                    .b = WeightValues4(u8Values[2], resultWeights),
                    .a = WeightValues4(u8Values[3], resultWeights),
                }                
            };

            SetPixelInternal(pt.a[0], pt.a[1], color);
        }

        //Small steps for the first and last to ensure pixel coloration around the borders of the trapazoid.
        inline double CalcNextStep(double v, double start, double end) { return v - start < 1.0 || end - v < 1.0 ? 0.25 : 1; }
        inline double CalcFillStart(double v) { return max(0.0, floor(v));}
        inline double CalcFillEnd(double v, double max) { return min(ceil(v), max); }
        
    public:
        MapOverlay(uint32_t width, uint32_t height) 
        : width(width), height(height), bitmapContext(AllocBitmapContext(width, height)) {}

        void InterpolateFill(const MapOverlayPixel& topLeft, const MapOverlayPixel& topRight, const MapOverlayPixel& bottomLeft, const MapOverlayPixel& bottomRight)
        {
            if(!RenderableDataPoint(topLeft) && !RenderableDataPoint(topRight) && !RenderableDataPoint(bottomLeft) && !RenderableDataPoint(bottomRight))
                return;

            double yStart = CalcFillStart(topLeft.pt.y), 
                    xStart = CalcFillStart(bottomLeft.pt.x), 
                    yEnd = CalcFillEnd(bottomRight.pt.y, height),
                    xEnd = CalcFillEnd(topRight.pt.x, width);

            for(auto y = yStart; y < yEnd; y += CalcNextStep(y, yStart, yEnd))
            for(auto x = xStart; x < xEnd; x += CalcNextStep(x, xStart, xEnd))
            {                     
                Vector2d v = {x, y};
                Vector2d bounds[4] = {
                    {topLeft.pt.x, topLeft.pt.y},
                    {topRight.pt.x, topRight.pt.y},
                    {bottomRight.pt.x, bottomRight.pt.y},
                    {bottomLeft.pt.x, bottomLeft.pt.y}
                };
                double resultWeights[4] = {0};

                if(!BarycentricCoordinatesForCWTetrahedron(v, bounds, resultWeights))
                    continue;

                DSColor values[4] = {
                    {.rgba = topLeft.px.rgba}, 
                    {.rgba = topRight.px.rgba}, 
                    {.rgba = bottomRight.px.rgba}, 
                    {.rgba = bottomLeft.px.rgba}
                };

                SetPxColorWithWeights(v, values, resultWeights);
            }
        }

        shared_ptr<IBitmapContext> GetBitmapContext()
        {
            return bitmapContext;
        }

        void Save(const char* fileName)
        {
            //#define DEBUG_POINTS
            #ifdef DEBUG_POINTS
            for(auto &setPx : setPixels)
            {
                setPx.px[0] = 255;
                setPx.px[1] = 0;
                setPx.px[2] = 0;
                setPx.px[3] = 255;
            }
            #endif

            bitmapContext->Save(fileName);
        }

        virtual ~MapOverlay() = default;
};

IMapOverlay* AllocMapOverlay(int32_t width, int32_t height)
{
    return new MapOverlay(width, height);
}