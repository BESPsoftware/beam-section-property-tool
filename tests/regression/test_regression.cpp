#include "calculation/SectionCalculator.h"
#include "geometry/SectionBuilder.h"

#include <cmath>
#include <iostream>
#include <map>
#include <vector>
#include <string>

namespace {

bool nearValue(const std::string& label, double actual, double expected, double tolerance) {
    if (std::abs(actual - expected) <= tolerance) {
        return true;
    }
    std::cerr << label << " expected " << expected << " actual " << actual << " tolerance " << tolerance << "\n";
    return false;
}

bool checkCondition(const std::string& label, bool condition) {
    if (condition) {
        return true;
    }
    std::cerr << label << " condition failed\n";
    return false;
}

bool checkRange(const std::string& label, double value, double lower, double upper) {
    if (value > lower && value < upper) {
        return true;
    }
    std::cerr << label << " expected in (" << lower << ", " << upper << ") actual " << value << "\n";
    return false;
}

double scaledTolerance(double expected, double absoluteMinimum) {
    return std::max(absoluteMinimum, std::abs(expected) * 1.0e-6);
}

spt::CalculationResult calculate(spt::SectionType type, std::map<std::string, double> values) {
    spt::SectionParameters parameters;
    parameters.type = type;
    parameters.values = std::move(values);
    auto built = spt::SectionBuilder::build(parameters);
    if (!built.ok()) {
        throw std::runtime_error("section build failed");
    }
    return spt::SectionCalculator::calculate(built.model);
}

spt::BuildResult build(spt::SectionType type, std::map<std::string, double> values) {
    spt::SectionParameters parameters;
    parameters.type = type;
    parameters.values = std::move(values);
    return spt::SectionBuilder::build(parameters);
}

bool checkCommon(const std::string& name, const spt::SectionProperties& p, double area, double jz, double jy, double jx, double az, double ay, double cy, double cz) {
    bool ok = true;
    ok &= nearValue(name + ".Area", p.area, area, scaledTolerance(area, 0.01));
    ok &= nearValue(name + ".Jz", p.Jz, jz, scaledTolerance(jz, 1.0));
    ok &= nearValue(name + ".Jy", p.Jy, jy, scaledTolerance(jy, 1.0));
    ok &= nearValue(name + ".Jx", p.Jx, jx, scaledTolerance(jx, 1.0));
    ok &= nearValue(name + ".Az", p.Az, az, scaledTolerance(az, 0.01));
    ok &= nearValue(name + ".Ay", p.Ay, ay, scaledTolerance(ay, 0.01));
    ok &= nearValue(name + ".cy", p.cy, cy, scaledTolerance(cy, 0.01));
    ok &= nearValue(name + ".cz", p.cz, cz, scaledTolerance(cz, 0.01));
    return ok;
}

bool checkStress(const std::string& name, const spt::CalculationResult& result, const double expected[4][2]) {
    bool ok = true;
    if (result.stressPoints.size() != 4) {
        std::cerr << name << " expected 4 stress points, actual " << result.stressPoints.size() << "\n";
        return false;
    }
    for (std::size_t i = 0; i < 4; ++i) {
        ok &= nearValue(name + ".stress.y0." + std::to_string(i + 1), result.stressPoints[i].principal.y, expected[i][0], scaledTolerance(expected[i][0], 0.01));
        ok &= nearValue(name + ".stress.z0." + std::to_string(i + 1), result.stressPoints[i].principal.z, expected[i][1], scaledTolerance(expected[i][1], 0.01));
    }
    return ok;
}

bool hasDiagnosticCode(const spt::BuildResult& result, int code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool runRegressionTests() {
    bool ok = true;

    const auto h = calculate(spt::SectionType::HSection, {{"A", 100.0}, {"H", 210.0}, {"e", 20.0}, {"f", 12.0}});
    ok &= checkCommon("H", h.properties, 6520.0, 3363573.33333, 62294333.3333, 654293.333333, 2520.0, 4000.0, 50.0, 125.0);
    ok &= nearValue("H.Cw", h.properties.warpingConstant, 30083333333.3333, scaledTolerance(30083333333.3333, 1.0));
    ok &= nearValue("H.ys", h.properties.shearCenterY, h.properties.cy, 1.0e-9);
    ok &= nearValue("H.zs", h.properties.shearCenterZ, h.properties.cz, 1.0e-9);
    const double hStress[4][2] = {{-6.0, -105.0}, {6.0, -105.0}, {6.0, 105.0}, {-6.0, 105.0}};
    ok &= checkStress("H", h, hStress);

    const auto box = calculate(spt::SectionType::BoxSection, {
        {"A", 1320.0}, {"B", 1250.0}, {"H", 2600.0}, {"D", 40.0},
        {"E", 16.0}, {"H1", 600.0}, {"D1", 22.0}, {"E1", 16.0}});
    ok &= checkCommon("Box", box.properties, 165040.0, 45222267733.3, 182728101833.0, 106149908037.0, 83200.0, 77500.0, 660.0, 1491.61754726);
    ok &= nearValue("Box.Cw", box.properties.warpingConstant, 0.0, 1.0e-9);
    ok &= nearValue("Box.ys", box.properties.shearCenterY, box.properties.cy, 1.0e-9);
    ok &= nearValue("Box.zs", box.properties.shearCenterZ, box.properties.cz, 1.0e-9);
    const double boxStress[4][2] = {
        {-641.0, -1491.61754726},
        {641.0, -1491.61754726},
        {641.0, 1108.38245274},
        {-641.0, 1108.38245274}};
    ok &= checkStress("Box", box, boxStress);

    const auto pipe = calculate(spt::SectionType::PipeSection, {{"Do", 1300.0}, {"t", 14.0}});
    ok &= checkCommon("Pipe", pipe.properties, 56561.2341352, 11693978596.2, 11693978596.2, 23387957192.4, 28282.8514351, 28282.8514351, 0.0, 0.0);
    ok &= nearValue("Pipe.Cw", pipe.properties.warpingConstant, 0.0, 1.0e-9);
    ok &= nearValue("Pipe.ys", pipe.properties.shearCenterY, pipe.properties.cy, 1.0e-9);
    ok &= nearValue("Pipe.zs", pipe.properties.shearCenterZ, pipe.properties.cz, 1.0e-9);
    const double pipeStress[4][2] = {{650.0, 0.0}, {0.0, 650.0}, {-650.0, 0.0}, {0.0, -650.0}};
    ok &= checkStress("Pipe", pipe, pipeStress);

    const auto crane = calculate(spt::SectionType::QuaysideCraneGirder, {
        {"A", 766.0}, {"B", 836.0}, {"G", 1090.0}, {"D", 1160.0}, {"e", 20.0},
        {"f", 12.0}, {"H", 2000.0}, {"W", 934.0}, {"M", 350.0}, {"N", 518.0},
        {"p", 20.0}, {"s", 12.0}, {"t", 10.0}, {"u", 10.0}, {"M1", 175.0},
        {"k", 150.0}, {"k1", 12.0}, {"h", 138.0}, {"h1", 10.0}});
    ok &= checkCommon("Crane", crane.properties, 87174.0357432, 14888546526.6, 49104778568.0, 21432269079.1, 51146.7628597, 54446.8233273, 543.238015938, 941.201791587);
    ok &= nearValue("Crane.Jyz", crane.properties.Jyz, -8026025854.48, scaledTolerance(-8026025854.48, 1.0));
    ok &= nearValue("Crane.theta", crane.properties.theta, 0.219326314663, 1.0e-8);
    ok &= nearValue("Crane.Jzo", crane.properties.Jzo, 13099447810.9, scaledTolerance(13099447810.9, 1.0));
    ok &= nearValue("Crane.Jyo", crane.properties.Jyo, 50893877283.7, scaledTolerance(50893877283.7, 1.0));
    ok &= checkRange("Crane.Cw magnitude", crane.properties.warpingConstant, 1.0e18, 1.0e20);
    ok &= checkCondition("Crane.ys nonzero offset", std::abs(crane.properties.shearCenterY - crane.properties.cy) > 1.0);
    ok &= checkCondition("Crane.zs nonzero offset", std::abs(crane.properties.shearCenterZ - crane.properties.cz) > 1.0);
    ok &= checkCondition("Crane.ys sign", crane.properties.shearCenterY > crane.properties.cy);
    ok &= checkCondition("Crane.zs sign", crane.properties.shearCenterZ < crane.properties.cz);
    const double craneStress[4][2] = {
        {-735.003614252, -800.46114302},
        {328.884613845, -1037.61476015},
        {447.790529767, 984.967100533},
        {-299.859362638, 1151.6273489}};
    ok &= checkStress("Crane", crane, craneStress);

    auto scaledCraneBuild = build(spt::SectionType::QuaysideCraneGirder, {
        {"A", 1532.0}, {"B", 1672.0}, {"G", 2180.0}, {"D", 2320.0}, {"e", 40.0},
        {"f", 24.0}, {"H", 4000.0}, {"W", 1868.0}, {"M", 700.0}, {"N", 1036.0},
        {"p", 40.0}, {"s", 24.0}, {"t", 20.0}, {"u", 20.0}, {"M1", 350.0},
        {"k", 300.0}, {"k1", 24.0}, {"h", 276.0}, {"h1", 20.0}});
    if (!scaledCraneBuild.ok()) {
        std::cerr << "Scaled crane build failed\n";
        return false;
    }
    const auto scaledCrane = spt::SectionCalculator::calculate(scaledCraneBuild.model);
    ok &= nearValue("CraneScaled.Area", scaledCrane.properties.area, 1654500.75860751, scaledTolerance(1654500.75860751, 0.01));
    ok &= nearValue("CraneScaled.Jz", scaledCrane.properties.Jz, 570344073018.856, scaledTolerance(570344073018.856, 1.0));
    ok &= nearValue("CraneScaled.Jy", scaledCrane.properties.Jy, 2501914645822.7, scaledTolerance(2501914645822.7, 1.0));
    ok &= nearValue("CraneScaled.Jyz", scaledCrane.properties.Jyz, -108933351564.659, scaledTolerance(-108933351564.659, 1.0));
    ok &= nearValue("CraneScaled.Jx", scaledCrane.properties.Jx, 100747733.050713, scaledTolerance(100747733.050713, 1.0));
    ok &= nearValue("CraneScaled.Az", scaledCrane.properties.Az, 1654500.75860751, scaledTolerance(1654500.75860751, 0.01));
    ok &= nearValue("CraneScaled.Ay", scaledCrane.properties.Ay, 1654500.75860751, scaledTolerance(1654500.75860751, 0.01));
    ok &= nearValue("CraneScaled.cy", scaledCrane.properties.cy, 1776.80229889757, scaledTolerance(1776.80229889757, 0.01));
    ok &= nearValue("CraneScaled.cz", scaledCrane.properties.cz, 2006.56561610585, scaledTolerance(2006.56561610585, 0.01));
    ok &= checkCondition("CraneScaled.diagnostics", scaledCrane.diagnostics.size() == 1);

    auto nonReferenceCraneBuild = build(spt::SectionType::QuaysideCraneGirder, {
        {"A", 800.0}, {"B", 860.0}, {"G", 1110.0}, {"D", 1180.0}, {"e", 22.0},
        {"f", 14.0}, {"H", 2050.0}, {"W", 950.0}, {"M", 360.0}, {"N", 530.0},
        {"p", 22.0}, {"s", 14.0}, {"t", 12.0}, {"u", 12.0}, {"M1", 180.0},
        {"k", 160.0}, {"k1", 14.0}, {"h", 145.0}, {"h1", 12.0}});
    if (!nonReferenceCraneBuild.ok()) {
        std::cerr << "Non-reference crane build failed\n";
        return false;
    }
    const auto nonReferenceCrane = spt::SectionCalculator::calculate(nonReferenceCraneBuild.model);
    ok &= checkCondition("NonReferenceCrane.Area positive", nonReferenceCrane.properties.area > 0.0);
    ok &= checkCondition("NonReferenceCrane.Jz positive", nonReferenceCrane.properties.Jz > 0.0);
    ok &= checkCondition("NonReferenceCrane.Jy positive", nonReferenceCrane.properties.Jy > 0.0);
    ok &= checkCondition("NonReferenceCrane.Jx positive", nonReferenceCrane.properties.Jx > 0.0);
    ok &= checkCondition("NonReferenceCrane.stress count", nonReferenceCrane.stressPoints.size() == 4);
    ok &= checkCondition("NonReferenceCrane.diagnostics", nonReferenceCrane.diagnostics.size() == 1);

    auto invalidCanvas = spt::SectionBuilder::buildFromCanvasLines({
        {{0.0, 0.0}, {100.0, 0.0}, -5.0, 0, "negative_t"},
        {{0.0, 0.0}, {0.0, 0.0}, 10.0, 0, "zero_length"},
        {{0.0, 0.0}, {100.0, 0.0}, 10.0, 0, "duplicate"},
        {{0.0, 10.0}, {100.0, 10.0}, 10.0, 0, "duplicate"},
    });
    ok &= !invalidCanvas.ok();
    ok &= hasDiagnosticCode(invalidCanvas, 2002);
    ok &= hasDiagnosticCode(invalidCanvas, 2003);
    ok &= hasDiagnosticCode(invalidCanvas, 2004);

    auto autoIdCanvas = spt::SectionBuilder::buildFromCanvasLines({
        {{0.0, 0.0}, {100.0, 0.0}, 10.0, 0, ""},
        {{0.0, 80.0}, {100.0, 80.0}, 10.0, 0, "plate_1"},
    });
    ok &= autoIdCanvas.ok();
    ok &= (autoIdCanvas.model.plates.size() == 2);
    ok &= (autoIdCanvas.model.plates[0].id == "plate_2");
    ok &= (autoIdCanvas.model.plates[1].id == "plate_1");

    auto canvasBuild = spt::SectionBuilder::buildFromCanvasLines({
        {{0.0, 0.0}, {100.0, 0.0}, 10.0, 0, "bottom"},
        {{0.0, 80.0}, {100.0, 80.0}, 10.0, 0, "top"},
        {{0.0, 0.0}, {0.0, 80.0}, 10.0, 0, "left"},
        {{100.0, 0.0}, {100.0, 80.0}, 10.0, 0, "right"},
    });
    if (!canvasBuild.ok()) {
        std::cerr << "Canvas build failed\n";
        return false;
    }
    const auto canvas = spt::SectionCalculator::calculate(canvasBuild.model);
    ok &= nearValue("Canvas.Area", canvas.properties.area, 3600.0, scaledTolerance(3600.0, 0.01));
    ok &= nearValue("Canvas.cy", canvas.properties.cy, 50.0, scaledTolerance(50.0, 0.01));
    ok &= nearValue("Canvas.cz", canvas.properties.cz, 40.0, scaledTolerance(40.0, 0.01));
    ok &= checkCondition("Canvas.Jz positive", canvas.properties.Jz > 0.0);
    ok &= checkCondition("Canvas.Jy positive", canvas.properties.Jy > 0.0);
    ok &= checkCondition("Canvas.Cw positive", canvas.properties.warpingConstant > 0.0);
    ok &= checkCondition("Canvas.ys offset", std::abs(canvas.properties.shearCenterY - canvas.properties.cy) > 1.0);
    ok &= checkCondition("Canvas.zs offset", std::abs(canvas.properties.shearCenterZ - canvas.properties.cz) > 1.0);
    ok &= checkCondition("Canvas.stress count", canvas.stressPoints.size() == 4);

    return ok;
}
