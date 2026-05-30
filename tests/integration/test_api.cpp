#include "section_property_tool.h"

#include <cstdio>
#include <iostream>

namespace {

bool expectOk(int code, const char* context) {
    if (code == 0) {
        return true;
    }
    std::cerr << context << ": " << spt_get_last_error()->message << "\n";
    return false;
}

}  // namespace

bool runApiTests() {
    bool ok = true;
    int major = 0;
    int minor = 0;
    int patch = 0;
    ok &= expectOk(spt_get_version(&major, &minor, &patch), "version");
    ok &= (major == 0 && minor == 1);

    const SptParameter values[] = {
        {"A", 100.0},
        {"H", 210.0},
        {"e", 20.0},
        {"f", 12.0},
    };
    SptSectionParameters params{SPT_H_SECTION, values, 4};
    SptSectionHandle section = nullptr;
    ok &= expectOk(spt_create_section_from_parameters(&params, &section), "create section");

    SptResultHandle result = nullptr;
    ok &= expectOk(spt_calculate_section_properties(section, &result), "calculate");
    SptSectionProperties props{};
    ok &= expectOk(spt_get_result_properties(result, &props), "get props");
    ok &= (props.area > 6500.0 && props.area < 6600.0);

    SptStressPointArray stress{};
    ok &= expectOk(spt_get_result_stress_points(result, &stress), "get stress");
    ok &= (stress.count == 4);
    spt_free_stress_point_array(&stress);

    SptMeshSettings settings{0.0, 1.0, 96, 1};
    SptMeshHandle mesh = nullptr;
    ok &= expectOk(spt_create_mesh(section, &settings, &mesh), "mesh");
    SptMeshCounts counts{};
    ok &= expectOk(spt_get_mesh_counts(mesh, &counts), "mesh counts");
    ok &= (counts.node_count > 0 && counts.triangle_count > 0);

    ok &= expectOk(spt_export_results(result, "spt_test_export.csv", SPT_EXPORT_CSV), "export");
    std::remove("spt_test_export.csv");

    spt_destroy_mesh(mesh);
    spt_destroy_result(result);
    spt_destroy_section(section);
    return ok;
}

