#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace spt {

constexpr double kPi = 3.141592653589793238462643383279502884;

enum class SectionType {
    HSection,
    BoxSection,
    PipeSection,
    QuaysideCraneGirder,
    CanvasThinWalled
};

enum class StressPointSource {
    Default,
    User
};

enum class StressPointValidity {
    Valid,
    OutsideSection,
    Disconnected,
    StaleAfterGeometryChange
};

enum class ErrorSeverity {
    Info,
    Warning,
    Error
};

enum class ExportFormat {
    Csv,
    Json,
    Ansys,
    Abaqus,
    MidasCivil
};

struct PointYZ {
    double y = 0.0;
    double z = 0.0;
};

struct ErrorInfo {
    int code = 0;
    ErrorSeverity severity = ErrorSeverity::Info;
    std::string field;
    std::string message;
    std::string remediation;
};

struct SectionParameters {
    SectionType type = SectionType::HSection;
    std::map<std::string, double> values;

    double get(const std::string& name, double fallback = 0.0) const {
        const auto it = values.find(name);
        return it == values.end() ? fallback : it->second;
    }
};

struct Material {
    std::string name = "Steel";
    double elasticModulus = 0.0;
    double shearModulus = 0.0;
    double poissonsRatio = 0.3;
    double density = 0.0;
};

struct RectComponent {
    double y1 = 0.0;
    double z1 = 0.0;
    double y2 = 0.0;
    double z2 = 0.0;
    int materialId = 0;
    std::string id;
};

struct PlateSegment {
    PointYZ start;
    PointYZ end;
    double thickness = 0.0;
    int materialId = 0;
    std::string id;
};

struct Contour {
    std::vector<PointYZ> outer;
    std::vector<std::vector<PointYZ>> holes;
};

struct SectionProperties {
    double area = 0.0;
    double Jz = 0.0;
    double Jy = 0.0;
    double Jyz = 0.0;
    double Jzo = 0.0;
    double Jyo = 0.0;
    double Jx = 0.0;
    double Az = 0.0;
    double Ay = 0.0;
    double cy = 0.0;
    double cz = 0.0;
    double theta = 0.0;
    double shearCenterY = 0.0;
    double shearCenterZ = 0.0;
    double warpingConstant = 0.0;
    std::vector<std::string> notes;
};

struct StressPoint {
    int id = 0;
    std::string label;
    PointYZ global;
    PointYZ principal;
    StressPointSource source = StressPointSource::Default;
    StressPointValidity validity = StressPointValidity::Valid;
};

struct MeshSettings {
    double targetSize = 0.0;
    double refinementFactor = 1.0;
    int curvedSegmentCount = 96;
    bool autoUpdate = true;
};

struct MeshTriangle {
    int n1 = 0;
    int n2 = 0;
    int n3 = 0;
    int materialId = 0;
};

struct MeshEdge {
    int n1 = 0;
    int n2 = 0;
};

struct MeshModel {
    std::vector<PointYZ> nodes;
    std::vector<MeshTriangle> triangles;
    std::vector<MeshEdge> boundaryEdges;
    std::vector<ErrorInfo> diagnostics;
};

struct SectionModel {
    SectionType type = SectionType::HSection;
    SectionParameters parameters;
    std::string name;
    std::string originConvention;
    std::vector<Material> materials;
    std::vector<RectComponent> rectangles;
    std::vector<PlateSegment> plates;
    std::vector<Contour> contours;
    bool hasOverrideProperties = false;
    SectionProperties overrideProperties;
    std::vector<StressPoint> overrideStressPoints;
    std::vector<std::string> warnings;
};

struct CalculationResult {
    SectionProperties properties;
    std::vector<StressPoint> stressPoints;
    MeshModel meshSummary;
    std::vector<ErrorInfo> diagnostics;
};

}  // namespace spt

