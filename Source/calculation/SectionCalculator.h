#pragma once

#include "common/DataModel.h"

namespace spt {

class SectionCalculator {
public:
    static CalculationResult calculate(const SectionModel& model);

private:
    static SectionProperties calculateH(const SectionModel& model);
    static SectionProperties calculateBox(const SectionModel& model);
    static SectionProperties calculatePipe(const SectionModel& model);
    static SectionProperties calculateCompositeRectangles(const SectionModel& model);
    static void computePrincipalProperties(SectionProperties& properties);
    static void computeWarpingAndShearCenter(SectionProperties& properties, const SectionModel& model);
};

}  // namespace spt

