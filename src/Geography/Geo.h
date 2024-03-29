#pragma once

#include "Calcs.h"
#include "Data/SelectedRegion.h"

#include <vector>

struct GeoCoordPoint : GeoCoord {
    double x, y;
};

class GeographicCalcs {
private:
  int16_t imageWidth;

protected:
  const int16_t imageHeight = 1164;
  double leftX = 0, rightX = 0, topY = 0, bottomY = 0, width = 0, height = 0, xScale = 0, yScale = 0;
  
public:
  GeographicCalcs(const SelectedRegion& selectedRegion);
  Int16Size Bounds() {return {imageWidth, imageHeight}; }
  DoublePoint FindXY(const GeoCoord& coords);
  virtual ~GeographicCalcs() = default;
};

class IGeoPointSet {
public:
  virtual void GetBoundingBox(const Location& location, GeoCoordPoint resultIndexes[4]) = 0;
  virtual ~IGeoPointSet() = default;
};

IGeoPointSet* AllocGeoPointSet(const std::vector<GeoCoord>& geoCoords, GeographicCalcs& geoCalcs);

double CalcDistanceInMetersBetweenCoords(GeoCoord first, GeoCoord second);