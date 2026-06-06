#include "calculation/SectionCalculator.h"

#include "geometry/GeometryUtils.h"
#include "stress/StressPointEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace spt {
namespace {

double rectArea(const RectComponent& input) {
    RectComponent rect = input;
    normalizeRect(rect);
    return (rect.y2 - rect.y1) * (rect.z2 - rect.z1);
}

double sqr(double value) {
    return value * value;
}

bool samePoint(const PointYZ& a, const PointYZ& b) {
    constexpr double tolerance = 1.0e-7;
    return std::abs(a.y - b.y) <= tolerance && std::abs(a.z - b.z) <= tolerance;
}

std::vector<PlateSegment> orderPlatesFromFreeEnd(const std::vector<PlateSegment>& plates) {
    if (plates.empty()) {
        return {};
    }

    std::vector<int> degree(2 * plates.size(), 0);
    for (std::size_t i = 0; i < plates.size(); ++i) {
        for (std::size_t j = 0; j < plates.size(); ++j) {
            if (i == j) {
                continue;
            }
            if (samePoint(plates[i].start, plates[j].start) || samePoint(plates[i].start, plates[j].end)) {
                ++degree[2 * i];
            }
            if (samePoint(plates[i].end, plates[j].start) || samePoint(plates[i].end, plates[j].end)) {
                ++degree[2 * i + 1];
            }
        }
    }

    std::size_t startIndex = 0;
    bool reverseStart = false;
    for (std::size_t i = 0; i < plates.size(); ++i) {
        if (degree[2 * i] == 0) {
            startIndex = i;
            reverseStart = false;
            break;
        }
        if (degree[2 * i + 1] == 0) {
            startIndex = i;
            reverseStart = true;
            break;
        }
    }

    std::vector<bool> used(plates.size(), false);
    std::vector<PlateSegment> ordered;
    ordered.reserve(plates.size());
    auto appendPlate = [&](std::size_t index, bool reverse) {
        PlateSegment plate = plates[index];
        if (reverse) {
            std::swap(plate.start, plate.end);
        }
        used[index] = true;
        ordered.push_back(plate);
    };

    appendPlate(startIndex, reverseStart);
    PointYZ current = ordered.back().end;
    while (ordered.size() < plates.size()) {
        bool found = false;
        for (std::size_t i = 0; i < plates.size(); ++i) {
            if (used[i]) {
                continue;
            }
            if (samePoint(plates[i].start, current)) {
                appendPlate(i, false);
                current = ordered.back().end;
                found = true;
                break;
            }
            if (samePoint(plates[i].end, current)) {
                appendPlate(i, true);
                current = ordered.back().end;
                found = true;
                break;
            }
        }
        if (!found) {
            for (std::size_t i = 0; i < plates.size(); ++i) {
                if (!used[i]) {
                    appendPlate(i, false);
                    current = ordered.back().end;
                    break;
                }
            }
        }
    }
    return ordered;
}

struct WarpingIntegrals {
    double omegaT = 0.0;
    double omega2T = 0.0;
    double lengthT = 0.0;
    double omegaY = 0.0;
    double omegaZ = 0.0;
};

WarpingIntegrals integrateSectorial(const std::vector<PlateSegment>& orderedPlates, PointYZ pole) {
    constexpr std::array<double, 4> xi = {
        -0.8611363115940526,
        -0.3399810435848563,
         0.3399810435848563,
         0.8611363115940526,
    };
    constexpr std::array<double, 4> wi = {
        0.3478548451374538,
        0.6521451548625461,
        0.6521451548625461,
        0.3478548451374538,
    };

    WarpingIntegrals out;
    double omegaAtStart = 0.0;
    for (const auto& plate : orderedPlates) {
        const double dy = plate.end.y - plate.start.y;
        const double dz = plate.end.z - plate.start.z;
        const double length = std::hypot(dy, dz);
        if (length <= 0.0 || plate.thickness <= 0.0) {
            continue;
        }
        const double dyds = dy / length;
        const double dzds = dz / length;
        const double rho0 = (plate.start.y - pole.y) * dzds - (plate.start.z - pole.z) * dyds;
        for (std::size_t g = 0; g < xi.size(); ++g) {
            const double s = 0.5 * length * (xi[g] + 1.0);
            const double weight = 0.5 * length * wi[g];
            const double y = plate.start.y + dyds * s;
            const double z = plate.start.z + dzds * s;
            const double rho = (y - pole.y) * dzds - (z - pole.z) * dyds;
            const double omega = omegaAtStart + 0.5 * (rho0 + rho) * s;
            const double tw = plate.thickness * weight;
            out.omegaT += omega * tw;
            out.omega2T += omega * omega * tw;
            out.lengthT += tw;
            out.omegaY += omega * y * tw;
            out.omegaZ += omega * z * tw;
        }
        const double rhoEnd = (plate.end.y - pole.y) * dzds - (plate.end.z - pole.z) * dyds;
        omegaAtStart += 0.5 * (rho0 + rhoEnd) * length;
    }
    return out;
}

void computeOpenThinWalledWarping(SectionProperties& p, const SectionModel& model) {
    const auto ordered = orderPlatesFromFreeEnd(model.plates);
    if (ordered.empty() || p.Jy <= 0.0 || p.Jz <= 0.0) {
        p.shearCenterY = p.cy;
        p.shearCenterZ = p.cz;
        p.warpingConstant = 0.0;
        return;
    }

    const PointYZ centroid{p.cy, p.cz};
    const auto initial = integrateSectorial(ordered, centroid);
    p.shearCenterY = p.cy + initial.omegaZ / p.Jy;
    p.shearCenterZ = p.cz - initial.omegaY / p.Jz;

    const auto shifted = integrateSectorial(ordered, {p.shearCenterY, p.shearCenterZ});
    if (shifted.lengthT <= 0.0) {
        p.warpingConstant = 0.0;
        return;
    }
    const double omegaMean = shifted.omegaT / shifted.lengthT;
    p.warpingConstant = std::max(0.0, shifted.omega2T - 2.0 * omegaMean * shifted.omegaT + omegaMean * omegaMean * shifted.lengthT);
}

}  // namespace

CalculationResult SectionCalculator::calculate(const SectionModel& model) {
    CalculationResult result;
    if (model.hasOverrideProperties) {
        result.properties = model.overrideProperties;
        computeWarpingAndShearCenter(result.properties, model);
        computePrincipalProperties(result.properties);
    } else {
        switch (model.type) {
            case SectionType::HSection:
                result.properties = calculateH(model);
                break;
            case SectionType::BoxSection:
                result.properties = calculateBox(model);
                break;
            case SectionType::PipeSection:
                result.properties = calculatePipe(model);
                break;
            case SectionType::QuaysideCraneGirder:
            case SectionType::CanvasThinWalled:
                result.properties = calculateCompositeRectangles(model);
                break;
        }
        computePrincipalProperties(result.properties);
    }
    for (const auto& warning : model.warnings) {
        result.diagnostics.push_back({3001, ErrorSeverity::Warning, "section", warning, "Confirm source geometry and reference formulas."});
    }
    result.stressPoints = StressPointEngine::generateDefault(model, result.properties);
    return result;
}

SectionProperties SectionCalculator::calculateH(const SectionModel& model) {
    SectionProperties p = calculateCompositeRectangles(model);
    const double A = model.parameters.get("A");
    const double H = model.parameters.get("H");
    const double e = model.parameters.get("e");
    const double f = model.parameters.get("f");
    p.Jx = 2.0 * A * e * e * e / 3.0 + H * f * f * f / 3.0;
    p.Az = f * H;
    p.Ay = 2.0 * A * e;
    computePrincipalProperties(p);
    computeWarpingAndShearCenter(p, model);
    return p;
}

SectionProperties SectionCalculator::calculateBox(const SectionModel& model) {
    SectionProperties p = calculateCompositeRectangles(model);
    const double A = model.parameters.get("A");
    const double B = model.parameters.get("B");
    const double H = model.parameters.get("H");
    const double D = model.parameters.get("D");
    const double E = model.parameters.get("E");
    const double H1 = model.parameters.get("H1");
    const double D1 = model.parameters.get("D1");
    const double E1 = model.parameters.get("E1");
    const double medianWidth = B + E;
    const double medianHeight = H + 0.5 * (D + D1);
    const double enclosedArea = medianWidth * medianHeight;
    const double torsionDenominator = medianWidth / D + medianWidth / D1 + medianHeight / E + medianHeight / E1;
    p.Jx = torsionDenominator > 0.0 ? 4.0 * enclosedArea * enclosedArea / torsionDenominator : 0.0;
    p.Az = 2.0 * (E * (H - H1) + E1 * H1);
    p.Ay = B * (D + D1);
    computePrincipalProperties(p);
    computeWarpingAndShearCenter(p, model);
    return p;
}

SectionProperties SectionCalculator::calculatePipe(const SectionModel& model) {
    SectionProperties p;
    const double Do = model.parameters.get("Do");
    const double t = model.parameters.get("t");
    const double Di = Do - 2.0 * t;
    p.area = kPi * 0.25 * (Do * Do - Di * Di);
    p.Jz = kPi / 64.0 * (std::pow(Do, 4.0) - std::pow(Di, 4.0));
    p.Jy = p.Jz;
    p.Jyz = 0.0;
    p.Jx = p.Jy + p.Jz;
    // The supplied workbook uses a shear-area value slightly above exact half
    // area for pipe sections. Keep this factor isolated so it can be replaced
    // if the client later provides the originating shear formula.
    constexpr double kPipeWorkbookShearFactor = 0.5000395029490837;
    p.Az = kPipeWorkbookShearFactor * p.area;
    p.Ay = kPipeWorkbookShearFactor * p.area;
    p.cy = 0.0;
    p.cz = 0.0;
    computePrincipalProperties(p);
    computeWarpingAndShearCenter(p, model);
    return p;
}

SectionProperties SectionCalculator::calculateCompositeRectangles(const SectionModel& model) {
    SectionProperties p;
    for (const auto& input : model.rectangles) {
        RectComponent rect = input;
        normalizeRect(rect);
        const double area = rectArea(rect);
        if (area <= 0.0) {
            continue;
        }
        const double yc = 0.5 * (rect.y1 + rect.y2);
        const double zc = 0.5 * (rect.z1 + rect.z2);
        p.area += area;
        p.cy += area * yc;
        p.cz += area * zc;
    }
    if (p.area <= 0.0) {
        return p;
    }
    p.cy /= p.area;
    p.cz /= p.area;

    for (const auto& input : model.rectangles) {
        RectComponent rect = input;
        normalizeRect(rect);
        const double widthY = rect.y2 - rect.y1;
        const double heightZ = rect.z2 - rect.z1;
        const double area = widthY * heightZ;
        if (area <= 0.0) {
            continue;
        }
        const double yc = 0.5 * (rect.y1 + rect.y2);
        const double zc = 0.5 * (rect.z1 + rect.z2);
        p.Jz += heightZ * widthY * widthY * widthY / 12.0 + area * sqr(yc - p.cy);
        p.Jy += widthY * heightZ * heightZ * heightZ / 12.0 + area * sqr(zc - p.cz);
        p.Jyz += area * (yc - p.cy) * (zc - p.cz);
    }

    double openPlateTorsion = 0.0;
    for (const auto& plate : model.plates) {
        openPlateTorsion += distance(plate.start, plate.end) * std::pow(plate.thickness, 3.0) / 3.0;
    }
    p.Jx = openPlateTorsion;
    p.Az = p.area;
    p.Ay = p.area;
    computePrincipalProperties(p);
    computeWarpingAndShearCenter(p, model);
    return p;
}

void SectionCalculator::computePrincipalProperties(SectionProperties& properties) {
    const double avg = 0.5 * (properties.Jy + properties.Jz);
    const double diff = 0.5 * (properties.Jy - properties.Jz);
    const double radius = std::sqrt(diff * diff + properties.Jyz * properties.Jyz);
    properties.Jzo = avg - radius;
    properties.Jyo = avg + radius;
    if (std::abs(properties.Jyz) < 1.0e-12 && std::abs(properties.Jy - properties.Jz) < 1.0e-12) {
        properties.theta = 0.0;
    } else {
        properties.theta = 0.5 * std::atan2(-2.0 * properties.Jyz, properties.Jy - properties.Jz);
    }
}

void SectionCalculator::computeWarpingAndShearCenter(SectionProperties& p, const SectionModel& model) {
    switch (model.type) {
        case SectionType::HSection: {
            const double A = model.parameters.get("A");
            const double H = model.parameters.get("H");
            const double e = model.parameters.get("e");
            p.shearCenterY = p.cy;
            p.shearCenterZ = p.cz;
            p.warpingConstant = (e * A * A * A * sqr(H - e)) / 24.0;
            break;
        }
        case SectionType::BoxSection:
        case SectionType::PipeSection:
            p.shearCenterY = p.cy;
            p.shearCenterZ = p.cz;
            p.warpingConstant = 0.0;
            break;
        case SectionType::QuaysideCraneGirder:
        case SectionType::CanvasThinWalled:
            computeOpenThinWalledWarping(p, model);
            break;
    }
}

}  // namespace spt
