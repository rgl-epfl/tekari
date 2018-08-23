#pragma once

#include <tekari/common.h>

#include <nanogui/screen.h>
#include <nanogui/textbox.h>

#include <thread>

#include <tekari/bsdf_canvas.h>
#include <tekari/data_sample_button.h>
#include <tekari/metadata_window.h>
#include <tekari/color_map_selection_window.h>
#include <tekari/help_window.h>
#include <tekari/color_map.h>
#include <tekari/shared_queue.h>
#include <tekari/slider_2d.h>
#include <tekari/thread_pool.h>

TEKARI_NAMESPACE_BEGIN

using nanogui::Widget;
using nanogui::Screen;
using nanogui::Window;
using nanogui::Button;
using nanogui::ToolButton;
using nanogui::CheckBox;
using nanogui::PopupButton;
using nanogui::Label;
using nanogui::VScrollPanel;
using nanogui::ComboBox;
using nanogui::Slider;
using nanogui::FloatBox;
using nanogui::IntBox;
using nanogui::GLFramebuffer;

struct DataSample_to_add
{
    string error_msg;
    std::shared_ptr<DataSample> data_sample;
};

class BSDFApplication : public Screen {
public:
    BSDFApplication(const vector<string>& data_sample_paths);
    ~BSDFApplication();

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

    void draw_contents() override;
    void request_layout_update() { m_requires_layout_update = true; }

    void open_data_sample_dialog();
    void save_screen_shot();
    void save_selected_data_sample();

    void toggle_window(Window*& window, function<Window*(void)> create_window);
    void toggle_metadata_window();
    void toggle_brdf_options_window();
    void toggle_help_window();
    void toggle_selection_info_window();
    void update_selection_info_window();
    void toggle_unsaved_data_window(const vector<string>& data_sample_names, function<void(void)> continue_callback);
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

    void open_files(const vector<string>& data_sample_paths);
private:
    void toggle_view(DataSample::Views view);

    void update_layout();
    void add_data_sample(std::shared_ptr<DataSample> data_sample);

    void toggle_tool_checkbox(CheckBox* checkbox);

    void try_load_data_sample(const string& file_path, std::shared_ptr<DataSample_to_add> data_sample_to_add);

    void reprint_footer();

private:
    bool m_requires_layout_update = false;
    bool m_distraction_free_mode = false;

    Window* m_tool_window;
    Widget* m_3d_view;

    // Hidden options widgets
    PopupButton* m_hidden_options_button;
    CheckBox* m_grid_view_checkbox;
    CheckBox* m_ortho_view_checkbox;
    CheckBox* m_use_diffuse_shading_checkbox;
    CheckBox* m_use_specular_shading_checkbox;
    CheckBox* m_use_wireframe_checkbox;
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

    // bsdf settings
    Button* m_display_as_log;
    Button* m_view_toggles[DataSample::Views::VIEW_COUNT];
    FloatBox<float>* m_phi_float_box;
    FloatBox<float>* m_theta_float_box;
    Slider2D* m_incident_angle_slider;

    // dialog windows
    Window* m_metadata_window;
    Window* m_brdf_options_window;
    Window* m_help_window;
    Window* m_color_map_selection_window;
    Window* m_selection_info_window;
    Window* m_unsaved_data_window;

    // canvas
    BSDFCanvas* m_bsdf_canvas;

    // cursors and mouse mode
    Button* m_mouse_mode_buttons[BSDFCanvas::MOUSE_MODE_COUNT];
    GLFWcursor* m_cursors[BSDFCanvas::MOUSE_MODE_COUNT];

    vector<std::shared_ptr<DataSample>> m_data_samples;
    std::shared_ptr<DataSample> m_selected_ds;
    vector<std::shared_ptr<ColorMap>> m_color_maps;

    // offscreen buffer
    GLFramebuffer m_framebuffer;

    SharedQueue<std::shared_ptr<DataSample_to_add>> m_data_samples_to_add;

    // threadpool
    ThreadPool<8> m_thread_pool;
};

TEKARI_NAMESPACE_END