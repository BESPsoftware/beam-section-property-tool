#include "stress/StressPointEngine.h"

#include "geometry/GeometryUtils.h"

#include <algorithm>
#include <cmath>

namespace spt {
namespace {

StressPoint makePoint(int id, PointYZ global, const SectionProperties& properties) {
    StressPoint point;
    point.id = id;
    point.label = std::to_string(id);
    point.global = global;
    point.principal = StressPointEngine::toPrincipal(global, properties);
    point.source = StressPointSource::Default;
    point.validity = StressPointValidity::Valid;
    return point;
}

}  // namespace

std::vector<StressPoint> StressPointEngine::generateDefault(const SectionModel& model, const SectionProperties& properties) {
    if (!model.overrideStressPoints.empty()) {
        return model.overrideStressPoints;
    }

    std::vector<StressPoint> points;
    switch (model.type) {
        case SectionType::HSection: {
            const double H = model.parameters.get("H");
            const double f = model.parameters.get("f");
            const std::vector<PointYZ> principal = {
                {-0.5 * f, -0.5 * H},
                {0.5 * f, -0.5 * H},
                {0.5 * f, 0.5 * H},
                {-0.5 * f, 0.5 * H},
            };
            int id = 1;
            for (const auto& p : principal) {
                StressPoint point;
                point.id = id;
                point.label = std::to_string(id);
                point.principal = p;
                point.global = fromPrincipal(p, properties);
                points.push_back(point);
                ++id;
            }
            break;
        }
        case SectionType::BoxSection: {
            const double A = model.parameters.get("A");
            const double B = model.parameters.get("B");
            const double H = model.parameters.get("H");
            const double E = model.parameters.get("E");
            const double E1 = model.parameters.get("E1");
            const double upperOffset = 0.5 * (A - B) - E;
            const double lowerOffset = 0.5 * (A - B) - E1;
            points.push_back(makePoint(1, {lowerOffset, 0.0}, properties));
            points.push_back(makePoint(2, {A - lowerOffset, 0.0}, properties));
            points.push_back(makePoint(3, {A - upperOffset, H}, properties));
            points.push_back(makePoint(4, {upperOffset, H}, properties));
            break;
        }
        case SectionType::PipeSection: {
            const double r = 0.5 * model.parameters.get("Do");
            points.push_back(makePoint(1, {r, 0.0}, properties));
            points.push_back(makePoint(2, {0.0, r}, properties));
            points.push_back(makePoint(3, {-r, 0.0}, properties));
            points.push_back(makePoint(4, {0.0, -r}, properties));
            break;
        }
        case SectionType::QuaysideCraneGirder:
        case SectionType::CanvasThinWalled: {
            int id = 1;
            for (const auto& plate : model.plates) {
                if (id > 4) {
                    break;
                }
                points.push_back(makePoint(id++, plate.start, properties));
                if (id > 4) {
                    break;
                }
                points.push_back(makePoint(id++, plate.end, properties));
            }
            break;
        }
    }
    return points;
}

PointYZ StressPointEngine::toPrincipal(PointYZ global, const SectionProperties& properties) {
    const double c = std::cos(properties.theta);
    const double s = std::sin(properties.theta);
    const double dy = global.y - properties.cy;
    const double dz = global.z - properties.cz;
    return {c * dy + s * dz, -s * dy + c * dz};
}

PointYZ StressPointEngine::fromPrincipal(PointYZ principal, const SectionProperties& properties) {
    const double c = std::cos(properties.theta);
    const double s = std::sin(properties.theta);
    const double dy = c * principal.y - s * principal.z;
    const double dz = s * principal.y + c * principal.z;
    return {properties.cy + dy, properties.cz + dz};
}

StressPointValidity StressPointEngine::validate(PointYZ global, const SectionModel& model, double tolerance) {
    if (model.rectangles.empty() && model.type != SectionType::PipeSection) {
        return StressPointValidity::Disconnected;
    }
    if (model.type == SectionType::PipeSection) {
        const double rOuter = 0.5 * model.parameters.get("Do");
        const double rInner = rOuter - model.parameters.get("t");
        const double r = std::sqrt(global.y * global.y + global.z * global.z);
        return (r <= rOuter + tolerance && r >= rInner - tolerance) ? StressPointValidity::Valid : StressPointValidity::OutsideSection;
    }
    for (const auto& input : model.rectangles) {
        RectComponent rect = input;
        normalizeRect(rect);
        if (global.y >= rect.y1 - tolerance && global.y <= rect.y2 + tolerance &&
            global.z >= rect.z1 - tolerance && global.z <= rect.z2 + tolerance) {
            return StressPointValidity::Valid;
        }
    }
    return StressPointValidity::OutsideSection;
}

}  // namespace spt

