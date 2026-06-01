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
        case ExportFormat::Abaqus:
        case ExportFormat::MidasCivil:
            if (error) {
                *error = {5002, ErrorSeverity::Warning, path, "FEM card export is a documented v1 placeholder.", "Use JSON or CSV in this release."};
            }
            return false;
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

}  // namespace spt

