#pragma once

#include "common/DataModel.h"

#include <vector>

namespace spt {

double distance(PointYZ a, PointYZ b);
double signedPolygonArea(const std::vector<PointYZ>& polygon);
Contour rectToContour(const RectComponent& rect);
RectComponent plateToBoundingRect(const PlateSegment& plate);
std::vector<PointYZ> plateToPolygon(const PlateSegment& plate);
void normalizeRect(RectComponent& rect);

}  // namespace spt

