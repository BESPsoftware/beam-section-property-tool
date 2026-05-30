#pragma once

#include "common/DataModel.h"

#include <vector>

namespace spt {

struct BuildResult {
    SectionModel model;
    std::vector<ErrorInfo> diagnostics;

    bool ok() const {
        for (const auto& diagnostic : diagnostics) {
            if (diagnostic.severity == ErrorSeverity::Error) {
                return false;
            }
        }
        return true;
    }
};

class SectionBuilder {
public:
    static BuildResult build(const SectionParameters& parameters);
    static BuildResult buildFromCanvasLines(const std::vector<PlateSegment>& lines);

private:
    static BuildResult buildH(const SectionParameters& parameters);
    static BuildResult buildBox(const SectionParameters& parameters);
    static BuildResult buildPipe(const SectionParameters& parameters);
    static BuildResult buildCrane(const SectionParameters& parameters);
};

}  // namespace spt

