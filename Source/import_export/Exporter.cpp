#include "import_export/Exporter.h"

#include "common/StringUtils.h"

#include <fstream>
#include <iomanip>

#if defined(_WIN32)
#  include <windows.h>
#endif

namespace spt {
namespace {

#if defined(_WIN32)
std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }
    int count = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (count <= 0) {
        return std::wstring();
    }
    std::wstring wide(count, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), count);
    if (!wide.empty() && wide.back() == L'\0') {
        wide.pop_back();
    }
    return wide;
}
#endif

std::ofstream openOutputFile(const std::string& path) {
#if defined(_WIN32)
    return std::ofstream(utf8ToWide(path));
#else
    return std::ofstream(path);
#endif
}

void setFileError(ErrorInfo* error, const std::string& path) {
    if (error) {
        *error = {5001, ErrorSeverity::Error, path, "Unable to write export file.", "Check path permissions and retry."};
    }
}

void writePropertyJson(std::ostream& os, const char* name, double value, bool comma = true) {
    os << "    \"" << name << "\": " << std::setprecision(15) << value;
    if (comma) {
        os << ",";
    }
    os << "\n";
}

}  // namespace

bool Exporter::exportResult(const CalculationResult& result, const std::string& path, ExportFormat format, ErrorInfo* error) {
    switch (format) {
        case ExportFormat::Csv:
            return exportCsv(result, path, error);
        case ExportFormat::Json:
            return exportJson(result, path, error);
        case ExportFormat::Ansys:
            return exportAnsys(result, path, error);
        case ExportFormat::Abaqus:
            return exportAbaqus(result, path, error);
        case ExportFormat::MidasCivil:
            return exportMidasCivil(result, path, error);
    }
    return false;
}

bool Exporter::saveProject(const SectionModel& model, const std::string& path, ErrorInfo* error) {
    std::ofstream os = openOutputFile(path);
    if (!os) {
        setFileError(error, path);
        return false;
    }
    os << "{\n";
    os << "  \"version\": 1,\n";
    os << "  \"sectionType\": \"" << toString(model.type) << "\",\n";
    os << "  \"parameters\": {\n";
    std::size_t index = 0;
    for (const auto& item : model.parameters.values) {
        os << "    \"" << item.first << "\": " << std::setprecision(15) << item.second;
        os << (++index == model.parameters.values.size() ? "\n" : ",\n");
    }
    os << "  },\n";
    os << "  \"plateSegments\": [\n";
    for (std::size_t i = 0; i < model.plates.size(); ++i) {
        const auto& p = model.plates[i];
        os << "    {\"id\":\"" << p.id << "\",\"y1\":" << p.start.y << ",\"z1\":" << p.start.z
           << ",\"y2\":" << p.end.y << ",\"z2\":" << p.end.z << ",\"thickness\":" << p.thickness << "}";
        os << (i + 1 == model.plates.size() ? "\n" : ",\n");
    }
    os << "  ]\n";
    os << "}\n";
    return true;
}

bool Exporter::exportCsv(const CalculationResult& result, const std::string& path, ErrorInfo* error) {
    std::ofstream os = openOutputFile(path);
    if (!os) {
        setFileError(error, path);
        return false;
    }
    const auto& p = result.properties;
    os << "property,value,unit\n";
    os << "Area," << std::setprecision(15) << p.area << ",mm2\n";
    os << "Jz," << p.Jz << ",mm4\n";
    os << "Jy," << p.Jy << ",mm4\n";
    os << "Jyz," << p.Jyz << ",mm4\n";
    os << "Jzo," << p.Jzo << ",mm4\n";
    os << "Jyo," << p.Jyo << ",mm4\n";
    os << "Jx," << p.Jx << ",mm4\n";
    os << "Az," << p.Az << ",mm2\n";
    os << "Ay," << p.Ay << ",mm2\n";
    os << "cy," << p.cy << ",mm\n";
    os << "cz," << p.cz << ",mm\n";
    os << "theta," << p.theta << ",rad\n";
    os << "\nstress_id,label,y,z,y0,z0,source,validity\n";
    for (const auto& sp : result.stressPoints) {
        os << sp.id << "," << sp.label << "," << sp.global.y << "," << sp.global.z << ","
           << sp.principal.y << "," << sp.principal.z << ","
           << (sp.source == StressPointSource::Default ? "Default" : "User") << ","
           << static_cast<int>(sp.validity) << "\n";
    }
    return true;
}

bool Exporter::exportJson(const CalculationResult& result, const std::string& path, ErrorInfo* error) {
    std::ofstream os = openOutputFile(path);
    if (!os) {
        setFileError(error, path);
        return false;
    }
    const auto& p = result.properties;
    os << "{\n";
    os << "  \"properties\": {\n";
    writePropertyJson(os, "Area", p.area);
    writePropertyJson(os, "Jz", p.Jz);
    writePropertyJson(os, "Jy", p.Jy);
    writePropertyJson(os, "Jyz", p.Jyz);
    writePropertyJson(os, "Jzo", p.Jzo);
    writePropertyJson(os, "Jyo", p.Jyo);
    writePropertyJson(os, "Jx", p.Jx);
    writePropertyJson(os, "Az", p.Az);
    writePropertyJson(os, "Ay", p.Ay);
    writePropertyJson(os, "cy", p.cy);
    writePropertyJson(os, "cz", p.cz);
    writePropertyJson(os, "theta", p.theta, false);
    os << "  },\n";
    os << "  \"stressPoints\": [\n";
    for (std::size_t i = 0; i < result.stressPoints.size(); ++i) {
        const auto& sp = result.stressPoints[i];
        os << "    {\"id\":" << sp.id << ",\"label\":\"" << sp.label << "\",\"y\":" << sp.global.y
           << ",\"z\":" << sp.global.z << ",\"y0\":" << sp.principal.y << ",\"z0\":" << sp.principal.z << "}";
        os << (i + 1 == result.stressPoints.size() ? "\n" : ",\n");
    }
    os << "  ],\n";
    os << "  \"meshSummary\": {\"nodes\": " << result.meshSummary.nodes.size()
       << ", \"triangles\": " << result.meshSummary.triangles.size() << "}\n";
    os << "}\n";
    return true;
}

bool Exporter::exportAnsys(const CalculationResult& result, const std::string& path, ErrorInfo* error) {
    std::ofstream os = openOutputFile(path);
    if (!os) {
        setFileError(error, path);
        return false;
    }
    const auto& p = result.properties;
    os << std::setprecision(15);
    // ANSYS Mechanical APDL -- BEAM188/BEAM189 arbitrary cross-section (ASEC).
    // SECDATA field order: A, Iy, Iz, Iyz, J, CGy, CGz, SHy, SHz, TKZ, TKY
    //   A          cross-sectional area
    //   Iy         moment of inertia about element y-axis
    //   Iz         moment of inertia about element z-axis
    //   Iyz        product of inertia
    //   J          torsional constant
    //   CGy, CGz   centroid coordinates relative to section origin
    //   SHy, SHz   shear centre offset relative to centroid (0 = not computed)
    //   TKZ, TKY   transverse shear stiffness factors (Az/A, Ay/A)
    os << "! ANSYS Mechanical APDL - Beam Section Card\n";
    os << "! Generated by SectionPropertyTool\n";
    os << "! Units match model input (mm, mm2, mm4)\n";
    os << "!\n";
    os << "SECTYPE,1,BEAM,ASEC,SPT_SECTION\n";
    // SECOFFSET,BSEC places the beam reference axis at the section geometric
    // origin (the same origin used to compute CGy/CGz). Without this command
    // ANSYS defaults to SECOFFSET,CENT and would double-count the centroid
    // offset because it shifts the axis to the centroid and then applies
    // CGy/CGz again.
    os << "SECOFFSET,BSEC\n";
    const double tkz = (p.area > 0.0) ? p.Az / p.area : 0.0;
    const double tky = (p.area > 0.0) ? p.Ay / p.area : 0.0;
    os << "SECDATA,"
       << p.area << ","   // A
       << p.Jy   << ","   // Iy
       << p.Jz   << ","   // Iz
       << p.Jyz  << ","   // Iyz
       << p.Jx   << ","   // J
       << p.cy   << ","   // CGy
       << p.cz   << ","   // CGz
       << 0.0    << ","   // SHy (shear centre not yet computed)
       << 0.0    << ","   // SHz
       << tkz    << ","   // TKZ = Az/A
       << tky    << "\n"; // TKY = Ay/A
    os << "!\n";
    os << "! Principal axes: theta=" << p.theta << " rad"
       << "  Jzo=" << p.Jzo << " mm4"
       << "  Jyo=" << p.Jyo << " mm4\n";
    if (!result.stressPoints.empty()) {
        os << "!\n";
        os << "! Stress recovery points (global Y-Z, mm):\n";
        for (const auto& sp : result.stressPoints) {
            os << "!   Point " << sp.id << " (" << sp.label << ")"
               << "  y=" << sp.global.y << "  z=" << sp.global.z << "\n";
        }
    }
    return true;
}

bool Exporter::exportAbaqus(const CalculationResult& result, const std::string& path, ErrorInfo* error) {
    std::ofstream os = openOutputFile(path);
    if (!os) {
        setFileError(error, path);
        return false;
    }
    const auto& p = result.properties;
    os << std::setprecision(15);
    // ABAQUS *BEAM GENERAL SECTION, SECTION=GENERAL
    // Data line: A, I11, I12, I22, J
    //   A    cross-sectional area
    //   I11  second moment about local axis 1 (= Iy)
    //   I12  product of inertia             (= Iyz)
    //   I22  second moment about local axis 2 (= Iz)
    //   J    torsional constant
    // *SECTION POINTS lists stress recovery coordinates (global Y-Z).
    os << "** ABAQUS Beam General Section\n";
    os << "** Generated by SectionPropertyTool\n";
    os << "** Units match model input (mm, mm2, mm4)\n";
    os << "**\n";
    os << "** Centroid: (" << p.cy << ", " << p.cz << ") mm"
       << "   theta=" << p.theta << " rad\n";
    os << "**\n";
    os << "*BEAM GENERAL SECTION, SECTION=GENERAL, ELSET=ALL_BEAMS\n";
    os << p.area << ", "  // A
       << p.Jy   << ", "  // I11 = Iy
       << p.Jyz  << ", "  // I12 = Iyz
       << p.Jz   << ", "  // I22 = Iz
       << p.Jx   << "\n"; // J
    if (!result.stressPoints.empty()) {
        os << "**\n";
        os << "*SECTION POINTS\n";
        for (const auto& sp : result.stressPoints) {
            os << sp.global.y << ", " << sp.global.z << "\n";
        }
    }
    return true;
}

bool Exporter::exportMidasCivil(const CalculationResult& result, const std::string& path, ErrorInfo* error) {
    std::ofstream os = openOutputFile(path);
    if (!os) {
        setFileError(error, path);
        return false;
    }
    const auto& p = result.properties;
    os << std::setprecision(15);
    // Midas Civil MCT (Midas Civil Text) -- DBUSER (user-defined) section.
    // *SECT header line:  iSECT, DBUSER, Name, iREFD, bSD, CY, CZ
    // Data line:          Area, Asy, Asz, Ixx, Iyy, Izz
    //   Ixx  torsional constant              (= Jx)
    //   Iyy  moment of inertia about y-axis  (= Jy)
    //   Izz  moment of inertia about z-axis  (= Jz)
    //   Asy  shear area in y direction       (= Ay)
    //   Asz  shear area in z direction       (= Az)
    os << "; Midas Civil MCT - User-Defined Section\n";
    os << "; Generated by SectionPropertyTool\n";
    os << "; Units match model input (mm, mm2, mm4)\n";
    os << ";\n";
    os << "; Centroid: CY=" << p.cy << "  CZ=" << p.cz << " mm\n";
    os << "; theta=" << p.theta << " rad"
       << "   Iyz=" << p.Jyz << " mm4\n";
    os << ";\n";
    os << "*SECT\n";
    os << "   1, DBUSER, SPT_SECTION, 0, 0, " << p.cy << ", " << p.cz << "\n";
    os << "   "
       << p.area << ", "  // Area
       << p.Ay   << ", "  // Asy
       << p.Az   << ", "  // Asz
       << p.Jx   << ", "  // Ixx (torsional)
       << p.Jy   << ", "  // Iyy
       << p.Jz   << "\n"; // Izz
    if (!result.stressPoints.empty()) {
        os << ";\n";
        os << "; Stress output points (global Y-Z, mm):\n";
        for (const auto& sp : result.stressPoints) {
            os << ";   Point " << sp.id << " (" << sp.label << ")"
               << "  y=" << sp.global.y << "  z=" << sp.global.z << "\n";
        }
    }
    return true;
}

}  // namespace spt

