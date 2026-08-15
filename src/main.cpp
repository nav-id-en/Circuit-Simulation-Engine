#include "proteus/ui/StudioApp.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    try {
        proteus::ui::StudioApp application;
        return application.run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
