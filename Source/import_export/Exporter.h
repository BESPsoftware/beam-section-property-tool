#pragma once

#include "common/DataModel.h"

#include <string>

namespace spt {

class Exporter {
public:
    static bool exportResult(const CalculationResult& result, const std::string& path, ExportFormat format, ErrorInfo* error = nullptr);
    static bool saveProject(const SectionModel& model, const std::string& path, ErrorInfo* error = nullptr);

private:
    static bool exportCsv(const CalculationResult& result, const std::string& path, ErrorInfo* error);
    static bool exportJson(const CalculationResult& result, const std::string& path, ErrorInfo* error);
};

}  // namespace spt

