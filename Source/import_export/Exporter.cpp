#include "import_export/Exporter.h"

#include "common/StringUtils.h"

#include <fstream>
#include <iomanip>
#include <sstream>

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
    const std::wstring widePath = utf8ToWide(path);
    return std::ofstream(widePath.c_str());
#else
    return std::ofstream(path);
#endif
}

void setFileError(ErrorInfo* error, const std::string& path) {
    if (error) {
        *error = {5001, ErrorSeverity::Error, path, "Unable to write export file.", "Check path permissions and retry."};
    }
}

std::string formatValue(double value, int precision) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

std::string format2(double value) {
    return formatValue(value, 2);
}

std::string format6(double value) {
    return formatValue(value, 6);
}

void writePropertyJson(std::ostream& os, const char* name, double value, int precision, bool comma = true) {
    os << "    \"" << name << "\": " << formatValue(value, precision);
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
    os << "Area," << format2(p.area) << ",mm2\n";
    os << "Jz," << format6(p.Jz) << ",mm4\n";
    os << "Jy," << format6(p.Jy) << ",mm4\n";
    os << "Jyz," << format6(p.Jyz) << ",mm4\n";
    os << "Jzo," << format6(p.Jzo) << ",mm4\n";
    os << "Jyo," << format6(p.Jyo) << ",mm4\n";
    os << "Jx," << format6(p.Jx) << ",mm4\n";
    os << "Az," << format2(p.Az) << ",mm2\n";
    os << "Ay," << format2(p.Ay) << ",mm2\n";
    os << "cy," << format2(p.cy) << ",mm\n";
    os << "cz," << format2(p.cz) << ",mm\n";
    os << "theta," << format2(p.theta) << ",rad\n";
    os << "Cw," << format6(p.warpingConstant) << ",mm6\n";
    os << "ys," << format2(p.shearCenterY) << ",mm\n";
    os << "zs," << format2(p.shearCenterZ) << ",mm\n";
    os << "\nstress_id,label,y,z,y0,z0,source,validity\n";
    for (const auto& sp : result.stressPoints) {
        os << sp.id << "," << sp.label << "," << format2(sp.global.y) << "," << format2(sp.global.z) << ","
           << format2(sp.principal.y) << "," << format2(sp.principal.z) << ","
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
    writePropertyJson(os, "Area", p.area, 2);
    writePropertyJson(os, "Jz", p.Jz, 6);
    writePropertyJson(os, "Jy", p.Jy, 6);
    writePropertyJson(os, "Jyz", p.Jyz, 6);
    writePropertyJson(os, "Jzo", p.Jzo, 6);
    writePropertyJson(os, "Jyo", p.Jyo, 6);
    writePropertyJson(os, "Jx", p.Jx, 6);
    writePropertyJson(os, "Az", p.Az, 2);
    writePropertyJson(os, "Ay", p.Ay, 2);
    writePropertyJson(os, "cy", p.cy, 2);
    writePropertyJson(os, "cz", p.cz, 2);
    writePropertyJson(os, "theta", p.theta, 2);
    writePropertyJson(os, "warpingConstant", p.warpingConstant, 6);
    writePropertyJson(os, "shearCenterY", p.shearCenterY, 2);
    writePropertyJson(os, "shearCenterZ", p.shearCenterZ, 2, false);
    os << "  },\n";
    os << "  \"stressPoints\": [\n";
    for (std::size_t i = 0; i < result.stressPoints.size(); ++i) {
        const auto& sp = result.stressPoints[i];
        os << "    {\"id\":" << sp.id << ",\"label\":\"" << sp.label << "\",\"y\":" << format2(sp.global.y)
           << ",\"z\":" << format2(sp.global.z) << ",\"y0\":" << format2(sp.principal.y)
           << ",\"z0\":" << format2(sp.principal.z) << "}";
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
       << format2(p.area) << ","   // A
       << format6(p.Jy)   << ","   // Iy
       << format6(p.Jz)   << ","   // Iz
       << format6(p.Jyz)  << ","   // Iyz
       << format6(p.Jx)   << ","   // J
       << format2(p.cy)   << ","   // CGy
       << format2(p.cz)   << ","   // CGz
       << format2(p.shearCenterY - p.cy) << ","   // SHy
       << format2(p.shearCenterZ - p.cz) << ","   // SHz
       << format2(tkz)    << ","   // TKZ = Az/A
       << format2(tky)    << "\n"; // TKY = Ay/A
    os << "!\n";
    os << "! Principal axes: theta=" << format2(p.theta) << " rad"
       << "  Jzo=" << format6(p.Jzo) << " mm4"
       << "  Jyo=" << format6(p.Jyo) << " mm4\n";
    os << "! Warping constant Cw = " << format6(p.warpingConstant)
       << " mm6   Shear center ys=" << format2(p.shearCenterY)
       << " zs=" << format2(p.shearCenterZ) << " mm\n";
    if (!result.stressPoints.empty()) {
        os << "!\n";
        os << "! Stress recovery points (global Y-Z, mm):\n";
        for (const auto& sp : result.stressPoints) {
            os << "!   Point " << sp.id << " (" << sp.label << ")"
               << "  y=" << format2(sp.global.y) << "  z=" << format2(sp.global.z) << "\n";
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
    os << "** Centroid: (" << format2(p.cy) << ", " << format2(p.cz) << ") mm"
       << "   theta=" << format2(p.theta) << " rad\n";
    os << "** Warping constant Cw = " << format6(p.warpingConstant)
       << " mm6   Shear center ys=" << format2(p.shearCenterY)
       << " zs=" << format2(p.shearCenterZ) << " mm\n";
    os << "**\n";
    os << "*BEAM GENERAL SECTION, SECTION=GENERAL, ELSET=ALL_BEAMS\n";
    os << format2(p.area) << ", "  // A
       << format6(p.Jy)   << ", "  // I11 = Iy
       << format6(p.Jyz)  << ", "  // I12 = Iyz
       << format6(p.Jz)   << ", "  // I22 = Iz
       << format6(p.Jx)   << "\n"; // J
    if (!result.stressPoints.empty()) {
        os << "**\n";
        os << "*SECTION POINTS\n";
        for (const auto& sp : result.stressPoints) {
            os << format2(sp.global.y) << ", " << format2(sp.global.z) << "\n";
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
    os << "; Centroid: CY=" << format2(p.cy) << "  CZ=" << format2(p.cz) << " mm\n";
    os << "; theta=" << format2(p.theta) << " rad"
       << "   Iyz=" << format6(p.Jyz) << " mm4\n";
    os << "; Warping constant Cw = " << format6(p.warpingConstant)
       << " mm6   Shear center ys=" << format2(p.shearCenterY)
       << " zs=" << format2(p.shearCenterZ) << " mm\n";
    os << ";\n";
    os << "*SECT\n";
    os << "   1, DBUSER, SPT_SECTION, 0, 0, " << format2(p.cy) << ", " << format2(p.cz) << "\n";
    os << "   "
       << format2(p.area) << ", "  // Area
       << format2(p.Ay)   << ", "  // Asy
       << format2(p.Az)   << ", "  // Asz
       << format6(p.Jx)   << ", "  // Ixx (torsional)
       << format6(p.Jy)   << ", "  // Iyy
       << format6(p.Jz)   << "\n"; // Izz
    if (!result.stressPoints.empty()) {
        os << ";\n";
        os << "; Stress output points (global Y-Z, mm):\n";
        for (const auto& sp : result.stressPoints) {
            os << ";   Point " << sp.id << " (" << sp.label << ")"
               << "  y=" << format2(sp.global.y) << "  z=" << format2(sp.global.z) << "\n";
        }
    }
    return true;
}

}  // namespace spt

