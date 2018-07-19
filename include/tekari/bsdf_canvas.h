#pragma once

#include <tekari/common.h>
#include <nanogui/glcanvas.h>
#include <nanogui/widget.h>
#include <nanogui/label.h>
#include <nanogui/window.h>

#include <tekari/axis.h>
#include <tekari/radial_grid.h>
#include <tekari/selections.h>
#include <tekari/data_sample.h>

TEKARI_NAMESPACE_BEGIN

class BSDFCanvas : public nanogui::GLCanvas
{
public:
    // usefull types
    enum ViewAngles
    {
        FRONT, BACK, UP, DOWN, LEFT, RIGHT
    };
    enum Mouse_mode
    {
        ROTATE,
        TRANSLATE,
        SELECTION,
        MOUSE_MODE_COUNT
    };

private:
    // view state constants
    static const Vector3f VIEW_ORIGIN;
    static const Vector3f VIEW_UP;
    static const Vector3f VIEW_RIGHT;
    static const Matrix4f VIEW;

    static const int BUTTON_MAPPINGS[2][BSDFCanvas::MOUSE_MODE_COUNT];

public:
    BSDFCanvas(nanogui::Widget* parent);

    // nanogui specific methods
    virtual bool mouse_motion_event(const Vector2i& p,
                                  const Vector2i& rel,
                                  int button, int modifiers) override;
    virtual bool mouse_button_event(const Vector2i& p, int button, bool down, int modifiers) override;
    virtual bool scroll_event(const Vector2i& p, const Vector2f& rel) override;
    virtual void perform_layout(NVGcontext*) override { m_arcball.set_size(m_size); }
    virtual void draw(NVGcontext* ctx) override;
    virtual void draw_gl() override;

    // data sample addition/removale/selection
    void select_data_sample(std::shared_ptr<DataSample> data_sample);
    void add_data_sample(std::shared_ptr<DataSample> data_sample);
    void remove_data_sample(std::shared_ptr<DataSample> data_sample);

    void snap_to_selection_center();


    void set_ortho_mode(bool ortho_mode) { m_ortho_mode = ortho_mode; }
    void set_view_angle(ViewAngles view_angle);
    void set_selection_callback(function<void(const Matrix4f&, const SelectionBox&,
        const Vector2i&, SelectionMode)> callback) { m_select_callback = callback; }

    // Setters/Getters
    const RadialGrid& grid() const { return m_grid; }
    RadialGrid& grid() { return m_grid; }

    int draw_flags() const { return m_draw_flags; }
    void set_draw_flags(int flags) { m_draw_flags = flags; }
    void set_draw_flag(int flag, bool state) { m_draw_flags = state ? m_draw_flags | flag : m_draw_flags & ~flag; }

    void set_color_map(std::shared_ptr<ColorMap> color_map) { m_color_map = color_map; }
    const std::shared_ptr<const ColorMap> color_map() const { return m_color_map; }

    void set_point_size_scale(float point_size_scale) { m_point_size_scale = point_size_scale; }

    void set_mouse_mode(Mouse_mode mode) { m_mouse_mode = mode; }
    Mouse_mode mouse_mode() const { return m_mouse_mode; }

private:
    SelectionBox get_selection_box() const;
    Matrix4f get_projection_matrix() const;

    inline int rotation_mouse_button(bool dragging)     const { return BUTTON_MAPPINGS[dragging][m_mouse_mode]; }
    inline int translation_mouse_button(bool dragging) const { return BUTTON_MAPPINGS[dragging][(m_mouse_mode + 2) % MOUSE_MODE_COUNT]; }
    inline int selection_mouse_button(bool dragging)     const { return BUTTON_MAPPINGS[dragging][(m_mouse_mode + 1) % MOUSE_MODE_COUNT]; }

    // data samples
    vector<std::shared_ptr<DataSample>>    m_data_samples_to_draw;
    std::shared_ptr<DataSample>            m_selected_data_sample;

    RadialGrid          m_grid;
    nanogui::Arcball    m_arcball;

    // view state
    Vector3f m_translation;
    float m_zoom;
    float m_point_size_scale;
    bool m_ortho_mode;
    Mouse_mode m_mouse_mode;

    // selection
    std::pair<Vector2i, Vector2i> m_selection_region;
    function<void(const Matrix4f&, const SelectionBox&,
        const Vector2i&, SelectionMode)> m_select_callback;

    // global state for sample display
    int m_draw_flags;
    std::shared_ptr<ColorMap> m_color_map;
};

TEKARI_NAMESPACE_END