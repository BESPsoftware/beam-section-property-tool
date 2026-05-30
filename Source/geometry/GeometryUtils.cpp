#include "geometry/GeometryUtils.h"

#include <algorithm>
#include <cmath>

namespace spt {

double distance(PointYZ a, PointYZ b) {
    const double dy = b.y - a.y;
    const double dz = b.z - a.z;
    return std::sqrt(dy * dy + dz * dz);
}

double signedPolygonArea(const std::vector<PointYZ>& polygon) {
    if (polygon.size() < 3) {
        return 0.0;
    }
    double twiceArea = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const auto& p = polygon[i];
        const auto& q = polygon[(i + 1) % polygon.size()];
        twiceArea += p.y * q.z - q.y * p.z;
    }
    return 0.5 * twiceArea;
}

Contour rectToContour(const RectComponent& rect) {
    RectComponent r = rect;
    normalizeRect(r);
    Contour contour;
    contour.outer = {
        {r.y1, r.z1},
        {r.y2, r.z1},
        {r.y2, r.z2},
        {r.y1, r.z2},
    };
    return contour;
}

RectComponent plateToBoundingRect(const PlateSegment& plate) {
    const auto polygon = plateToPolygon(plate);
    RectComponent rect;
    rect.id = plate.id;
    rect.materialId = plate.materialId;
    if (polygon.empty()) {
        return rect;
    }
    rect.y1 = rect.y2 = polygon.front().y;
    rect.z1 = rect.z2 = polygon.front().z;
    for (const auto& p : polygon) {
        rect.y1 = std::min(rect.y1, p.y);
        rect.y2 = std::max(rect.y2, p.y);
        rect.z1 = std::min(rect.z1, p.z);
        rect.z2 = std::max(rect.z2, p.z);
    }
    return rect;
}

std::vector<PointYZ> plateToPolygon(const PlateSegment& plate) {
    const double length = distance(plate.start, plate.end);
    if (length <= 0.0 || plate.thickness <= 0.0) {
        return {};
    }
    const double uy = (plate.end.y - plate.start.y) / length;
    const double uz = (plate.end.z - plate.start.z) / length;
    const double ny = -uz;
    const double nz = uy;
    const double half = 0.5 * plate.thickness;
    return {
        {plate.start.y + ny * half, plate.start.z + nz * half},
        {plate.end.y + ny * half, plate.end.z + nz * half},
        {plate.end.y - ny * half, plate.end.z - nz * half},
        {plate.start.y - ny * half, plate.start.z - nz * half},
    };
}

void normalizeRect(RectComponent& rect) {
    if (rect.y2 < rect.y1) {
        std::swap(rect.y1, rect.y2);
    }
    if (rect.z2 < rect.z1) {
        std::swap(rect.z1, rect.z2);
    }
}

}  // namespace spt

