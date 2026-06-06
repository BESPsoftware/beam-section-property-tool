#include "section_property_tool.h"

#include <iostream>
#include <limits>
#if defined(_WIN32)
#  include <io.h>
#else
#  include <unistd.h>
#endif

static bool shouldPauseConsole() {
#if defined(_WIN32)
    return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
    return isatty(fileno(stdin)) && isatty(fileno(stdout));
#endif
}

static void pauseConsole() {
    if (!shouldPauseConsole()) {
        return;
    }
    std::cout << "Press Enter to exit..." << std::flush;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

namespace {

void runCase(const char* name, SptSectionType type, const SptParameter* values, size_t count) {
    SptSectionParameters params{type, values, count};
    SptSectionHandle section = nullptr;
    if (spt_create_section_from_parameters(&params, &section) != 0) {
        std::cerr << name << ": " << spt_get_last_error()->message << "\n";
        return;
    }
    SptResultHandle result = nullptr;
    spt_calculate_section_properties(section, &result);
    SptSectionProperties props{};
    spt_get_result_properties(result, &props);
    std::cout << name << ": Area=" << props.area << " Jx=" << props.Jx
              << " Cw=" << props.warping_constant
              << " ys=" << props.shear_center_y
              << " zs=" << props.shear_center_z << "\n";
    spt_destroy_result(result);
    spt_destroy_section(section);
}

}  // namespace

int main() {
    const SptParameter h[] = {{"A", 100.0}, {"H", 210.0}, {"e", 20.0}, {"f", 12.0}};
    const SptParameter pipe[] = {{"Do", 1300.0}, {"t", 14.0}};
    runCase("H Section", SPT_H_SECTION, h, 4);
    runCase("Pipe Section", SPT_PIPE_SECTION, pipe, 2);
    pauseConsole();
    return 0;
}

