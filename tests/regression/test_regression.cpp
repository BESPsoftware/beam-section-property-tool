#include "calculation/SectionCalculator.h"
#include "geometry/SectionBuilder.h"

#include <cmath>
#include <iostream>
#include <map>
#include <string>

namespace {

bool nearValue(const std::string& label, double actual, double expected, double tolerance) {
    if (std::abs(actual - expected) <= tolerance) {
        return true;
    }
    std::cerr << label << " expected " << expected << " actual " << actual << " tolerance " << tolerance << "\n";
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

}  // namespace

bool runRegressionTests() {
    bool ok = true;

    const auto h = calculate(spt::SectionType::HSection, {{"A", 100.0}, {"H", 210.0}, {"e", 20.0}, {"f", 12.0}});
    ok &= checkCommon("H", h.properties, 6520.0, 3363573.33333, 62294333.3333, 654293.333333, 2520.0, 4000.0, 50.0, 125.0);
    const double hStress[4][2] = {{-6.0, -105.0}, {6.0, -105.0}, {6.0, 105.0}, {-6.0, 105.0}};
    ok &= checkStress("H", h, hStress);

    const auto box = calculate(spt::SectionType::BoxSection, {
        {"A", 1320.0}, {"B", 1250.0}, {"H", 2600.0}, {"D", 40.0},
        {"E", 16.0}, {"H1", 600.0}, {"D1", 22.0}, {"E1", 16.0}});
    ok &= checkCommon("Box", box.properties, 165040.0, 45222267733.3, 182728101833.0, 106149908037.0, 83200.0, 77500.0, 660.0, 1491.61754726);
    const double boxStress[4][2] = {
        {-641.0, -1491.61754726},
        {641.0, -1491.61754726},
        {641.0, 1108.38245274},
        {-641.0, 1108.38245274}};
    ok &= checkStress("Box", box, boxStress);

    const auto pipe = calculate(spt::SectionType::PipeSection, {{"Do", 1300.0}, {"t", 14.0}});
    ok &= checkCommon("Pipe", pipe.properties, 56561.2341352, 11693978596.2, 11693978596.2, 23387957192.4, 28282.8514351, 28282.8514351, 0.0, 0.0);
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
    const double craneStress[4][2] = {
        {-735.003614252, -800.46114302},
        {328.884613845, -1037.61476015},
        {447.790529767, 984.967100533},
        {-299.859362638, 1151.6273489}};
    ok &= checkStress("Crane", crane, craneStress);

    return ok;
}

