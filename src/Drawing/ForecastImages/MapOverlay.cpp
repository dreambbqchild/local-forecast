#include "BarycentricCoordinates.h"
#include "MapOverlay.h"
#include "Calcs.h"
#include "Error.h"


#include <algorithm>
#include <boost/functional/hash.hpp>
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

struct SetPixelData
{
    DoublePoint pt;
    uint8_t* px;
};

template<> 
struct hash<DoublePoint>
{
  size_t operator()(const DoublePoint& k) const
  {
    auto h1 = hash<double>{}(k.x);
    auto h2 = hash<double>{}(k.y);

    size_t seed = 0;
    boost::hash_combine(seed, h1);
    boost::hash_combine(seed, h2);
    return seed;
  }
};

class MapOverlay : public IMapOverlay
{
    private:
        bool isInFillMode = false;
        int32_t width, height;
        shared_ptr<IBitmapContext> bitmapContext;
        vector<SetPixelData> setPixels;
        unordered_map<DoublePoint, uint8_t[4]> virtualPx; 

        DSInt32Size GetSize() { return { width, height }; }

        inline bool RenderableDataPoint(const SetPixelData& data) { return (data.pt.x >= 0 && data.pt.y >= 0) || (data.pt.x <= width && data.pt.y <= height); }

        bool GetKnownDataPoints(int32_t pointsPerRow, vector<SetPixelData>::iterator itr, SetPixelData &topLeft, SetPixelData &topRight, SetPixelData &bottomLeft, SetPixelData &bottomRight)
        {
            topLeft = *itr++;
            topRight = *itr;

            if(topLeft.pt.x > topRight.pt.x)
                return false;

            itr += pointsPerRow - 1;

            bottomLeft = *itr++;
            bottomRight = *itr;

            return RenderableDataPoint(topLeft) || RenderableDataPoint(topRight) || RenderableDataPoint(bottomLeft) || RenderableDataPoint(bottomRight);
        }

        inline uint8_t* GetPixelInternal(DoublePoint pt)
        {
            if(pt.x < 0 || pt.y < 0 || pt.x >= width || pt.y >= height)
                return nullptr;
            else
                return bitmapContext->GetPointer({static_cast<uint16_t>(pt.x), static_cast<uint16_t>(pt.y)});
        }

        inline uint8_t* GetPixelInternal(double dx, double dy) { return GetPixelInternal({dx, dy}); }

        inline uint8_t* SetPixelInternal(DoublePoint pt, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            uint8_t* px = GetPixelInternal(pt);
            if(!px)
                return nullptr;

            px[0] = r;
            px[1] = g;
            px[2] = b;
            px[3] = a;

            return px;    
        }

        inline uint8_t* SetPixelInternal(double dx, double dy, uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return SetPixelInternal({dx, dy}, r, g, b, a); }

        inline void SetPxColorWithWeights(Vector2d& pt, uint8_t values[4][4], double resultWeights[4])
        {
            uint8_t rgba[4] = {0};
            rgba[0] = WeightValues4(values[0], resultWeights);
            rgba[1] = WeightValues4(values[1], resultWeights);
            rgba[2] = WeightValues4(values[2], resultWeights);
            rgba[3] = WeightValues4(values[3], resultWeights);

            SetPixelInternal(pt.a[0], pt.a[1], rgba[0], rgba[1], rgba[2], rgba[3]);
        }

        //Small steps for the first and last to ensure pixel coloration around the borders of the trapazoid.
        inline double CalcNextStep(double v, double start, double end) { return v - start < 1.0 || end - v < 1.0 ? 0.25 : 1; }
        inline double CalcFillStart(double v) { return max(0.0, floor(v));}
        inline double CalcFillEnd(double v, double max) { return min(ceil(v), max); }
        
    public:
        MapOverlay(uint32_t width, uint32_t height) 
        : width(width), height(height), bitmapContext(AllocBitmapContext(width, height)) {}

        void SetPixel(double dx, double dy, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            auto px = SetPixelInternal(dx, dy, r, g, b, a);
            if(!px)
            {
                auto& arr = virtualPx[{dx, dy}];
                arr[0] = r;
                arr[1] = g;
                arr[2] = b;
                arr[3] = a;
                uint8_t* ptr = &arr[0];
                setPixels.push_back({dx, dy, ptr});
            }
            else
                setPixels.push_back({dx, dy, px});
        }

        void InterpolateFill(int32_t pointsPerRow)
        {
            auto itr = setPixels.begin();
            auto setPixelsSize = static_cast<int32_t>(setPixels.size());
            SetPixelData topLeft = {0}, 
                        topRight = {0},
                        bottomLeft = {0},
                        bottomRight = {0};

            isInFillMode = true;   
            for(auto i = pointsPerRow; i < setPixelsSize; i++, itr++)
            {
                if(!GetKnownDataPoints(pointsPerRow, itr, topLeft, topRight, bottomLeft, bottomRight))
                    continue;

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

                    uint8_t values[4][4] = {
                        { topLeft.px[0], topRight.px[0], bottomRight.px[0], bottomLeft.px[0] }, //r
                        { topLeft.px[1], topRight.px[1], bottomRight.px[1], bottomLeft.px[1] }, //g
                        { topLeft.px[2], topRight.px[2], bottomRight.px[2], bottomLeft.px[2] }, //b
                        { topLeft.px[3], topRight.px[3], bottomRight.px[3], bottomLeft.px[3] }  //a
                    };

                    SetPxColorWithWeights(v, values, resultWeights);
                }
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