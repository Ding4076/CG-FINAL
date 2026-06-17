#include "aim_trainer.h"

#include <iostream>

Options getOptions(int argc, char* argv[]) {
    Options options;
    options.windowTitle = "AimTrainer - Final Project";
    options.windowWidth = 1280;
    options.windowHeight = 720;
    options.windowResizable = false;
    options.vSync = true;
    options.msaa = true;
    options.glVersion = {3, 3};
    options.backgroundColor = glm::vec4(0.1f, 0.1f, 0.12f, 1.0f);
    // Relative asset path. CMake copies finalproject/media next to the exe
    // (POST_BUILD copy_directory), so "media/" resolves from the exe's working
    // directory regardless of machine / build mode. Run the exe from its own
    // directory (the default when double-clicked).
    options.assetRootDir = "media/";
    return options;
}

int main(int argc, char* argv[]) {
    try {
        AimTrainer app(getOptions(argc, argv));
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown Error" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
