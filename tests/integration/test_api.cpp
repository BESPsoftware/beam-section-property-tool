#include "section_property_tool.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

bool expectOk(int code, const char* context) {
    if (code == 0) {
        return true;
    }
    std::cerr << context << ": " << spt_get_last_error()->message << "\n";
    return false;
}

bool fileContains(const std::filesystem::path& path, const std::string& needle) {
    std::ifstream in(path);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (text.find(needle) != std::string::npos) {
        return true;
    }
    std::cerr << path.string() << " does not contain: " << needle << "\n";
    return false;
}

bool fileNotContains(const std::filesystem::path& path, const std::string& needle) {
    std::ifstream in(path);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (text.find(needle) == std::string::npos) {
        return true;
    }
    std::cerr << path.string() << " should not contain: " << needle << "\n";
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
    ok &= (props.warping_constant > 0.0);
    ok &= (props.shear_center_y > 0.0);
    ok &= (props.shear_center_z > 0.0);

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

    ok &= expectOk(spt_export_results(result, "spt_test_export.csv", SPT_EXPORT_CSV), "export csv");
    ok &= fileContains("spt_test_export.csv", "Cw,");
    ok &= fileContains("spt_test_export.csv", "ys,");
    ok &= fileContains("spt_test_export.csv", "zs,");
    std::remove("spt_test_export.csv");

    // ANSYS: SECDATA field order A,Iy,Iz,Iyz,J,CGy,CGz,...
    // SECOFFSET,BSEC must be present to avoid double-counting CGy/CGz.
    ok &= expectOk(spt_export_results(result, "spt_test_export.mac", SPT_EXPORT_ANSYS), "export ansys");
    ok &= fileContains("spt_test_export.mac", "SECTYPE,1,BEAM,ASEC,SPT_SECTION");
    ok &= fileContains("spt_test_export.mac", "SECOFFSET,BSEC");
    ok &= fileContains("spt_test_export.mac", "SECDATA,6520.00");
    ok &= fileContains("spt_test_export.mac", "Warping constant Cw");
    std::filesystem::remove("spt_test_export.mac");

    // ABAQUS: correct field order (A, I11=Iy, I12=Iyz, I22=Iz, J) + *SECTION POINTS
    ok &= expectOk(spt_export_results(result, "spt_test_export.inp", SPT_EXPORT_ABAQUS), "export abaqus");
    ok &= fileContains("spt_test_export.inp", "*BEAM GENERAL SECTION, SECTION=GENERAL, ELSET=ALL_BEAMS");
    ok &= fileContains("spt_test_export.inp", "6520.00");
    ok &= fileContains("spt_test_export.inp", "Warping constant Cw");
    ok &= fileContains("spt_test_export.inp", "*SECTION POINTS");
    std::filesystem::remove("spt_test_export.inp");

    // Midas Civil MCT: *SECT keyword + DBUSER type + tabular data line
    ok &= expectOk(spt_export_results(result, "spt_test_export.mct", SPT_EXPORT_MIDAS_CIVIL), "export midas");
    ok &= fileContains("spt_test_export.mct", "*SECT");
    ok &= fileContains("spt_test_export.mct", "DBUSER");
    ok &= fileContains("spt_test_export.mct", "6520.00");
    ok &= fileContains("spt_test_export.mct", "Warping constant Cw");
    std::filesystem::remove("spt_test_export.mct");

    // Empty-stress-points path: a Canvas section with no plates produces a
    // result with zero stress points. ABAQUS must omit *SECTION POINTS;
    // ANSYS must omit the stress-recovery comment block.
    {
        SptSectionHandle emptySection = nullptr;
        spt_create_section_from_canvas_lines(nullptr, 0, &emptySection);
        SptResultHandle emptyResult = nullptr;
        if (emptySection) {
            spt_calculate_section_properties(emptySection, &emptyResult);
        }
        if (emptyResult) {
            ok &= expectOk(spt_export_results(emptyResult, "spt_test_nostress.inp", SPT_EXPORT_ABAQUS), "abaqus no-stress-points");
            ok &= fileNotContains("spt_test_nostress.inp", "*SECTION POINTS");
            std::filesystem::remove("spt_test_nostress.inp");

            ok &= expectOk(spt_export_results(emptyResult, "spt_test_nostress.mac", SPT_EXPORT_ANSYS), "ansys no-stress-points");
            ok &= fileNotContains("spt_test_nostress.mac", "Stress recovery points");
            std::filesystem::remove("spt_test_nostress.mac");

            spt_destroy_result(emptyResult);
        }
        if (emptySection) {
            spt_destroy_section(emptySection);
        }
    }

    const std::filesystem::path utf8ExportPath = u8"spt_test_export_测试.csv";
    ok &= expectOk(spt_export_results(result, utf8ExportPath.u8string().c_str(), SPT_EXPORT_CSV), "export utf8");
    std::filesystem::remove(utf8ExportPath);

    spt_destroy_mesh(mesh);
    spt_destroy_result(result);
    spt_destroy_section(section);
    return ok;
}

