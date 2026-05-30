#pragma once

#include "common/DataModel.h"

#include <string>

namespace spt {

inline std::string toString(SectionType type) {
    switch (type) {
        case SectionType::HSection:
            return "H Section";
        case SectionType::BoxSection:
            return "Box Section";
        case SectionType::PipeSection:
            return "Pipe Section";
        case SectionType::QuaysideCraneGirder:
            return "Quayside Crane Girder";
        case SectionType::CanvasThinWalled:
            return "Canvas Thin-Walled";
    }
    return "Unknown";
}

inline std::string propertyUnit(const std::string& name) {
    if (name == "Area" || name == "Az" || name == "Ay" || name == "Az0" || name == "Ay0") {
        return "mm2";
    }
    if (name == "Jz" || name == "Jy" || name == "Jyz" || name == "Jzo" || name == "Jyo" || name == "Jx") {
        return "mm4";
    }
    if (name == "theta") {
        return "rad";
    }
    return "mm";
}

}  // namespace spt

