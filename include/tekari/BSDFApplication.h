#pragma once

#include "common.h"

#include <nanogui/screen.h>
#include <memory>
#include <thread>

#include "BSDFCanvas.h"
#include "DataSampleButton.h"
#include "MetadataWindow.h"
#include "ColorMapSelectionWindow.h"
#include "HelpWindow.h"
#include "ColorMap.h"
#include "SharedQueue.h"
#include "ThreadPool.h"

TEKARI_NAMESPACE_BEGIN

using nanogui::Widget;
using nanogui::Screen;
using nanogui::Window;
using nanogui::Button;
using nanogui::CheckBox;
using nanogui::PopupButton;
using nanogui::Label;
using nanogui::VScrollPanel;
using nanogui::ComboBox;
using nanogui::GLFramebuffer;

struct DataSample_to_add
{
    std::string error_msg;
    std::shared_ptr<DataSample> data_sample;
};

class BSDFApplication : public Screen {
public:
    BSDFApplication(const std::vector<std::string>& data_sample_paths);
    ~BSDFApplication();

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

    void draw_contents() override;
    void request_layout_update() { m_requires_layout_update = true; }

    void open_data_sample_dialog();
    void save_screen_shot();
    void save_selected_data_sample();

    void toggle_window(Window*& window, std::function<Window*(void)> create_window);
    void toggle_metadata_window();
    void toggle_help_window();
    void toggle_selection_info_window();
    void update_selection_info_window();
    void toggle_unsaved_data_window(const std::vector<std::string>& data_sample_names, std::function<void(void)> continue_callback);
    void toggle_color_map_selection_window();
    
    void select_color_map(std::shared_ptr<ColorMap> color_map);
    
    void delete_data_sample(std::shared_ptr<DataSample> data_sample);
    void select_data_sample(std::shared_ptr<DataSample> data_sample);
    void select_data_sample(int index, bool clamped = true);

    int data_sample_index(const std::shared_ptr<const DataSample> data_sample) const;
    int selected_data_sample_index() const { return data_sample_index(m_selected_ds); }

    DataSampleButton* corresponding_button(const std::shared_ptr<const DataSample> data_sample);
    const DataSampleButton* corresponding_button(const std::shared_ptr<const DataSample> data_sample) const;

    void hide_windows();

private:
    void toggle_view(DataSample::Views view, std::shared_ptr<DataSample> data_sample, bool toggle);
    void toggle_log_view(std::shared_ptr<DataSample> data_sample);

    void update_layout();
    void add_data_sample(std::shared_ptr<DataSample> data_sample);

    void toggle_tool_button(Button* button);

    void open_files(const std::vector<std::string>& data_sample_paths);
    void try_load_data_sample(std::string file_path, std::shared_ptr<DataSample_to_add> data_sample_to_add);

    void toggle_canvas_draw_flags(int flag, CheckBox* checkbox);

    void reprint_footer();

private:
    bool m_requires_layout_update = false;
    bool m_distraction_free_mode = false;

    Window* m_tool_window;
    Widget* m3DView;

    PopupButton* m_hidden_options_button;
    CheckBox* m_use_shadows_checkbox;
    CheckBox* m_use_specular_checkbox;
    CheckBox* m_display_center_axis;
    CheckBox* m_display_degrees_checkbox;
    CheckBox* m_display_predicted_outgoing_angle_checkbox;

    // footer
    Widget* m_footer;
    Label* m_data_sample_name;
    Label* m_data_sample_points_count;
    Label* m_data_sample_average_height;

    // data sample scroll panel
    VScrollPanel* m_data_samples_scroll_panel;
    Widget* m_scroll_content;
    Widget* m_data_sample_button_container;

    // tool buttons
    Button* m_help_button;
    Button* m_grid_view_toggle;
    Button* m_ortho_view_toggle;

    // dialog windows
    Window* m_metadata_window;
    Window* m_help_window;
    Window* m_color_map_selection_window;
    Window* m_selection_info_window;
    Window* m_unsaved_data_window;

    // canvas
    BSDFCanvas* m_bsdf_canvas;

    // cursors and mouse mode
    ComboBox* m_mouse_mode_selector;
    GLFWcursor* m_cursors[BSDFCanvas::MOUSE_MODE_COUNT];

    std::vector<std::shared_ptr<DataSample>> m_data_samples;
    std::shared_ptr<DataSample> m_selected_ds;
    std::vector<std::shared_ptr<ColorMap>> m_color_maps;

    // offscreen buffer
    GLFramebuffer m_framebuffer;

    SharedQueue<std::shared_ptr<DataSample_to_add>> m_data_samples_to_add;

    // threadpool
    ThreadPool<8> m_thread_pool;
};

TEKARI_NAMESPACE_END