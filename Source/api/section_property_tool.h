#pragma once

#include <stddef.h>

#if defined(_WIN32)
#  if defined(SPT_BUILD_DLL)
#    define SPT_API __declspec(dllexport)
#  else
#    define SPT_API __declspec(dllimport)
#  endif
#else
#  define SPT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SptSection_T* SptSectionHandle;
typedef struct SptResult_T* SptResultHandle;
typedef struct SptMesh_T* SptMeshHandle;

typedef enum SptSectionType {
    SPT_H_SECTION = 0,
    SPT_BOX_SECTION = 1,
    SPT_PIPE_SECTION = 2,
    SPT_CRANE_GIRDER = 3,
    SPT_CANVAS = 4
} SptSectionType;

typedef enum SptExportFormat {
    SPT_EXPORT_CSV = 0,
    SPT_EXPORT_JSON = 1,
    SPT_EXPORT_ANSYS = 2,
    SPT_EXPORT_ABAQUS = 3,
    SPT_EXPORT_MIDAS_CIVIL = 4
} SptExportFormat;

typedef struct SptParameter {
    const char* name;
    double value;
} SptParameter;

typedef struct SptSectionParameters {
    SptSectionType type;
    const SptParameter* parameters;
    size_t parameter_count;
} SptSectionParameters;

typedef struct SptPointYZ {
    double y;
    double z;
} SptPointYZ;

typedef struct SptPlateSegment {
    SptPointYZ start;
    SptPointYZ end;
    double thickness;
    int material_id;
    const char* id;
} SptPlateSegment;

typedef struct SptSectionProperties {
    double area;
    double Jz;
    double Jy;
    double Jyz;
    double Jzo;
    double Jyo;
    double Jx;
    double Az;
    double Ay;
    double cy;
    double cz;
    double theta;
    double warping_constant;
    double shear_center_y;
    double shear_center_z;
} SptSectionProperties;

typedef struct SptStressPoint {
    int id;
    char label[32];
    double y;
    double z;
    double y0;
    double z0;
    int source;
    int validity;
} SptStressPoint;

typedef struct SptStressPointArray {
    SptStressPoint* points;
    size_t count;
} SptStressPointArray;

typedef struct SptMeshSettings {
    double target_size;
    double refinement_factor;
    int curved_segment_count;
    int auto_update;
} SptMeshSettings;

typedef struct SptMeshCounts {
    size_t node_count;
    size_t triangle_count;
    size_t boundary_edge_count;
} SptMeshCounts;

typedef struct SptErrorInfo {
    int code;
    int severity;
    char field[64];
    char message[256];
    char remediation[256];
} SptErrorInfo;

SPT_API int spt_get_version(int* major, int* minor, int* patch);
SPT_API int spt_create_section_from_parameters(const SptSectionParameters* params, SptSectionHandle* out);
SPT_API int spt_create_section_from_canvas_lines(const SptPlateSegment* lines, size_t count, SptSectionHandle* out);
SPT_API int spt_calculate_section_properties(SptSectionHandle section, SptResultHandle* out);
SPT_API int spt_get_result_properties(SptResultHandle result, SptSectionProperties* out);
SPT_API int spt_get_result_stress_points(SptResultHandle result, SptStressPointArray* out);
SPT_API int spt_generate_default_stress_points(SptSectionHandle section, SptStressPointArray* out);
SPT_API int spt_update_stress_points(SptSectionHandle section, const SptStressPoint* points, size_t count);
SPT_API int spt_create_mesh(SptSectionHandle section, const SptMeshSettings* settings, SptMeshHandle* out);
SPT_API int spt_get_mesh_counts(SptMeshHandle mesh, SptMeshCounts* out);
SPT_API int spt_export_results(SptResultHandle result, const char* path_utf8, SptExportFormat format);
SPT_API void spt_free_stress_point_array(SptStressPointArray* array);
SPT_API void spt_destroy_section(SptSectionHandle section);
SPT_API void spt_destroy_result(SptResultHandle result);
SPT_API void spt_destroy_mesh(SptMeshHandle mesh);
SPT_API const SptErrorInfo* spt_get_last_error(void);

#ifdef __cplusplus
}
#endif

