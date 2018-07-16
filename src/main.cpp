#include <tekari/BSDFApplication.h>

#include <nanogui/glutil.h>

#include <vector>
#include <string>
#include <iostream>

int main(int argc, char* * argv) {

    std::vector<std::string> data_sample_paths;
    for (int i = 1; i < argc; ++i)
    {
        data_sample_paths.push_back(argv[i]);
    }

    // nanogui
    try {
        nanogui::init();
        // scoped variables
        {
            nanogui::ref<tekari::BSDFApplication> app = new tekari::BSDFApplication(data_sample_paths);
            app->draw_all();
            app->set_visible(true);
            nanogui::mainloop(50);
        }

        nanogui::shutdown();
    } catch (const std::runtime_error& e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(_WIN32)
            Message_box_a(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            std::cerr << error_msg << std::endl;
        #endif
        return -1;
    }
    
    return 0;
}
