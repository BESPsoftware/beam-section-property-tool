#include <exception>
#include <iostream>

bool runRegressionTests();
bool runApiTests();

int main() {
    try {
        const bool regressionOk = runRegressionTests();
        const bool apiOk = runApiTests();
        if (!regressionOk || !apiOk) {
            std::cerr << "SectionPropertyTests failed\n";
            return 1;
        }
        std::cout << "SectionPropertyTests passed\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Unhandled exception: " << ex.what() << "\n";
        return 1;
    }
}

