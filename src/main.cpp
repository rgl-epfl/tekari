#include <tekari/bsdf_application.h>

#if defined(EMSCRIPTEN)
#  include <emscripten.h>
#endif

#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/progressbar.h>
#include <nanogui/messagedialog.h>

int main(int argc, char** argv) {
    using namespace tekari;
    using namespace nanogui;

    vector<string> dataset_paths;
    bool log_mode = false,
         help = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-l") == 0) {
            log_mode = true;
            continue;
        }
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            help = true;
            continue;
        }
        if (strncmp(argv[i], "-psn_", 5) == 0) // OSX app launch ID
            continue;
        dataset_paths.push_back(argv[i]);
    }

    if (help) {
        std::cout << "Usage: tekari [-l] <file1.bsdf> <file2.bsdf> ..." << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "   -l      Directly open in logarithmic view." << std::endl;
        return 0;
    }

    try {
        init();
        // scoped variables
        {
            #if defined(EMSCRIPTEN)
                vector<string> dataset_paths_launch;
            #else
                const auto& dataset_paths_launch = dataset_paths;
            #endif
            ref<BSDFApplication> screen = new BSDFApplication(dataset_paths_launch,
                                                              log_mode);

#if defined(EMSCRIPTEN)
            Window *window = new Window(screen, "Please wait");
            Label *label = new Label(window, " ");
            window->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 15, 15));
            ProgressBar *progress = new ProgressBar(window);
            progress->set_fixed_width(250);
            window->center();

            if (dataset_paths.size() != 1)
                throw std::runtime_error("Invalid number of program arguments!");

            struct Temp {
                BSDFApplication *screen;
                Window *window;
                ProgressBar *progress;
                const char *fname;
                std::string fname_short;
            } temp;

            temp.screen = screen;
            temp.progress = progress;
            temp.window = window;
            temp.fname = dataset_paths[0].c_str();
            auto tokens = tokenize(temp.fname, "/");
            temp.fname_short = tokens[tokens.size()-1];

            emscripten_async_wget2(
                temp.fname,
                temp.fname_short.c_str(),
                "GET",
                nullptr,
                &temp,
                (em_async_wget2_onload_func) [](unsigned, void* ptr, const char*) {
                    Temp *temp = (Temp *) ptr;
                    temp->screen->remove_child(temp->window);
                    temp->screen->open_files({temp->fname_short});
                    temp->screen->redraw();
                },
                (em_async_wget2_onstatus_func) [](unsigned, void* ptr, int err) {
                    Temp *temp = (Temp *) ptr;
                    temp->screen->remove_child(temp->window);
                    new MessageDialog(
                        temp->screen, MessageDialog::Type::Warning, "Error",
                        "The file \"" + std::string(temp->fname) +
                            "\" could not be dowloaded: error code " + std::to_string(err));
                    temp->screen->redraw();
                },
                (em_async_wget2_onstatus_func) [](unsigned, void* ptr, int progress) {
                    Temp *temp = (Temp *) ptr;
                    temp->progress->set_value(progress / 100.f);
                    temp->screen->redraw();
                }
            );
#endif

            screen->set_visible(true);
            screen->perform_layout();
            mainloop(-1);
        }

        shutdown();
    } catch (const std::runtime_error& e) {
        string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(_WIN32)
            MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            cerr << error_msg << endl;
        #endif
        return -1;
    }

    return 0;
}
