#pragma once

#include "common/DataModel.h"

#include <vector>

namespace spt {

class StressPointEngine {
public:
    static std::vector<StressPoint> generateDefault(const SectionModel& model, const SectionProperties& properties);
    static PointYZ toPrincipal(PointYZ global, const SectionProperties& properties);
    static PointYZ fromPrincipal(PointYZ principal, const SectionProperties& properties);
    static StressPointValidity validate(PointYZ global, const SectionModel& model, double tolerance = 1.0e-6);
};

}  // namespace spt

