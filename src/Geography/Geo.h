#pragma once

#include "Calcs.h"

#include <vector>

const double leftlon = -93.63166870117188, rightlon = -92.85987915039063, toplat = 45.183375349113845, bottomlat = 44.61724084030939;

struct GeoCoord {
    double lat, lon;
};

struct DetailedGeoCoord : GeoCoord {
    uint16_t index;
    double x, y;
};

class GeographicCalcs {
private:
  int16_t imageWidth;

protected:
  const int16_t imageHeight = 1164;
  double leftX = 0, rightX = 0, topY = 0, bottomY = 0, width = 0, height = 0, xScale = 0, yScale = 0;
  
public:
  GeographicCalcs(double toplat, double leftlon, double bottomlat, double rightlon);
  Int16Size Bounds() {return {imageWidth, imageHeight}; }
  DoublePoint FindXY(double lat, double lon);
  virtual ~GeographicCalcs() = default;
};

class IGeoPointSet {
public:
  virtual void GetBoundingBox(uint32_t loc, uint32_t pointsPerRow, DetailedGeoCoord resultIndexes[4]) = 0;
  virtual ~IGeoPointSet() = default;
};

IGeoPointSet* AllocGeoPointSet(std::vector<GeoCoord>& geoCoords, GeographicCalcs& geoCalcs);

double CalcDistanceInMetersBetweenCoords(GeoCoord first, GeoCoord second);