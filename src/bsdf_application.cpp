#include <tekari/bsdf_application.h>

#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/popupbutton.h>
#include <nanogui/colorwheel.h>
#include <nanogui/checkbox.h>
#include <nanogui/imageview.h>
#include <nanogui/slider.h>
#include <nanogui/combobox.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/messagedialog.h>
#include <nanogui/label.h>
#include <nanogui/graph.h>

#include <algorithm>
#include <bitset>
#include <string>

#include <stb_image.h>
#include <tekari/light_theme.h>
#include <tekari/arrow.h>
#include <tekari/bsdf_data_sample.h>
#include <tekari/standard_data_sample.h>
#include <tekari_resources.h>

#define FOOTER_HEIGHT 25

using nanogui::Window;
using nanogui::Widget;
using nanogui::MessageDialog;
using nanogui::BoxLayout;
using nanogui::GridLayout;
using nanogui::GroupLayout;
using nanogui::Orientation;
using nanogui::Label;
using nanogui::ColorWheel;
using nanogui::Slider;
using nanogui::Alignment;
using nanogui::Theme;
using nanogui::Graph;

TEKARI_NAMESPACE_BEGIN

BSDFApplication::BSDFApplication(const vector<string>& data_sample_paths)
:   Screen(Vector2i(1200, 750), "Tekari", true, false, 8, 8, 24, 8, 2)
,   m_metadata_window(nullptr)
,   m_data_sample_sliders_window(nullptr)
,   m_help_window(nullptr)
,   m_color_map_selection_window(nullptr)
,   m_selection_info_window(nullptr)
,   m_unsaved_data_window(nullptr)
,   m_selected_ds(nullptr)
{
    Arrow::instance().load_shaders();

    // load color maps
    for (auto& p : ColorMap::PREDEFINED_MAPS)
    {
        m_color_maps.push_back(make_shared<ColorMap>(p.first, p.second.first, p.second.second));
    }

    m_3d_view = new Widget{this};
    m_3d_view->set_layout(new BoxLayout{ Orientation::Vertical, Alignment::Fill });

    // canvas
    m_bsdf_canvas = new BSDFCanvas{ m_3d_view };
    m_bsdf_canvas->set_background_color(Color(55, 255));
    m_bsdf_canvas->set_color_map(m_color_maps[0]);
    m_bsdf_canvas->set_selection_callback([this](const Matrix4f& mvp, const SelectionBox& selection_box,
        const Vector2i& canvas_size, SelectionMode mode) {
        if (!m_selected_ds)
            return;

        
        if (selection_box.empty())
        {
            m_selected_ds->select_closest_point(mvp, selection_box.top_left, canvas_size);
        }
        else
        {
            m_selected_ds->select_points(mvp, selection_box, canvas_size, mode);
        }
        update_selection_info_window();
    });
    m_bsdf_canvas->set_update_incident_angle_callback([this](const Vector2f& incident_angle) {
        if (!m_selected_ds)
            return;
        
        m_selected_ds->set_incident_angle(incident_angle);

        if (!m_data_sample_sliders_window)
            return;
        m_theta_float_box->set_value(incident_angle.x());
        m_phi_float_box->set_value(incident_angle.y());
        m_incident_angle_slider->set_value(incident_angle);
        reprint_footer();
    });

    // Footer
    {
        m_footer = new Widget{ m_3d_view };
        m_footer->set_layout(new GridLayout{ Orientation::Horizontal, 3, Alignment::Fill, 5});

        auto make_footer_info = [this](string label) {
            auto container = new Widget{ m_footer };
            container->set_layout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill });
            container->set_fixed_width(width() / 3);
            new Label{ container, label };
            auto info = new Label{ container, "-" };
            return info;
        };

        m_data_sample_name = make_footer_info("Data Sample Name : ");
        m_data_sample_points_count = make_footer_info("Points Count : ");
        m_data_sample_average_height = make_footer_info("Average_intensity : ");
    }

    m_tool_window = new Window(this, "Tools");
    m_tool_window->set_layout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 5, 5});
    m_tool_window->set_visible(true);
    m_tool_window->set_position({ 20, 20 });

    m_help_button = new Button(m_tool_window->button_panel(), "", ENTYPO_ICON_HELP);
    m_help_button->set_callback([this]() { toggle_help_window(); });
    m_help_button->set_font_size(15);
    m_help_button->set_tooltip("Information about using BSDF Vidualizer (H)");
    m_help_button->set_position({20, 0});

    // Hidden options
    {
        m_hidden_options_button = new PopupButton(m_tool_window->button_panel(), "", ENTYPO_ICON_TOOLS);
        m_hidden_options_button->set_background_color(Color{0.4f, 0.1f, 0.1f, 1.0f});
        m_hidden_options_button->set_tooltip("More view options");
        auto hidden_options_popup = m_hidden_options_button->popup();
        hidden_options_popup->set_layout(new GroupLayout{});

        auto add_hidden_option_toggle = [hidden_options_popup](const string& label, const string& tooltip,
            const function<void(bool)>& callback, bool checked = false) {
            auto checkbox = new CheckBox{ hidden_options_popup, label, callback };
            checkbox->set_checked(checked);
            checkbox->set_tooltip(tooltip);
            return checkbox;
        };

        new Label{ hidden_options_popup, "Advanced View Options", "sans-bold" };
        m_use_shadows_checkbox = add_hidden_option_toggle("Shadows", "Enable/Disable shadows (Shift+S)",
            [this](bool checked) {
            m_bsdf_canvas->set_draw_flag(USE_SHADOWS, checked);
            m_use_specular_checkbox->set_enabled(checked);
        }, true);
        m_use_specular_checkbox = add_hidden_option_toggle("Specular", "Enable/Disable specular lighting",
            [this](bool checked) {
            m_bsdf_canvas->set_draw_flag(USE_SPECULAR, checked);
        }, false);
#if !defined(EMSCRIPTEN)
        m_use_wireframe_checkbox = add_hidden_option_toggle("Wireframe", "Enable/Disable wireframe",
            [this](bool checked) {
            m_bsdf_canvas->set_draw_flag(USE_WIREFRAME, checked);
        }, false);
#endif
        m_display_center_axis = add_hidden_option_toggle("Center Axis", "Show/Hide Center Axis (A)",
            [this](bool checked) {
            m_bsdf_canvas->set_draw_flag(DISPLAY_AXIS, checked);
        }, true);
        m_display_predicted_outgoing_angle_checkbox = add_hidden_option_toggle("Predicted Outgoing Angle", "Show/Hide Predicted Outgoing Angle (Ctrl+I)",
            [this](bool checked) {
            m_bsdf_canvas->set_draw_flag(DISPLAY_PREDICTED_OUTGOING_ANGLE, checked);
        });
        add_hidden_option_toggle("Use Light Theme", "Switch from dark to light theme",
            [this](bool checked) {
            set_theme(checked ? new LightTheme{ nvg_context() } : new Theme{ nvg_context() });
            set_background(m_theme->m_window_fill_focused);
            m_hidden_options_button->set_background_color(checked ? Color{ 0.7f, 0.3f, 0.3f, 1.0f } : Color{ 0.4f, 0.1f, 0.1f, 1.0f });
        });

        auto point_size_label = new Label{ hidden_options_popup , "Point Size" };
        point_size_label->set_tooltip("Changes the point size based on a arbitrary heuristic (also distance dependent)");
        auto point_size_slider = new Slider{ hidden_options_popup };
        point_size_slider->set_range(make_pair(0.1f, 20.0f));
        point_size_slider->set_value(m_bsdf_canvas->point_size_scale());
        point_size_slider->set_callback([this](float value) {
            m_bsdf_canvas->set_point_size_scale(value);
        });

        auto chose_color_map_button = new Button{ hidden_options_popup, "Chose Color Map" };
        chose_color_map_button->set_tooltip("Chose with which color map the data should be displayed (M)");
        chose_color_map_button->set_callback([this]() {
            toggle_color_map_selection_window();
        });

        new Label{ hidden_options_popup, "Grid Options", "sans-bold" };
        auto grid_color_label = new Label{ hidden_options_popup, "Color" };
        grid_color_label->set_tooltip("Chose in witch color the grid should be displayed");
        auto colorwheel = new ColorWheel{ hidden_options_popup, m_bsdf_canvas->grid().color() };

        auto grid_alpha_label = new Label{ hidden_options_popup, "Alpha" };
        grid_alpha_label->set_tooltip("Chose the grid transparency (left = fully transparent, right = fully opaque)");
        auto grid_alpha_slider = new Slider{ hidden_options_popup };
        grid_alpha_slider->set_range({ 0.0f, 1.0f });
        grid_alpha_slider->set_callback([this](float value) { m_bsdf_canvas->grid().set_alpha(value); });

        grid_alpha_slider->set_value(m_bsdf_canvas->grid().alpha());

        colorwheel->set_callback([grid_alpha_slider, this](const Color& value) {
            m_bsdf_canvas->grid().set_color(value);
            m_bsdf_canvas->grid().set_alpha(grid_alpha_slider->value());
        });

        m_display_degrees_checkbox = add_hidden_option_toggle("Grid Degrees", "Show/Hide grid degrees (Shift+G)",
            [this](bool checked) { m_bsdf_canvas->grid().set_show_degrees(checked); }, true);
    }

    // mouse mode
    {
        auto mouse_mode_label = new Label{ m_tool_window, "Mouse Mode", "sans-bold"};
        mouse_mode_label->set_tooltip("Change mouse mode to rotation (R), translation (T) or box selection (B)");
        m_mouse_mode_selector = new ComboBox{ m_tool_window, {"Rotation", "Translation", "Box Selection"} };
        m_mouse_mode_selector->set_callback([this](int index) {
            m_bsdf_canvas->set_mouse_mode(static_cast<BSDFCanvas::Mouse_mode>(index));
            glfwSetCursor(m_glfw_window, m_cursors[index]);
        });

        m_cursors[BSDFCanvas::Mouse_mode::ROTATE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        m_cursors[BSDFCanvas::Mouse_mode::TRANSLATE] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        m_cursors[BSDFCanvas::Mouse_mode::SELECTION] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
        glfwSetCursor(m_glfw_window, m_cursors[m_bsdf_canvas->mouse_mode()]);
    }

    // grid view otpions
    {
        auto label = new Label(m_tool_window, "View Options", "sans-bold");
        label->set_tooltip(
            "Various view modes. Hover on them to learn what they do."
        );

        auto panel = new Widget(m_tool_window);
        panel->set_layout(new GridLayout(Orientation::Horizontal, 3, Alignment::Fill));

        m_grid_view_toggle = new Button(panel, "Grid");
        m_grid_view_toggle->set_flags(Button::Flags::ToggleButton);
        m_grid_view_toggle->set_tooltip("Display/Hide grid (G)");
        m_grid_view_toggle->set_change_callback( [this] (bool checked) { m_bsdf_canvas->grid().set_visible(checked); });
        m_grid_view_toggle->set_pushed(true);

        m_ortho_view_toggle = new Button(panel, "Ortho");
        m_ortho_view_toggle->set_flags(Button::Flags::ToggleButton);
        m_ortho_view_toggle->set_tooltip("Enable/Disable orthogonal projection (O)");
        m_ortho_view_toggle->set_change_callback([this](bool checked) { m_bsdf_canvas->set_ortho_mode(checked); });
        m_ortho_view_toggle->set_pushed(false);

        auto background_color_popup_button = new PopupButton(panel, "", ENTYPO_ICON_BUCKET);

        background_color_popup_button->set_font_size(15);
        background_color_popup_button->set_chevron_icon(0);
        background_color_popup_button->set_tooltip("Background Color");

        // Background color popup
        {
            auto popup = background_color_popup_button->popup();
            popup->set_layout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 10});

            new Label{popup, "Background Color"};
            auto colorwheel = new ColorWheel{popup, m_bsdf_canvas->background_color()};

            colorwheel->set_callback([this](const Color& value) { m_bsdf_canvas->set_background_color(value); });
        }
    }

    // Open, save screenshot, save data
    {
        new Label{ m_tool_window, "Loaded materials", "sans-bold" };
        auto tools = new Widget{ m_tool_window };
        tools->set_layout(new GridLayout{Orientation::Horizontal, 4, Alignment::Fill});

        auto make_tool_button = [&](bool enabled, function<void()> callback, int icon = 0, string tooltip = "") {
            auto button = new Button{tools, "", icon};
            button->set_callback(callback);
            button->set_tooltip(tooltip);
            button->set_font_size(15);
            button->set_enabled(enabled);
            return button;
        };

        auto open_button        = make_tool_button(true, [this] { open_data_sample_dialog(); }, ENTYPO_ICON_FOLDER, "Open data sample (CTRL+O)");
        auto save_image_button  = make_tool_button(true, [this] { save_screen_shot(); }, ENTYPO_ICON_IMAGE, "Save image (CTRL+P)");
        auto save_data_button   = make_tool_button(true, [this] { save_selected_data_sample(); }, ENTYPO_ICON_SAVE, "Save data (CTRL+S)");
        auto show_infos_button  = make_tool_button(true, [this]() { toggle_metadata_window(); }, ENTYPO_ICON_INFO, "Show selected dataset infos (I)");

#if defined(EMSCRIPTEN)
        open_button->set_enabled(false);
        save_image_button->set_enabled(false);
        save_data_button->set_enabled(false);
#endif
    }

    // Data sample selection
    {
        m_data_samples_scroll_panel = new VScrollPanel{ m_tool_window };

        m_scroll_content = new Widget{ m_data_samples_scroll_panel };
        m_scroll_content->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill));

        m_data_sample_button_container = new Widget{ m_scroll_content };
        m_data_sample_button_container->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 0));
    }

#if !defined(EMSCRIPTEN)
    // load application icon
    {
        const vector<pair<const uint8_t*, uint32_t>> icon_paths =
        {
            { tekari_icon_16x16_png, tekari_icon_16x16_png_size },
            { tekari_icon_32x32_png, tekari_icon_32x32_png_size },
            { tekari_icon_64x64_png, tekari_icon_64x64_png_size },
            { tekari_icon_128x128_png, tekari_icon_128x128_png_size },
            { tekari_icon_256x256_png, tekari_icon_256x256_png_size }
        };

        GLFWimage icons[icon_paths.size()];
        size_t i;
        for (i = 0; i < icon_paths.size(); i++)
        {
            int num_chanels;
            icons[i].pixels = stbi_load_from_memory(icon_paths[i].first, icon_paths[i].second,
                                                    &icons[i].width, &icons[i].height, &num_chanels, 0);
            if (!icons[i].pixels)
            {
                cout << "Warning : unable to load Tekari's icons\n";
                break;
            }
        }
        if (i == icon_paths.size())
            glfwSetWindowIcon(m_glfw_window, icon_paths.size(), icons);

        for (size_t j = 0; j < i; j++)
        {
            stbi_image_free(icons[j].pixels);
        }
    }
#endif

    set_resize_callback([this](Vector2i) { request_layout_update(); });
    set_background(m_theme->m_window_fill_focused);

    request_layout_update();
    open_files(data_sample_paths);
}

BSDFApplication::~BSDFApplication()
{
    m_framebuffer.free();

    for (size_t i = 0; i < BSDFCanvas::MOUSE_MODE_COUNT; i++)
    {
        glfwDestroyCursor(m_cursors[i]);
    }
}

bool BSDFApplication::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboard_event(key, scancode, action, modifiers))
        return true;

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        bool alt = modifiers & GLFW_MOD_ALT;
        // control options
        if (modifiers & SYSTEM_COMMAND_MOD)
        {
            switch (key)
            {
#if !defined(EMSCRIPTEN)
            case GLFW_KEY_O:
                open_data_sample_dialog();
                return true;
            case GLFW_KEY_S:
                save_selected_data_sample();
                return true;
            case GLFW_KEY_P:
                save_screen_shot();
                return true;
#endif
            case GLFW_KEY_A:
                if (!m_selected_ds)
                    return false;

                m_selected_ds->select_all_points();
                update_selection_info_window();
                return true;
            case GLFW_KEY_1: if (!alt) return false;
            case GLFW_KEY_KP_1:
                m_bsdf_canvas->set_view_angle(BSDFCanvas::ViewAngles::BACK);
                return true;
            case GLFW_KEY_3: if (!alt) return false;
            case GLFW_KEY_KP_3:
                m_bsdf_canvas->set_view_angle(BSDFCanvas::ViewAngles::RIGHT);
                return true;
            case GLFW_KEY_7: if (!alt) return false;
            case GLFW_KEY_KP_7:
                m_bsdf_canvas->set_view_angle(BSDFCanvas::ViewAngles::DOWN);
                return true;
            case GLFW_KEY_I:
                toggle_canvas_draw_flags(DISPLAY_PREDICTED_OUTGOING_ANGLE, m_display_predicted_outgoing_angle_checkbox);
                return true;
            }
        }
        else if (modifiers & GLFW_MOD_SHIFT)
        {
            switch (key)
            {
                case GLFW_KEY_S:
                    toggle_canvas_draw_flags(USE_SHADOWS, m_use_shadows_checkbox);
                    m_use_specular_checkbox->set_enabled(m_use_shadows_checkbox->checked());
                    return true;
                case GLFW_KEY_G:
                {
                    int show_degrees = !m_bsdf_canvas->grid().show_degrees();
                    m_display_degrees_checkbox->set_checked(show_degrees);
                    m_bsdf_canvas->grid().set_show_degrees(show_degrees);
                    return true;
                }
                case GLFW_KEY_P:
                    toggle_view(DataSample::Views::POINTS, m_selected_ds, !m_selected_ds->display_view(DataSample::Views::POINTS));
                    return true;
                case GLFW_KEY_I:
                    toggle_view(DataSample::Views::INCIDENT_ANGLE, m_selected_ds, !m_selected_ds->display_view(DataSample::Views::INCIDENT_ANGLE));
                    return true;
                case GLFW_KEY_H:
                case GLFW_KEY_L:
                    if (!m_selected_ds)
                        return false;
                    
                    m_selected_ds->select_extreme_point(key == GLFW_KEY_H);
                    update_selection_info_window();
                    return true;
                case GLFW_KEY_1:
                case GLFW_KEY_2:
                    if (!m_selected_ds || !m_selected_ds->has_selection())
                        return false;

                    m_selected_ds->move_selection_along_path(key == GLFW_KEY_1);
                    update_selection_info_window();
                    return true;
                default:
                    return false;
            }
        }
        else if (alt)
        {
            switch (key)
            {
            case GLFW_KEY_1:
                m_bsdf_canvas->set_view_angle(BSDFCanvas::ViewAngles::FRONT);
                return true;
            case GLFW_KEY_3:
                m_bsdf_canvas->set_view_angle(BSDFCanvas::ViewAngles::LEFT);
                return true;
            case GLFW_KEY_7:
                m_bsdf_canvas->set_view_angle(BSDFCanvas::ViewAngles::UP);
                return true;
            case GLFW_KEY_5:
                toggle_tool_button(m_ortho_view_toggle);
                return true;
            }
        }
        else
        {
            switch (key)
            {
            case GLFW_KEY_F1:
                hide_windows();
                return true;
            case GLFW_KEY_ESCAPE:
                if (!m_selected_ds || !m_selected_ds->has_selection())
                    return false;
                 
                m_selected_ds->deselect_all_points();
                update_selection_info_window();
                return true;
#if !defined(EMSCRIPTEN)
            case GLFW_KEY_Q:
            {
                vector<string> ds_names;
                for (const auto& ds : m_data_samples)
                    if (ds->dirty())
                        ds_names.push_back(ds->name());
                
                if (ds_names.empty()) set_visible(false);
                else toggle_unsaved_data_window(ds_names, [this]() { set_visible(false); });
            }
                return true;
#endif
            case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4: case GLFW_KEY_5:
            case GLFW_KEY_6: case GLFW_KEY_7: case GLFW_KEY_8: case GLFW_KEY_9:
                select_data_sample(key - GLFW_KEY_1);
                return true;
            case GLFW_KEY_DELETE:
                delete_data_sample(m_selected_ds);
                return true;
            case GLFW_KEY_D:
                if (!m_selected_ds || !m_selected_ds->has_selection())
                    return false;

                m_selected_ds->delete_selected_points();
                m_selected_ds->set_dirty(true);
                corresponding_button(m_selected_ds)->set_dirty(true);

                reprint_footer();
                update_selection_info_window();
                request_layout_update();
                return true;
            case GLFW_KEY_UP: case GLFW_KEY_W:
                select_data_sample(selected_data_sample_index() - 1, false);
                return true;
            case GLFW_KEY_DOWN: case GLFW_KEY_S:
                select_data_sample(selected_data_sample_index() + 1, false);
                return true;
            case GLFW_KEY_ENTER:
                if (!m_selected_ds)
                    return false;
                corresponding_button(m_selected_ds)->toggle_view();
                return true;
            case GLFW_KEY_L:
                toggle_log_view(m_selected_ds);
                return true;
            case GLFW_KEY_T: case GLFW_KEY_R: case GLFW_KEY_B:
            {
                BSDFCanvas::Mouse_mode mode = BSDFCanvas::Mouse_mode::ROTATE;
                if (key == GLFW_KEY_T) mode = BSDFCanvas::Mouse_mode::TRANSLATE;
                if (key == GLFW_KEY_B) mode = BSDFCanvas::Mouse_mode::SELECTION;
                m_mouse_mode_selector->set_selected_index(mode);
                m_bsdf_canvas->set_mouse_mode(mode);
                glfwSetCursor(m_glfw_window, m_cursors[mode]);
                return true;
            }
            case GLFW_KEY_P:
                toggle_view(DataSample::Views::PATH, m_selected_ds, !m_selected_ds->display_view(DataSample::Views::PATH));
                return true;
            case GLFW_KEY_G:
                toggle_tool_button(m_grid_view_toggle);
                return true;
            case GLFW_KEY_O: case GLFW_KEY_KP_5:
                toggle_tool_button(m_ortho_view_toggle);
                return true;
            case GLFW_KEY_I:
                toggle_metadata_window();
                return true;
            case GLFW_KEY_C:
                m_bsdf_canvas->snap_to_selection_center();
                return true;
            case GLFW_KEY_M:
                toggle_color_map_selection_window();
                return true;
            case GLFW_KEY_A:
                toggle_canvas_draw_flags(DISPLAY_AXIS, m_display_center_axis);
                return true;
            case GLFW_KEY_H:
                toggle_help_window();
                return true;
            case GLFW_KEY_KP_1:
                m_bsdf_canvas->set_view_angle(BSDFCanvas::ViewAngles::FRONT);
                return true;
            case GLFW_KEY_KP_3:
                m_bsdf_canvas->set_view_angle(BSDFCanvas::ViewAngles::LEFT);
                return true;
            case GLFW_KEY_KP_7:
                m_bsdf_canvas->set_view_angle(BSDFCanvas::ViewAngles::UP);
                return true;
            case GLFW_KEY_U:
                toggle_data_sample_sliders_window();
                return true;
            case GLFW_KEY_KP_ADD:
            case GLFW_KEY_KP_SUBTRACT:
                if (!m_selected_ds || !m_selected_ds->has_selection())
                    return false;

                m_selected_ds->move_selection_along_path(key == GLFW_KEY_KP_ADD);
                update_selection_info_window();
                return true;
            default:
                return false;
            }
        }
    }
    return false;
}

void BSDFApplication::draw_contents() {
    if (m_requires_layout_update)
    {
        update_layout();
        m_requires_layout_update = false;
    }

    try {
        while (true) {
            auto new_data_sample = m_data_samples_to_add.try_pop();
            if (!new_data_sample->data_sample)
            {
                auto error_msg_dialog = new MessageDialog(this, MessageDialog::Type::Warning, "Error",
                    new_data_sample->error_msg, "Retry", "Cancel", true);
                error_msg_dialog->set_callback([this](int index) {
                    if (index == 0) { open_data_sample_dialog(); }
                });
            }
            else
            {
                new_data_sample->data_sample->init_shaders();
                new_data_sample->data_sample->link_data_to_shaders();
                new_data_sample->data_sample->set_intensity_index(0);
                add_data_sample(new_data_sample->data_sample);
            }
            redraw();
        }
    }
    catch (std::runtime_error) {
    }
}

void BSDFApplication::update_layout()
{
    m_3d_view->set_fixed_size(m_size);

    m_footer->set_fixed_size(Vector2i( m_size.x(), FOOTER_HEIGHT ));
    for(auto& footer_infos: m_footer->children())
        footer_infos->set_fixed_width(width() / m_footer->children().size());

    m_bsdf_canvas->set_fixed_size(Vector2i{ m_size.x(), m_size.y() - FOOTER_HEIGHT });
    m_tool_window->set_fixed_size({ 210, 600 });

    m_data_samples_scroll_panel->set_fixed_height(
        m_tool_window->height() - m_data_samples_scroll_panel->position().y()
    );

    if (m_data_sample_sliders_window) 
        m_data_sample_sliders_window->set_position({m_size[0] - 250, m_size[1] - 400});

    perform_layout();

    // With a changed layout the relative position of the mouse
    // within children changes and therefore should get updated.
    // nanogui does not handle this for us.
    double x, y;
    glfwGetCursorPos(m_glfw_window,& x,& y);
    cursor_pos_callback_event(x, y);
}

void BSDFApplication::open_data_sample_dialog()
{
    vector<string> data_sample_paths = nanogui::file_dialog(
        {
            { "txt",  "Data samples" },
            { "bsdf",  "Data samples" }
        }, false, true);
    open_files(data_sample_paths);
    // Make sure we gain focus after seleting a file to be loaded.
    glfwFocusWindow(m_glfw_window);
}

void BSDFApplication::open_files(const vector<string>& data_sample_paths)
{
    for (const auto& data_sample_path : data_sample_paths)
    {
        m_thread_pool.add_task([this, data_sample_path]() {
            auto new_data_sample = make_shared<DataSample_to_add>();
            try_load_data_sample(data_sample_path, new_data_sample);
            m_data_samples_to_add.push(new_data_sample);
            redraw();
        });
    }
}

void BSDFApplication::save_selected_data_sample()
{
    if (!m_selected_ds)
        return;
    
    string path = nanogui::file_dialog(
    {
        { "txt",  "Data samples" },
    }, true);

    if (path.empty())
        return;

    m_selected_ds->save(path);
    m_selected_ds->set_dirty(false);
    corresponding_button(m_selected_ds)->set_dirty(false);
}

void BSDFApplication::save_screen_shot()
{
    string screenshot_name = nanogui::file_dialog(
    {
        { "tga", "TGA images" }
    }, true);

    if (screenshot_name.empty())
        return;
        
    if (m_framebuffer.ready())
    {
        m_framebuffer.free();
    }
    int view_port_width = static_cast<int>(m_pixel_ratio* width());
    int view_port_height = static_cast<int>(m_pixel_ratio* height());
    m_framebuffer.init(Vector2i{ view_port_width, view_port_height }, 1);
    glViewport(0, 0, view_port_width, view_port_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    m_framebuffer.bind();
    draw_all();
    m_framebuffer.download_tga(screenshot_name);
    m_framebuffer.release();
}

void BSDFApplication::toggle_window(Window* & window, function<Window*(void)> create_window)
{
    if (window)
    {
        window->dispose();
        window = nullptr;
    }
    else
    {
        window = create_window();
        window->request_focus();
        window->set_visible(!m_distraction_free_mode);
        request_layout_update();
    }
}

void BSDFApplication::toggle_metadata_window()
{
    toggle_window(m_metadata_window, [this]() {
        Window* window;
        if (m_selected_ds)
        {
            window = new MetadataWindow(this, &m_selected_ds->metadata(), [this]() { toggle_metadata_window(); });
        }
        else
        {
            auto error_window = new MessageDialog{ this, MessageDialog::Type::Warning, "Metadata",
                "No data sample selected.", "close" };
            error_window->set_callback([this](int) { m_metadata_window = nullptr; });
            window = error_window;
        }
        window->center();
        return window;
    });
}

void BSDFApplication::toggle_data_sample_sliders_window()
{
    toggle_window(m_data_sample_sliders_window, [this]() -> Window* {
        Window *window = new Window(this, "BRDF Parameters");
        window->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 5, 5));

        Vector2f curr_i_angle = m_selected_ds ? m_selected_ds->incident_angle() : Vector2f(0.0f, 0.0f);
        if (curr_i_angle[1] > 180.0f) curr_i_angle[1] -= 360.0f;

        new Label{ window, "Incident angle", "sans-bold"};
        m_incident_angle_slider = new Slider2D{ window };
        m_incident_angle_slider->set_value(curr_i_angle);
        m_incident_angle_slider->set_callback([this](Vector2f value) {
            m_phi_float_box->set_value(value[0]);
            m_theta_float_box->set_value(value[1]);

            m_selected_ds->set_incident_angle(value);
            update_selection_info_window();
            reprint_footer();
        });
        m_incident_angle_slider->set_range(make_pair(Vector2f(0.0f, -180.0f), Vector2f(80.0f, 180.0f)));
        m_incident_angle_slider->set_fixed_size({ 200, 200 });
        m_incident_angle_slider->set_enabled(m_selected_ds != nullptr);

        auto add_float_box = [window, this](const string& label, float value, function<void(float)> callback) {
            auto float_box_container = new Widget{window};
            float_box_container->set_layout(new GridLayout{Orientation::Horizontal, 2, Alignment::Fill});
            new Label{float_box_container, label};
            auto float_box = new FloatBox<float>{ float_box_container };
            float_box->set_value(value);
            float_box->set_editable(m_selected_ds != nullptr);
            float_box->set_enabled(m_selected_ds != nullptr);
            float_box->set_callback(callback);
            float_box->set_units("°");
            float_box->set_spinnable(true);
            return float_box;
        };
        m_theta_float_box = add_float_box("Theta", curr_i_angle.x(), [this](float value) {
            Vector2f incident_angle = {clamp(value, 0.0f, 80.0f), m_phi_float_box->value()};
            m_incident_angle_slider->set_value(incident_angle);
            m_selected_ds->set_incident_angle(incident_angle); 
            update_selection_info_window();
            reprint_footer();
        });
        m_phi_float_box = add_float_box("Phi", curr_i_angle.y(), [this](float value) {
            Vector2f incident_angle = {m_theta_float_box->value(), clamp(value, -180.0f, 180.0f)};
            m_incident_angle_slider->set_value(incident_angle);
            m_selected_ds->set_incident_angle(incident_angle);
            update_selection_info_window();
            reprint_footer();
        });

        auto add_int_box = [window, this](const string& label, size_t value, function<void(size_t)> callback) {
            auto int_box_container = new Widget{window};
            int_box_container->set_layout(new GridLayout{Orientation::Horizontal, 2, Alignment::Fill});
            new Label{int_box_container, label};
            auto int_box = new IntBox<size_t>{ int_box_container };
            int_box->set_value(value);
            int_box->set_editable(m_selected_ds != nullptr);
            int_box->set_enabled(m_selected_ds != nullptr);
            int_box->set_callback(callback);
            int_box->set_spinnable(true);
            return int_box;
        };

        m_wave_length_int_box = add_int_box("Wavelength", 0, [this](size_t value) {
            value = clamp(value, 0ul, m_selected_ds->intensity_count()-1);
            m_wave_length_slider->set_value(value);
            m_selected_ds->set_intensity_index(value);
            m_wave_length_int_box->set_value(value);
            reprint_footer();
        });

        m_wave_length_slider = new Slider{ window };
        m_wave_length_slider->set_range(make_pair(0, m_selected_ds ? m_selected_ds->n_wave_lengths() : 1));
        m_wave_length_slider->set_callback([this](float value) {
            int int_val = static_cast<int>(round(value));
            m_wave_length_slider->set_value(int_val);
            m_selected_ds->set_intensity_index(int_val);
            m_wave_length_int_box->set_value(int_val);
            reprint_footer();
        });
        m_wave_length_slider->set_enabled(m_selected_ds != nullptr);

        return window;
    });
}

void BSDFApplication::update_selection_info_window()
{
    if(m_selection_info_window) toggle_selection_info_window();
    toggle_selection_info_window();
}

void BSDFApplication::toggle_selection_info_window()
{
    // if we are trying to toggle the selection window without a selection, just return
    if (!m_selection_info_window &&
        (!m_selected_ds || !m_selected_ds->has_selection()))
        return;

    toggle_window(m_selection_info_window, [this]() {

        auto window = new Window{ this, "Selection Info" };
        window->set_layout(new BoxLayout{ Orientation::Vertical, Alignment::Fill, 5, 5 });

        auto labels_container = new Widget{ window };
        labels_container->set_layout(new GridLayout{ Orientation::Horizontal, 2, Alignment::Fill, 0, 5});

        auto make_selection_info_labels = [labels_container](const string& caption, const string& value) {
            new Label{ labels_container, caption, "sans-bold" };
            new Label{ labels_container, value };
        };
        
        make_selection_info_labels("Points In Selection :", to_string(m_selected_ds->selection_points_count()));
        make_selection_info_labels("Minimum Intensity :", to_string(m_selected_ds->selection_min_intensity()));
        make_selection_info_labels("Maximum Intensity :", to_string(m_selected_ds->selection_max_intensity()));
        make_selection_info_labels("Average Intensity :", to_string(m_selected_ds->selection_average_intensity()));

        auto graph = new Graph{ window, "Spectral plot" };
        graph->set_values(vector<float>{0.0f, 0.5f, 0.3f, 0.8f, 0.4f, 0.8f, 1.0f});
        graph->set_foreground_color(Color(0.5f, 0.3f, 0.7f, 0.8f));
        // graph->set_footer("footer");
        // graph->set_header("header");

        window->set_position(Vector2i{width() - 200, 20});
        return window;
    });
}

void BSDFApplication::toggle_unsaved_data_window(const vector<string>& data_sample_names, function<void(void)> continue_callback)
{
    if (data_sample_names.empty())
        return;

    toggle_window(m_unsaved_data_window, [this,& data_sample_names, continue_callback]() {
        std::ostringstream error_msg;
        error_msg << data_sample_names[0];
        for (size_t i = 1; i < data_sample_names.size(); ++i)
            error_msg << " and " << data_sample_names[i];

        error_msg << (data_sample_names.size() == 1 ? " has " : " have ");
        error_msg << "some unsaved changed. Are you sure you want to continue ?";

        auto window = new MessageDialog{ this, MessageDialog::Type::Warning, "Unsaved Changes",
            error_msg.str(), "Cancel", "Continue", true };

        window->set_callback([this, continue_callback](int i) {
            if (i != 0)
                continue_callback();
            m_unsaved_data_window = nullptr;
        });
        window->center();
        return window;
    });
}

void BSDFApplication::toggle_help_window()
{
    toggle_window(m_help_window, [this]() {
        auto window = new HelpWindow(this, [this]() {toggle_help_window(); });
        window->center();
        return window;
    });
}

void BSDFApplication::toggle_color_map_selection_window()
{
    toggle_window(m_color_map_selection_window, [this]() {
        auto window = new ColorMapSelectionWindow{ this, m_color_maps };
        window->set_close_callback([this]() { toggle_color_map_selection_window(); });
        window->set_selection_callback([this](shared_ptr<ColorMap> color_map) { select_color_map(color_map); });
        auto pos = distance(m_color_maps.begin(), find(m_color_maps.begin(), m_color_maps.end(), m_bsdf_canvas->color_map()));
        window->set_selected_button(static_cast<size_t>(pos));
        window->center();
        return dynamic_cast<Window*>(window);
    });
}

void BSDFApplication::select_color_map(shared_ptr<ColorMap> color_map)
{
    m_bsdf_canvas->set_color_map(color_map);
}

int BSDFApplication::data_sample_index(const shared_ptr<const DataSample> data_sample) const
{
    auto pos = static_cast<size_t>(distance(m_data_samples.begin(), find(m_data_samples.begin(), m_data_samples.end(), data_sample)));
    return pos >= m_data_samples.size() ? -1 : static_cast<int>(pos);
}

void BSDFApplication::select_data_sample(int index, bool clamped)
{
    if (m_data_samples.empty())
        return;

    if (clamped)
        index = std::max(0, std::min(static_cast<int>(m_data_samples.size()-1), index));
    else if (index < 0 || index >= static_cast<int>(m_data_samples.size()))
        return;

    select_data_sample(m_data_samples[index]);
}

void BSDFApplication::select_data_sample(shared_ptr<DataSample> data_sample)
{
    // de-select previously selected button
    if (data_sample != m_selected_ds && m_selected_ds)
    {
        DataSampleButton* old_button = corresponding_button(m_selected_ds);
        old_button->set_selected(false);
        old_button->show_popup(false);
    }
    
    m_selected_ds = data_sample;
    m_bsdf_canvas->select_data_sample(data_sample);

    reprint_footer();
    update_selection_info_window();
    if (m_metadata_window)
    {
        toggle_metadata_window();
        toggle_metadata_window();
    }
    if (m_data_sample_sliders_window)
    {
        toggle_data_sample_sliders_window();
        toggle_data_sample_sliders_window();
    }
    request_layout_update();

    if (!m_selected_ds) // if no data sample is selected, we can stop there
        return;
 
    auto button = corresponding_button(m_selected_ds);
    button->set_selected(true);

    // move scroll panel if needed
    int button_abs_y = button->absolute_position()[1];
    int scroll_abs_y = m_data_samples_scroll_panel->absolute_position()[1];
    int button_h = button->height();
    int scroll_h = m_data_samples_scroll_panel->height();

    float scroll = m_data_samples_scroll_panel->scroll();
    if (button_abs_y < scroll_abs_y)
    {
        scroll = static_cast<float>(button->position()[1]) / m_data_sample_button_container->height();
    }
    else if (button_abs_y + button_h > scroll_abs_y + scroll_h)
    {
        scroll = static_cast<float>(button->position()[1]) / (m_data_sample_button_container->height() - button_h);
    }
    m_data_samples_scroll_panel->set_scroll(scroll);
}

void BSDFApplication::delete_data_sample(shared_ptr<DataSample> data_sample)
{
    int index = data_sample_index(data_sample);
    if (index == -1)
        return;

    // erase data sample and corresponding button
    auto button = corresponding_button(data_sample);
    button->remove_popup_from_parent();
    m_data_sample_button_container->remove_child(index);

    m_bsdf_canvas->remove_data_sample(data_sample);
    m_data_samples.erase(find(m_data_samples.begin(), m_data_samples.end(), data_sample));

    // clear focus path and drag widget pointer, since it may refer to deleted button
    m_drag_widget = nullptr;
    m_drag_active = false;
    m_focus_path.clear();

    // update selected datasample, if we just deleted the selected data sample
    if (data_sample == m_selected_ds)
    {
        shared_ptr<DataSample> data_sample_to_select = nullptr;
        if (index >= static_cast<int>(m_data_samples.size())) --index;
        if (index >= 0)
        {
            data_sample_to_select = m_data_samples[index];
        }
        // Make sure no button is selected
        m_selected_ds = nullptr;
        select_data_sample(data_sample_to_select);
    }
    request_layout_update();
}

void BSDFApplication::add_data_sample(shared_ptr<DataSample> data_sample)
{
    if (!data_sample) {
        throw std::invalid_argument{ "Data sample may not be null." };
    }

    string clean_name = data_sample->name();
    replace(clean_name.begin(), clean_name.end(), '_', ' ');
    auto data_sample_button = new DataSampleButton{ m_data_sample_button_container, clean_name,
        data_sample->is_spectral()};
    data_sample_button->set_fixed_height(30);

    data_sample_button->set_callback([this, data_sample]() { select_data_sample(data_sample); });

    data_sample_button->set_delete_callback([this, data_sample]() {
        if (data_sample->dirty())
        {
            toggle_unsaved_data_window({ data_sample->name() }, [this, data_sample]() { delete_data_sample(data_sample); });
        }
        else {
            delete_data_sample(data_sample);
        }
    });

    data_sample_button->set_toggle_view_callback([this, data_sample](bool checked) {
        int index = data_sample_index(data_sample);
        if (checked)    m_bsdf_canvas->add_data_sample(m_data_samples[index]);
        else            m_bsdf_canvas->remove_data_sample(m_data_samples[index]);
    });

    data_sample_button->set_view_toggles_callback([this, data_sample, data_sample_button](bool) {
        for (int i = 0; i != DataSample::Views::VIEW_COUNT; ++i)
        {
            DataSample::Views view = static_cast<DataSample::Views>(i);
            toggle_view(view, data_sample, data_sample_button->is_view_toggled(view));
        }
    });

    data_sample_button->set_display_as_log_callback([data_sample](bool /* unused*/) {
        data_sample->toggle_log_view();
    });

    m_data_samples.push_back(data_sample);
    select_data_sample(data_sample);

    // by default toggle view for the new data samples
    m_bsdf_canvas->add_data_sample(m_selected_ds);
}

void BSDFApplication::toggle_tool_button(Button* button)
{
    button->set_pushed(!button->pushed());
    button->change_callback()(button->pushed());
}

void BSDFApplication::toggle_view(DataSample::Views view, shared_ptr<DataSample> data_sample, bool toggle)
{
    if (data_sample)
    {
        data_sample->toggle_view(view, toggle);
        corresponding_button(data_sample)->toggle_view(view, data_sample->display_view(view));
    }
}

void BSDFApplication::toggle_log_view(shared_ptr<DataSample> data_sample)
{
    if (data_sample)
    {
        data_sample->toggle_log_view();
        corresponding_button(data_sample)->toggle_log_view();
    }
}

DataSampleButton* BSDFApplication::corresponding_button(const shared_ptr<const DataSample> data_sample)
{
    int index = data_sample_index(data_sample);
    if (index == -1)
        return nullptr;
    return dynamic_cast<DataSampleButton*>(m_data_sample_button_container->child_at(index));
}

const DataSampleButton* BSDFApplication::corresponding_button(const shared_ptr<const DataSample> data_sample) const
{
    int index = data_sample_index(data_sample);
    if (index == -1)
        return nullptr;
    return dynamic_cast<DataSampleButton*>(m_data_sample_button_container->child_at(index));
}

void BSDFApplication::toggle_canvas_draw_flags(int flag, CheckBox* checkbox)
{
    int flags = m_bsdf_canvas->draw_flags() ^ flag;
    checkbox->set_checked(static_cast<bool>(flags & flag));
    m_bsdf_canvas->set_draw_flags(flags);
}

void BSDFApplication::reprint_footer()
{
    m_data_sample_name->set_caption             (!m_selected_ds ? "-" : m_selected_ds->name());
    m_data_sample_points_count->set_caption     (!m_selected_ds ? "-" : to_string(m_selected_ds->points_count()));
    m_data_sample_average_height->set_caption   (!m_selected_ds ? "-" : to_string(m_selected_ds->average_intensity()));
}

void BSDFApplication::hide_windows()
{
    m_distraction_free_mode = !m_distraction_free_mode;

    auto toggle_visibility = [this](Window* w) { if (w) w->set_visible(!m_distraction_free_mode); };

    toggle_visibility(m_tool_window);
    toggle_visibility(m_metadata_window);
    toggle_visibility(m_help_window);
    toggle_visibility(m_color_map_selection_window);
    toggle_visibility(m_selection_info_window);
    toggle_visibility(m_unsaved_data_window);
}

void BSDFApplication::try_load_data_sample(const string& file_path, shared_ptr<DataSample_to_add> data_sample_to_add)
{
    try {
        size_t pos = file_path.find_last_of(".");
        string extension = file_path.substr(pos+1, file_path.length());

        shared_ptr<DataSample> ds;
        if (extension == "bsdf")
        {
            ds = make_shared<BSDFDataSample>(file_path);
        }
        else
        {
            ds = make_shared<StandardDataSample>(file_path);
        }
        data_sample_to_add->data_sample = ds;
    }
    catch (const std::exception &e) {
        string error_msg = "Could not open data sample \"" + file_path + "\" : " + std::string(e.what());
        cerr << error_msg << endl;
        data_sample_to_add->error_msg = error_msg;
    }
}

TEKARI_NAMESPACE_END
