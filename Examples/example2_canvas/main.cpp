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

int main() {
    const SptPlateSegment lines[] = {
        {{0.0, 0.0}, {100.0, 0.0}, 10.0, 0, "bottom"},
        {{50.0, 0.0}, {50.0, 120.0}, 8.0, 0, "web"},
        {{0.0, 120.0}, {100.0, 120.0}, 10.0, 0, "top"},
    };
    SptSectionHandle section = nullptr;
    if (spt_create_section_from_canvas_lines(lines, 3, &section) != 0) {
        std::cerr << spt_get_last_error()->message << "\n";
        return 1;
    }
    SptResultHandle result = nullptr;
    spt_calculate_section_properties(section, &result);
    SptSectionProperties props{};
    spt_get_result_properties(result, &props);
    std::cout << "Canvas area: " << props.area << " mm2\n";
    std::cout << "Canvas centroid: (" << props.cy << ", " << props.cz << ") mm\n";
    spt_destroy_result(result);
    spt_destroy_section(section);
    pauseConsole();
    return 0;
}

