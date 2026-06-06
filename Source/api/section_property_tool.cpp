#include "section_property_tool.h"

#include "calculation/SectionCalculator.h"
#include "geometry/SectionBuilder.h"
#include "import_export/Exporter.h"
#include "mesh/MeshEngine.h"
#include "stress/StressPointEngine.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

struct SptSection_T {
    spt::SectionModel model;
};

struct SptResult_T {
    spt::CalculationResult result;
};

struct SptMesh_T {
    spt::MeshModel mesh;
};

namespace {

thread_local SptErrorInfo g_lastError = {};

void copyText(char* out, size_t outSize, const std::string& value) {
    if (outSize == 0) {
        return;
    }
    std::strncpy(out, value.c_str(), outSize - 1);
    out[outSize - 1] = '\0';
}

void clearError() {
    g_lastError = {};
}

int fail(int code, const std::string& field, const std::string& message, const std::string& remediation) {
    g_lastError.code = code;
    g_lastError.severity = 2;
    copyText(g_lastError.field, sizeof(g_lastError.field), field);
    copyText(g_lastError.message, sizeof(g_lastError.message), message);
    copyText(g_lastError.remediation, sizeof(g_lastError.remediation), remediation);
    return code;
}

int failFromDiagnostic(const spt::ErrorInfo& error) {
    g_lastError.code = error.code;
    g_lastError.severity = static_cast<int>(error.severity);
    copyText(g_lastError.field, sizeof(g_lastError.field), error.field);
    copyText(g_lastError.message, sizeof(g_lastError.message), error.message);
    copyText(g_lastError.remediation, sizeof(g_lastError.remediation), error.remediation);
    return error.code == 0 ? 1 : error.code;
}

spt::SectionType toCoreType(SptSectionType type) {
    switch (type) {
        case SPT_H_SECTION:
            return spt::SectionType::HSection;
        case SPT_BOX_SECTION:
            return spt::SectionType::BoxSection;
        case SPT_PIPE_SECTION:
            return spt::SectionType::PipeSection;
        case SPT_CRANE_GIRDER:
            return spt::SectionType::QuaysideCraneGirder;
        case SPT_CANVAS:
            return spt::SectionType::CanvasThinWalled;
    }
    return spt::SectionType::HSection;
}

spt::ExportFormat toCoreFormat(SptExportFormat format) {
    switch (format) {
        case SPT_EXPORT_CSV:
            return spt::ExportFormat::Csv;
        case SPT_EXPORT_JSON:
            return spt::ExportFormat::Json;
        case SPT_EXPORT_ANSYS:
            return spt::ExportFormat::Ansys;
        case SPT_EXPORT_ABAQUS:
            return spt::ExportFormat::Abaqus;
        case SPT_EXPORT_MIDAS_CIVIL:
            return spt::ExportFormat::MidasCivil;
    }
    return spt::ExportFormat::Csv;
}

spt::MeshSettings toCoreSettings(const SptMeshSettings* settings) {
    spt::MeshSettings core;
    if (settings) {
        core.targetSize = settings->target_size;
        core.refinementFactor = settings->refinement_factor > 0.0 ? settings->refinement_factor : 1.0;
        core.curvedSegmentCount = settings->curved_segment_count > 0 ? settings->curved_segment_count : 96;
        core.autoUpdate = settings->auto_update != 0;
    }
    return core;
}

void copyProperties(const spt::SectionProperties& source, SptSectionProperties* out) {
    out->area = source.area;
    out->Jz = source.Jz;
    out->Jy = source.Jy;
    out->Jyz = source.Jyz;
    out->Jzo = source.Jzo;
    out->Jyo = source.Jyo;
    out->Jx = source.Jx;
    out->Az = source.Az;
    out->Ay = source.Ay;
    out->cy = source.cy;
    out->cz = source.cz;
    out->theta = source.theta;
    out->warping_constant = source.warpingConstant;
    out->shear_center_y = source.shearCenterY;
    out->shear_center_z = source.shearCenterZ;
}

SptStressPoint toApiStressPoint(const spt::StressPoint& source) {
    SptStressPoint out = {};
    out.id = source.id;
    copyText(out.label, sizeof(out.label), source.label);
    out.y = source.global.y;
    out.z = source.global.z;
    out.y0 = source.principal.y;
    out.z0 = source.principal.z;
    out.source = static_cast<int>(source.source);
    out.validity = static_cast<int>(source.validity);
    return out;
}

int allocateStressArray(const std::vector<spt::StressPoint>& points, SptStressPointArray* out) {
    if (!out) {
        return fail(9001, "out", "Output stress point array pointer is null.", "Pass a valid SptStressPointArray pointer.");
    }
    out->points = nullptr;
    out->count = 0;
    if (points.empty()) {
        return 0;
    }
    auto* raw = static_cast<SptStressPoint*>(std::calloc(points.size(), sizeof(SptStressPoint)));
    if (!raw) {
        return fail(9002, "memory", "Unable to allocate stress point array.", "Free memory and retry.");
    }
    for (std::size_t i = 0; i < points.size(); ++i) {
        raw[i] = toApiStressPoint(points[i]);
    }
    out->points = raw;
    out->count = points.size();
    return 0;
}

}  // namespace

int spt_get_version(int* major, int* minor, int* patch) {
    if (major) {
        *major = 0;
    }
    if (minor) {
        *minor = 1;
    }
    if (patch) {
        *patch = 0;
    }
    return 0;
}

int spt_create_section_from_parameters(const SptSectionParameters* params, SptSectionHandle* out) {
    clearError();
    if (!params || !out) {
        return fail(9003, "params", "Input parameters or output handle pointer is null.", "Pass valid pointers.");
    }
    spt::SectionParameters core;
    core.type = toCoreType(params->type);
    for (std::size_t i = 0; i < params->parameter_count; ++i) {
        if (!params->parameters[i].name) {
            return fail(9004, "parameter", "Parameter name is null.", "Pass a valid UTF-8 parameter name.");
        }
        core.values[params->parameters[i].name] = params->parameters[i].value;
    }
    auto built = spt::SectionBuilder::build(core);
    if (!built.ok()) {
        return failFromDiagnostic(built.diagnostics.front());
    }
    auto handle = std::make_unique<SptSection_T>();
    handle->model = std::move(built.model);
    *out = handle.release();
    return 0;
}

int spt_create_section_from_canvas_lines(const SptPlateSegment* lines, size_t count, SptSectionHandle* out) {
    clearError();
    if (!out) {
        return fail(9005, "out", "Output handle pointer is null.", "Pass a valid output pointer.");
    }
    if (count > 0 && !lines) {
        return fail(9014, "lines", "Canvas line array is null while count is nonzero.", "Pass a valid SptPlateSegment array.");
    }
    std::vector<spt::PlateSegment> coreLines;
    for (std::size_t i = 0; i < count; ++i) {
        spt::PlateSegment line;
        line.start = {lines[i].start.y, lines[i].start.z};
        line.end = {lines[i].end.y, lines[i].end.z};
        line.thickness = lines[i].thickness;
        line.materialId = lines[i].material_id;
        line.id = lines[i].id ? lines[i].id : "";
        coreLines.push_back(line);
    }
    auto built = spt::SectionBuilder::buildFromCanvasLines(coreLines);
    if (!built.ok()) {
        return failFromDiagnostic(built.diagnostics.front());
    }
    auto handle = std::make_unique<SptSection_T>();
    handle->model = std::move(built.model);
    *out = handle.release();
    return 0;
}

int spt_calculate_section_properties(SptSectionHandle section, SptResultHandle* out) {
    clearError();
    if (!section || !out) {
        return fail(9006, "section", "Section handle or output result pointer is null.", "Create a section first.");
    }
    auto result = std::make_unique<SptResult_T>();
    result->result = spt::SectionCalculator::calculate(section->model);
    *out = result.release();
    return 0;
}

int spt_get_result_properties(SptResultHandle result, SptSectionProperties* out) {
    clearError();
    if (!result || !out) {
        return fail(9007, "result", "Result handle or output properties pointer is null.", "Calculate properties first.");
    }
    copyProperties(result->result.properties, out);
    return 0;
}

int spt_get_result_stress_points(SptResultHandle result, SptStressPointArray* out) {
    clearError();
    if (!result) {
        return fail(9008, "result", "Result handle is null.", "Calculate properties first.");
    }
    return allocateStressArray(result->result.stressPoints, out);
}

int spt_generate_default_stress_points(SptSectionHandle section, SptStressPointArray* out) {
    clearError();
    if (!section) {
        return fail(9009, "section", "Section handle is null.", "Create a section first.");
    }
    const auto result = spt::SectionCalculator::calculate(section->model);
    return allocateStressArray(result.stressPoints, out);
}

int spt_update_stress_points(SptSectionHandle section, const SptStressPoint* points, size_t count) {
    clearError();
    if (!section) {
        return fail(9010, "section", "Section handle is null.", "Create a section first.");
    }
    if (count > 0 && !points) {
        return fail(9015, "points", "Stress point array is null while count is nonzero.", "Pass a valid SptStressPoint array.");
    }
    const auto current = spt::SectionCalculator::calculate(section->model);
    section->model.overrideStressPoints.clear();
    for (std::size_t i = 0; i < count; ++i) {
        spt::StressPoint point;
        point.id = points[i].id;
        point.label = points[i].label;
        point.global = {points[i].y, points[i].z};
        point.principal = spt::StressPointEngine::toPrincipal(point.global, current.properties);
        point.source = spt::StressPointSource::User;
        point.validity = spt::StressPointEngine::validate(point.global, section->model);
        section->model.overrideStressPoints.push_back(point);
    }
    return 0;
}

int spt_create_mesh(SptSectionHandle section, const SptMeshSettings* settings, SptMeshHandle* out) {
    clearError();
    if (!section || !out) {
        return fail(9011, "mesh", "Section handle or mesh output pointer is null.", "Create a section first.");
    }
    auto mesh = std::make_unique<SptMesh_T>();
    mesh->mesh = spt::MeshEngine::generate(section->model, toCoreSettings(settings));
    *out = mesh.release();
    return 0;
}

int spt_get_mesh_counts(SptMeshHandle mesh, SptMeshCounts* out) {
    clearError();
    if (!mesh || !out) {
        return fail(9012, "mesh", "Mesh handle or output counts pointer is null.", "Create a mesh first.");
    }
    out->node_count = mesh->mesh.nodes.size();
    out->triangle_count = mesh->mesh.triangles.size();
    out->boundary_edge_count = mesh->mesh.boundaryEdges.size();
    return 0;
}

int spt_export_results(SptResultHandle result, const char* path_utf8, SptExportFormat format) {
    clearError();
    if (!result || !path_utf8) {
        return fail(9013, "export", "Result handle or path is null.", "Calculate properties and pass a valid path.");
    }
    spt::ErrorInfo error;
    if (!spt::Exporter::exportResult(result->result, path_utf8, toCoreFormat(format), &error)) {
        return failFromDiagnostic(error);
    }
    return 0;
}

void spt_free_stress_point_array(SptStressPointArray* array) {
    if (!array) {
        return;
    }
    std::free(array->points);
    array->points = nullptr;
    array->count = 0;
}

void spt_destroy_section(SptSectionHandle section) {
    delete section;
}

void spt_destroy_result(SptResultHandle result) {
    delete result;
}

void spt_destroy_mesh(SptMeshHandle mesh) {
    delete mesh;
}

const SptErrorInfo* spt_get_last_error(void) {
    return &g_lastError;
}
