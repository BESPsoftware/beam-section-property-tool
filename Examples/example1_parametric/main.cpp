#include "section_property_tool.h"

#include <iostream>

int main() {
    const SptParameter values[] = {
        {"A", 100.0},
        {"H", 210.0},
        {"e", 20.0},
        {"f", 12.0},
    };
    SptSectionParameters params{SPT_H_SECTION, values, 4};
    SptSectionHandle section = nullptr;
    if (spt_create_section_from_parameters(&params, &section) != 0) {
        std::cerr << spt_get_last_error()->message << "\n";
        return 1;
    }
    SptResultHandle result = nullptr;
    spt_calculate_section_properties(section, &result);
    SptSectionProperties props{};
    spt_get_result_properties(result, &props);
    std::cout << "Area: " << props.area << " mm2\n";
    std::cout << "Jz: " << props.Jz << " mm4\n";
    std::cout << "Jy: " << props.Jy << " mm4\n";
    spt_destroy_result(result);
    spt_destroy_section(section);
    return 0;
}

