#include <exception>
#include <iostream>
#include <limits>
#if defined(_WIN32)
#  include <io.h>
#else
#  include <unistd.h>
#endif

bool runRegressionTests();
bool runApiTests();

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
    try {
        const bool regressionOk = runRegressionTests();
        const bool apiOk = runApiTests();
        if (!regressionOk || !apiOk) {
            std::cerr << "SectionPropertyTests failed\n";
            pauseConsole();
            return 1;
        }
        std::cout << "SectionPropertyTests passed\n";
        pauseConsole();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Unhandled exception: " << ex.what() << "\n";
        pauseConsole();
        return 1;
    }
}

