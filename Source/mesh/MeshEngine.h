#pragma once

#include "common/DataModel.h"

namespace spt {

class MeshEngine {
public:
    static MeshModel generate(const SectionModel& model, const MeshSettings& settings);

private:
    static void appendRectMesh(const RectComponent& rect, const MeshSettings& settings, MeshModel& mesh);
    static void appendPipeMesh(const SectionModel& model, const MeshSettings& settings, MeshModel& mesh);
};

}  // namespace spt

