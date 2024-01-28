#include "Geo.h"

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/Constants.hpp>
#include <GeographicLib/NearestNeighbor.hpp>
#include <GeographicLib/UTMUPS.hpp>

#include <iostream>

using namespace std;
using namespace GeographicLib;

Geodesic geod(Constants::WGS84_a(), Constants::WGS84_f());

class DistanceFunctor {
  public:
    double operator() (const GeoCoord& a, const GeoCoord& b) const
    {
        double d = 0.0;
        geod.Inverse(a.lat, a.lon, b.lat, b.lon, d);
        
        if ( !(d >= 0) )
            throw GeographicErr("distance doesn't satisfy d >= 0");

        return d;
    }
} distanceFunctor;

inline void ForwardInternal(double lat, double lon, double& x, double& y)
{
    int32_t zone = 0;
    bool isNorthernHemisphere = true;
    UTMUPS::Forward(lat, lon, zone, isNorthernHemisphere, x, y);
}

GeographicCalcs::GeographicCalcs(const SelectedRegion& selectedLocation) 
{
    auto& bounds = selectedLocation.GetRegionBounds();
    auto metersWidth = CalcDistanceInMetersBetweenCoords({bounds.topLat, bounds.leftLon}, {bounds.topLat, bounds.rightLon});
    auto metersHeight = CalcDistanceInMetersBetweenCoords({bounds.topLat, bounds.leftLon}, {bounds.bottomLat, bounds.leftLon});
    imageWidth = static_cast<int16_t>(ceil((metersWidth / metersHeight) * imageHeight));

    ForwardInternal(bounds.topLat, bounds.leftLon, leftX, topY);
    ForwardInternal(bounds.bottomLat, bounds.rightLon, rightX, bottomY);
    width = rightX - leftX;
    height = topY - bottomY;
    xScale = imageWidth / width;
    yScale = imageHeight / height;
}

DoublePoint GeographicCalcs::FindXY(const GeoCoord& coords)
{
    DoublePoint pt = {0};
    ForwardInternal(coords.lat, coords.lon, pt.x, pt.y);
    pt.x = pt.x - leftX;
    pt.y = pt.y - bottomY;
    pt.y = height - pt.y; //Remember: inverted because bigger values are more north.
    pt.x *= xScale;
    pt.y *= yScale;
    return pt;
}

class GeoPointSet : public IGeoPointSet 
{
private:
  NearestNeighbor<double, GeoCoord, DistanceFunctor> pointSet;
  vector<GeoCoord> geoCoords;

  GeographicCalcs& geoCalcs;

public:
    GeoPointSet(vector<GeoCoord>& geoCoords, GeographicCalcs& geoCalcs) 
        : geoCoords(geoCoords), geoCalcs(geoCalcs)
    {
        pointSet.Initialize(geoCoords, distanceFunctor);        
    }

    void GetBoundingBox(const Location& location, uint32_t pointsPerRow, DetailedGeoCoord resultIndexes[4])
    {
        vector<int32_t> indexes;
        vector<DetailedGeoCoord> nearPoints;
        GeoCoord geoPt = {location.lat, location.lon};
        if(geoPt.lon < 0)
            geoPt.lon += 360;

        pointSet.Search(geoCoords, distanceFunctor, geoPt, indexes, 4);
        for(auto &i : indexes)
        {
            auto geoCoord = geoCoords[i];
            auto pt = geoCalcs.FindXY(geoCoord);
            nearPoints.push_back(DetailedGeoCoord {
                {   //GeoCoord Part
                    .lat = geoCoord.lat, 
                    .lon = geoCoord.lon
                },
                .index = static_cast<uint16_t>(i), 
                .x = pt.x, 
                .y = pt.y
            });
        }

        for(auto& pt : nearPoints)
        {
            //Lon x, lat y
            if(pt.lon <= geoPt.lon && pt.lat <= geoPt.lat)
                resultIndexes[0] = pt;
            else if(pt.lon >= geoPt.lon && pt.lat <= geoPt.lat)
                resultIndexes[1] = pt;
            else if(pt.lon >= geoPt.lon && pt.lat >= geoPt.lat)
                resultIndexes[2] = pt;
            else if(pt.lon <= geoPt.lon && pt.lat >= geoPt.lat)
                resultIndexes[3] = pt;
            else
            {
                cerr << "Bad Pt" << endl;
                exit(1);
            }
        }
    }
};

IGeoPointSet* AllocGeoPointSet(vector<GeoCoord>& geoCoords, GeographicCalcs& geoCalcs)
{
    return new GeoPointSet(geoCoords, geoCalcs);
}

double CalcDistanceInMetersBetweenCoords(GeoCoord first, GeoCoord second)
{
    auto line = geod.InverseLine(first.lat, first.lon, second.lat, second.lon);
    return line.Distance();
}