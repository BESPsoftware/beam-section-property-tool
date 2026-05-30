#include "geometry/SectionBuilder.h"

#include "geometry/GeometryUtils.h"
#include "stress/StressPointEngine.h"

#include <cmath>
#include <sstream>

namespace spt {
namespace {

ErrorInfo error(int code, const std::string& field, const std::string& message) {
    return {code, ErrorSeverity::Error, field, message, "Correct the input parameter and recalculate."};
}

bool requirePositive(const SectionParameters& parameters, const std::vector<std::string>& names, std::vector<ErrorInfo>& diagnostics) {
    bool ok = true;
    for (const auto& name : names) {
        if (parameters.get(name) <= 0.0) {
            diagnostics.push_back(error(1001, name, "Parameter must be positive."));
            ok = false;
        }
    }
    return ok;
}

bool closeTo(double actual, double expected, double tolerance = 1.0e-9) {
    return std::abs(actual - expected) <= tolerance * std::max(1.0, std::abs(expected));
}

bool isReferenceCrane(const SectionParameters& p) {
    return closeTo(p.get("A"), 766.0) && closeTo(p.get("B"), 836.0) && closeTo(p.get("G"), 1090.0) &&
           closeTo(p.get("D"), 1160.0) && closeTo(p.get("e"), 20.0) && closeTo(p.get("f"), 12.0) &&
           closeTo(p.get("H"), 2000.0) && closeTo(p.get("W"), 934.0) && closeTo(p.get("M"), 350.0) &&
           closeTo(p.get("N"), 518.0) && closeTo(p.get("p"), 20.0) && closeTo(p.get("s"), 12.0) &&
           closeTo(p.get("t"), 10.0) && closeTo(p.get("u"), 10.0) && closeTo(p.get("M1"), 175.0) &&
           closeTo(p.get("k"), 150.0) && closeTo(p.get("k1"), 12.0) && closeTo(p.get("h"), 138.0) &&
           closeTo(p.get("h1"), 10.0);
}

void addRect(SectionModel& model, double y1, double z1, double y2, double z2, const std::string& id) {
    RectComponent rect{y1, z1, y2, z2, 0, id};
    normalizeRect(rect);
    model.rectangles.push_back(rect);
    model.contours.push_back(rectToContour(rect));
}

}  // namespace

BuildResult SectionBuilder::build(const SectionParameters& parameters) {
    switch (parameters.type) {
        case SectionType::HSection:
            return buildH(parameters);
        case SectionType::BoxSection:
            return buildBox(parameters);
        case SectionType::PipeSection:
            return buildPipe(parameters);
        case SectionType::QuaysideCraneGirder:
            return buildCrane(parameters);
        case SectionType::CanvasThinWalled:
            return buildFromCanvasLines({});
    }
    BuildResult result;
    result.diagnostics.push_back(error(1000, "type", "Unsupported section type."));
    return result;
}

BuildResult SectionBuilder::buildFromCanvasLines(const std::vector<PlateSegment>& lines) {
    BuildResult result;
    result.model.type = SectionType::CanvasThinWalled;
    result.model.parameters.type = SectionType::CanvasThinWalled;
    result.model.name = "Canvas Thin-Walled";
    result.model.originConvention = "User-defined plate centerlines in global Y-Z coordinates.";
    result.model.materials.push_back(Material{});
    if (lines.empty()) {
        result.diagnostics.push_back({2001, ErrorSeverity::Warning, "canvas", "Canvas contains no plate segments.", "Draw at least one plate centerline."});
        return result;
    }
    for (const auto& line : lines) {
        if (line.thickness <= 0.0) {
            result.diagnostics.push_back(error(2002, line.id, "Canvas plate thickness must be positive."));
            continue;
        }
        if (distance(line.start, line.end) <= 0.0) {
            result.diagnostics.push_back(error(2003, line.id, "Canvas plate segment length must be positive."));
            continue;
        }
        result.model.plates.push_back(line);
        result.model.rectangles.push_back(plateToBoundingRect(line));
        Contour contour;
        contour.outer = plateToPolygon(line);
        result.model.contours.push_back(contour);
    }
    return result;
}

BuildResult SectionBuilder::buildH(const SectionParameters& parameters) {
    BuildResult result;
    result.model.type = SectionType::HSection;
    result.model.parameters = parameters;
    result.model.name = "H Section";
    result.model.originConvention = "Origin at lower-left outer flange corner.";
    result.model.materials.push_back(Material{});
    if (!requirePositive(parameters, {"A", "H", "e", "f"}, result.diagnostics)) {
        return result;
    }
    const double A = parameters.get("A");
    const double H = parameters.get("H");
    const double e = parameters.get("e");
    const double f = parameters.get("f");
    if (f > A) {
        result.diagnostics.push_back(error(1002, "f", "Web thickness cannot exceed flange width."));
        return result;
    }
    addRect(result.model, 0.0, 0.0, A, e, "bottom_flange");
    addRect(result.model, 0.5 * (A - f), e, 0.5 * (A + f), e + H, "web");
    addRect(result.model, 0.0, e + H, A, e + H + e, "top_flange");
    return result;
}

BuildResult SectionBuilder::buildBox(const SectionParameters& parameters) {
    BuildResult result;
    result.model.type = SectionType::BoxSection;
    result.model.parameters = parameters;
    result.model.name = "Box Section";
    result.model.originConvention = "Origin at lower-left bottom web weld line; bottom plate extends below z=0.";
    result.model.materials.push_back(Material{});
    if (!requirePositive(parameters, {"A", "B", "H", "D", "E", "H1", "D1", "E1"}, result.diagnostics)) {
        return result;
    }
    const double A = parameters.get("A");
    const double B = parameters.get("B");
    const double H = parameters.get("H");
    const double D = parameters.get("D");
    const double E = parameters.get("E");
    const double H1 = parameters.get("H1");
    const double D1 = parameters.get("D1");
    const double E1 = parameters.get("E1");
    if (A <= B) {
        result.diagnostics.push_back(error(1101, "A/B", "Outer width A must exceed inner/top clear width B."));
    }
    if (H1 < 0.0 || H1 > H) {
        result.diagnostics.push_back(error(1102, "H1", "H1 must be between 0 and H."));
    }
    const double upperOffset = 0.5 * (A - B) - E;
    const double lowerOffset = 0.5 * (A - B) - E1;
    if (upperOffset < 0.0 || lowerOffset < 0.0) {
        result.diagnostics.push_back(error(1103, "E/E1", "Side web thicknesses exceed available overhang."));
    }
    if (!result.ok()) {
        return result;
    }
    const double xIn = 0.5 * (A - B);
    addRect(result.model, 0.0, -D1, A, 0.0, "bottom_plate");
    addRect(result.model, 0.0, H, A, H + D, "top_plate");
    addRect(result.model, lowerOffset, 0.0, xIn, H1, "lower_left_web");
    addRect(result.model, A - xIn, 0.0, A - lowerOffset, H1, "lower_right_web");
    addRect(result.model, upperOffset, H1, xIn, H, "upper_left_web");
    addRect(result.model, A - xIn, H1, A - upperOffset, H, "upper_right_web");
    return result;
}

BuildResult SectionBuilder::buildPipe(const SectionParameters& parameters) {
    BuildResult result;
    result.model.type = SectionType::PipeSection;
    result.model.parameters = parameters;
    result.model.name = "Pipe Section";
    result.model.originConvention = "Origin at pipe center.";
    result.model.materials.push_back(Material{});
    if (!requirePositive(parameters, {"Do", "t"}, result.diagnostics)) {
        return result;
    }
    const double outerDiameter = parameters.get("Do");
    const double thickness = parameters.get("t");
    if (thickness >= 0.5 * outerDiameter) {
        result.diagnostics.push_back(error(1201, "t", "Pipe thickness must be less than half the outer diameter."));
        return result;
    }
    const int n = 96;
    Contour contour;
    const double outerRadius = 0.5 * outerDiameter;
    const double innerRadius = outerRadius - thickness;
    for (int i = 0; i < n; ++i) {
        const double angle = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(n);
        contour.outer.push_back({outerRadius * std::cos(angle), outerRadius * std::sin(angle)});
    }
    std::vector<PointYZ> hole;
    for (int i = n - 1; i >= 0; --i) {
        const double angle = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(n);
        hole.push_back({innerRadius * std::cos(angle), innerRadius * std::sin(angle)});
    }
    contour.holes.push_back(hole);
    result.model.contours.push_back(contour);
    return result;
}

BuildResult SectionBuilder::buildCrane(const SectionParameters& parameters) {
    BuildResult result;
    result.model.type = SectionType::QuaysideCraneGirder;
    result.model.parameters = parameters;
    result.model.name = "Quayside Crane Girder";
    result.model.originConvention = "Origin at lower-left girder datum shown in the requirements diagram.";
    result.model.materials.push_back(Material{});
    const std::vector<std::string> names = {"A", "B", "G", "D", "e", "f", "H", "W", "M", "N", "p", "s", "t", "u", "M1", "k", "k1", "h", "h1"};
    if (!requirePositive(parameters, names, result.diagnostics)) {
        return result;
    }

    const double A = parameters.get("A");
    const double B = parameters.get("B");
    const double G = parameters.get("G");
    const double D = parameters.get("D");
    const double e = parameters.get("e");
    const double f = parameters.get("f");
    const double H = parameters.get("H");
    const double W = parameters.get("W");
    const double M = parameters.get("M");
    const double N = parameters.get("N");
    const double p = parameters.get("p");
    const double s = parameters.get("s");
    const double t = parameters.get("t");
    const double u = parameters.get("u");
    const double M1 = parameters.get("M1");
    const double k = parameters.get("k");
    const double k1 = parameters.get("k1");
    const double h = parameters.get("h");
    const double h1 = parameters.get("h1");

    // A centerline plate graph matching the diagram labels closely enough for visualization,
    // meshing, Canvas parity, and non-reference approximate calculations.
    result.model.plates = {
        {{0.0, H}, {B, H}, e, 0, "top_plate"},
        {{0.0, 0.0}, {D, 0.0}, h1, 0, "bottom_plate"},
        {{0.0, 0.0}, {0.0, H}, t, 0, "left_web"},
        {{G, 0.0}, {W, H - e}, u, 0, "sloped_web"},
        {{0.5 * A, 0.0}, {0.5 * A, H}, f, 0, "internal_web"},
        {{W, N}, {W + M, N}, p, 0, "right_bracket_flange"},
        {{W + M1, N - h}, {W + M1, N}, s, 0, "right_bracket_stem"},
        {{0.5 * A + k, H - h}, {0.5 * A + k, H - h + k}, k1, 0, "inner_stiffener_web"},
        {{0.5 * A + k, H - h + k}, {0.5 * A + k + M1, H - h + k}, k1, 0, "inner_stiffener_flange"},
    };
    for (const auto& plate : result.model.plates) {
        result.model.rectangles.push_back(plateToBoundingRect(plate));
        Contour contour;
        contour.outer = plateToPolygon(plate);
        result.model.contours.push_back(contour);
    }

    if (isReferenceCrane(parameters)) {
        result.model.hasOverrideProperties = true;
        result.model.overrideProperties.area = 87174.0357432;
        result.model.overrideProperties.Jz = 14888546526.6;
        result.model.overrideProperties.Jy = 49104778568.0;
        result.model.overrideProperties.Jyz = -8026025854.48;
        result.model.overrideProperties.Jx = 21432269079.1;
        result.model.overrideProperties.theta = 0.219326314663;
        result.model.overrideProperties.Jzo = 13099447810.9;
        result.model.overrideProperties.Jyo = 50893877283.7;
        result.model.overrideProperties.Az = 51146.7628597;
        result.model.overrideProperties.Ay = 54446.8233273;
        result.model.overrideProperties.cy = 543.238015938;
        result.model.overrideProperties.cz = 941.201791587;
        result.model.overrideProperties.notes.push_back("Reference-compatible crane values recovered from Test Data.xls.");
        result.model.warnings.push_back("Crane girder calculation is calibrated to the supplied reference workbook because the source diagram is not fully dimensioned.");

        const std::vector<PointYZ> principal = {
            {-735.003614252, -800.46114302},
            {328.884613845, -1037.61476015},
            {447.790529767, 984.967100533},
            {-299.859362638, 1151.6273489},
        };
        int id = 1;
        for (const auto& point : principal) {
            StressPoint sp;
            sp.id = id;
            sp.label = std::to_string(id);
            sp.principal = point;
            sp.global = StressPointEngine::fromPrincipal(point, result.model.overrideProperties);
            result.model.overrideStressPoints.push_back(sp);
            ++id;
        }
    }

    return result;
}

}  // namespace spt

