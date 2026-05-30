#include "mesh/MeshEngine.h"

#include "geometry/GeometryUtils.h"

#include <algorithm>
#include <cmath>

namespace spt {
namespace {

double meshTargetForRect(const RectComponent& rect, const MeshSettings& settings) {
    const double width = std::abs(rect.y2 - rect.y1);
    const double height = std::abs(rect.z2 - rect.z1);
    const double maxDim = std::max(width, height);
    if (settings.targetSize > 0.0) {
        return settings.targetSize;
    }
    const double refinement = std::max(0.1, settings.refinementFactor);
    return std::max(maxDim / (8.0 * refinement), 1.0);
}

int appendNode(MeshModel& mesh, PointYZ point) {
    mesh.nodes.push_back(point);
    return static_cast<int>(mesh.nodes.size()) - 1;
}

}  // namespace

MeshModel MeshEngine::generate(const SectionModel& model, const MeshSettings& settings) {
    MeshModel mesh;
    if (model.type == SectionType::PipeSection) {
        appendPipeMesh(model, settings, mesh);
        return mesh;
    }
    if (model.rectangles.empty()) {
        mesh.diagnostics.push_back({4001, ErrorSeverity::Warning, "mesh", "No meshable rectangles are available.", "Build a valid section first."});
        return mesh;
    }
    for (const auto& rect : model.rectangles) {
        appendRectMesh(rect, settings, mesh);
    }
    return mesh;
}

void MeshEngine::appendRectMesh(const RectComponent& input, const MeshSettings& settings, MeshModel& mesh) {
    RectComponent rect = input;
    normalizeRect(rect);
    const double width = rect.y2 - rect.y1;
    const double height = rect.z2 - rect.z1;
    if (width <= 0.0 || height <= 0.0) {
        return;
    }
    const double target = meshTargetForRect(rect, settings);
    const int ny = std::max(1, std::min(80, static_cast<int>(std::ceil(width / target))));
    const int nz = std::max(1, std::min(80, static_cast<int>(std::ceil(height / target))));
    const int base = static_cast<int>(mesh.nodes.size());
    for (int iz = 0; iz <= nz; ++iz) {
        const double z = rect.z1 + height * static_cast<double>(iz) / static_cast<double>(nz);
        for (int iy = 0; iy <= ny; ++iy) {
            const double y = rect.y1 + width * static_cast<double>(iy) / static_cast<double>(ny);
            mesh.nodes.push_back({y, z});
        }
    }
    const auto idx = [base, ny](int iy, int iz) { return base + iz * (ny + 1) + iy; };
    for (int iz = 0; iz < nz; ++iz) {
        for (int iy = 0; iy < ny; ++iy) {
            const int a = idx(iy, iz);
            const int b = idx(iy + 1, iz);
            const int c = idx(iy + 1, iz + 1);
            const int d = idx(iy, iz + 1);
            mesh.triangles.push_back({a, b, c, rect.materialId});
            mesh.triangles.push_back({a, c, d, rect.materialId});
        }
    }
    for (int iy = 0; iy < ny; ++iy) {
        mesh.boundaryEdges.push_back({idx(iy, 0), idx(iy + 1, 0)});
        mesh.boundaryEdges.push_back({idx(iy, nz), idx(iy + 1, nz)});
    }
    for (int iz = 0; iz < nz; ++iz) {
        mesh.boundaryEdges.push_back({idx(0, iz), idx(0, iz + 1)});
        mesh.boundaryEdges.push_back({idx(ny, iz), idx(ny, iz + 1)});
    }
}

void MeshEngine::appendPipeMesh(const SectionModel& model, const MeshSettings& settings, MeshModel& mesh) {
    const double outerRadius = 0.5 * model.parameters.get("Do");
    const double innerRadius = outerRadius - model.parameters.get("t");
    if (outerRadius <= 0.0 || innerRadius <= 0.0) {
        mesh.diagnostics.push_back({4002, ErrorSeverity::Error, "pipe", "Invalid pipe radii.", "Check Do and t."});
        return;
    }
    const int segments = std::max(16, std::min(360, static_cast<int>(settings.curvedSegmentCount * std::max(0.5, settings.refinementFactor))));
    std::vector<int> outer;
    std::vector<int> inner;
    for (int i = 0; i < segments; ++i) {
        const double angle = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(segments);
        outer.push_back(appendNode(mesh, {outerRadius * std::cos(angle), outerRadius * std::sin(angle)}));
        inner.push_back(appendNode(mesh, {innerRadius * std::cos(angle), innerRadius * std::sin(angle)}));
    }
    for (int i = 0; i < segments; ++i) {
        const int j = (i + 1) % segments;
        mesh.triangles.push_back({outer[i], outer[j], inner[j], 0});
        mesh.triangles.push_back({outer[i], inner[j], inner[i], 0});
        mesh.boundaryEdges.push_back({outer[i], outer[j]});
        mesh.boundaryEdges.push_back({inner[j], inner[i]});
    }
}

}  // namespace spt

