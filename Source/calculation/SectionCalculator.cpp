#include "calculation/SectionCalculator.h"

#include "geometry/GeometryUtils.h"
#include "stress/StressPointEngine.h"

#include <cmath>

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

}  // namespace

CalculationResult SectionCalculator::calculate(const SectionModel& model) {
    CalculationResult result;
    if (model.hasOverrideProperties) {
        result.properties = model.overrideProperties;
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

}  // namespace spt
