#include <tekari/bsdf_application.h>

using namespace tekari;

int main(int argc, char** argv) {

    vector<string> data_sample_paths;
    for (int i = 1; i < argc; ++i)
    {
        data_sample_paths.push_back(argv[i]);
    }
    // for debugging purposes
    // data_sample_paths.push_back(DATA_SAMPLES_PATH "gold_satin_spec.bsdf");
    // data_sample_paths.push_back(DATA_SAMPLES_PATH "iridescent_paper.txt");
    // data_sample_paths.push_back(DATA_SAMPLES_PATH "iridescent-paper.txt");

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
        string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(_WIN32)
            Message_box_a(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            cerr << error_msg << endl;
        #endif
        return -1;
    }
    
    return 0;
}
