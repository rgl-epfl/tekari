#include "tekari/BSDFCanvas.h"

#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <string>

#include "tekari/DataSample.h"

#define MAX_ZOOM 10.0f
#define MIN_ZOOM -MAX_ZOOM

using namespace nanogui;
using namespace std;


TEKARI_NAMESPACE_BEGIN

const Vector3f BSDFCanvas::VIEW_ORIGIN{ 0, 0, 4 };
const Vector3f BSDFCanvas::VIEW_UP{ 0, 1, 0 };
const Vector3f BSDFCanvas::VIEW_RIGHT{ 1, 0, 0 };
const Matrix4f BSDFCanvas::VIEW{ look_at(VIEW_ORIGIN, Vector3f{ 0.0f,0.0f,0.0f }, VIEW_UP) };

const int BSDFCanvas::BUTTON_MAPPINGS[2][BSDFCanvas::MOUSE_MODE_COUNT] =
{
    { GLFW_MOUSE_BUTTON_1, GLFW_MOUSE_BUTTON_2, GLFW_MOUSE_BUTTON_3 },
    { GLFW_MOUSE_BUTTON_2, GLFW_MOUSE_BUTTON_3, GLFW_MOUSE_BUTTON_5 }
};

BSDFCanvas::BSDFCanvas(Widget *parent)
:   GLCanvas(parent)
,   m_translation(0, 0, 0)
,    m_zoom(0)
,   m_point_size_scale(1.0f)
,    m_ortho_mode(false)
,   m_selection_region(make_pair(Vector2i(0,0), Vector2i(0,0)))
,   m_draw_flags(DISPLAY_AXIS | USES_SHADOWS)
,   m_mouse_mode(ROTATE)
{
    m_arcball.set_state(Quaternionf(Eigen::AngleAxisf(static_cast<float>(M_PI / 4.0), Vector3f::UnitX())));
}

bool BSDFCanvas::mouse_motion_event(const Vector2i &p,
                              const Vector2i &rel,
                              int button, int modifiers) {
    if (GLCanvas::mouse_motion_event(p, rel, button, modifiers))
        return true;
    if (!focused())
        return false;
    
    if (button == rotation_mouse_button(true))
    {
        m_arcball.motion(p);
        return true;
    }
    else if (button == selection_mouse_button(true))
    {
        m_selection_region.second = p;
    }
    else if (button == translation_mouse_button(true))
    {
        float move_speed = 0.04f / (m_zoom + MAX_ZOOM + 0.1f);
        Vector3f translation = m_arcball.matrix().block<3,3>(0,0).inverse() * (-rel[0] * move_speed * VIEW_RIGHT + rel[1] * move_speed * VIEW_UP);
        m_translation += translation;
        return true;
    }
    return false;
}

bool BSDFCanvas::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) {
    if (GLCanvas::mouse_button_event(p, button, down, modifiers))
        return true;
    if (!focused() && !down)
        return false;

    // Whenever we click on the canvas, we request focus (no matter the button)
    if (down)
        request_focus();

    if (button == rotation_mouse_button(false))
    {
        m_arcball.button(p, down);
        return true;
    }
    else if (button == selection_mouse_button(false))
    {
        if (!down && m_selected_data_sample)
        {
            Matrix4f model = m_arcball.matrix() * translate(-m_translation);
            Matrix4f proj = get_projection_matrix();

            Matrix4f mvp = proj * VIEW * model;

            SelectionBox selection_box = get_selection_box();
            
            SelectionMode                       mode = SelectionMode::STANDARD;
            if (modifiers & GLFW_MOD_SHIFT)     mode = SelectionMode::ADD;
            else if (modifiers & GLFW_MOD_ALT)  mode = SelectionMode::SUBTRACT;

            m_select_callback(mvp, selection_box, m_size, mode);
            m_selection_region = make_pair(Vector2i(0, 0), Vector2i(0, 0));
        }
        else
        {
            m_selection_region = make_pair(p, p);
        }
        return true;
    }
    return false;
}

bool BSDFCanvas::scroll_event(const Vector2i &p, const Vector2f &rel)
{
    if (!GLCanvas::scroll_event(p, rel))
    {
        m_zoom += rel[1] * 0.2f;
        m_zoom = min(MAX_ZOOM, max(MIN_ZOOM, m_zoom));
    }
    return true;
}

void BSDFCanvas::draw(NVGcontext* ctx)
{
    GLCanvas::draw(ctx);

    Matrix4f model = m_arcball.matrix() * translate(-m_translation);
    Matrix4f proj = get_projection_matrix();

    m_grid.draw(ctx, m_size, model, VIEW, proj);

    // draw selection region
    SelectionBox selection_box = get_selection_box();
    nvg_begin_path(ctx);
    nvg_rect(ctx, selection_box.top_left.x(), selection_box.top_left.y(),
        selection_box.size.x(), selection_box.size.y());
    nvg_stroke_color(ctx, Color(1.0f, 1.0f));
    nvg_stroke(ctx);
    nvg_fill_color(ctx, Color(1.0f, 0.1f));
    nvg_fill(ctx);
}

void BSDFCanvas::draw_gl() {
    Matrix4f model = m_arcball.matrix() * translate(-m_translation);
    Matrix4f proj = get_projection_matrix();

    float point_size_factor = (m_zoom - MIN_ZOOM) / (MAX_ZOOM - MIN_ZOOM);
    gl_point_size(point_size_factor * point_size_factor * m_point_size_scale);
    for (const auto& data_sample: m_data_samples_to_draw)
    {
        data_sample->draw_gl(VIEW_ORIGIN, model, VIEW, proj, m_draw_flags, m_color_map);
    }
    m_grid.draw_gl(model, VIEW, proj);
}

void BSDFCanvas::select_data_sample(shared_ptr<DataSample> data_sample) {
    m_selected_data_sample = data_sample;
}

void BSDFCanvas::add_data_sample(shared_ptr<DataSample> data_sample)
{
    if (find(m_data_samples_to_draw.begin(), m_data_samples_to_draw.end(), data_sample) == m_data_samples_to_draw.end())
    {
        m_data_samples_to_draw.push_back(data_sample);
    }
}
void BSDFCanvas::remove_data_sample(shared_ptr<DataSample> data_sample)
{
    auto data_sample_to_erase = find(m_data_samples_to_draw.begin(), m_data_samples_to_draw.end(), data_sample);
    if (data_sample_to_erase != m_data_samples_to_draw.end())
    {
        m_data_samples_to_draw.erase(data_sample_to_erase);
    }
}

void BSDFCanvas::snap_to_selection_center()
{
    m_translation = !m_selected_data_sample ? Vector3f{ 0.0f, 0.0f, 0.0f } :
                                            m_selected_data_sample->selection_center();
}

void BSDFCanvas::set_view_angle(ViewAngles view_angle)
{
    float dir = 0.0f;
    switch (view_angle)
    {
    case UP:
        dir = (float)M_PI;
    case DOWN:
        m_arcball.set_state(Quaternionf(Eigen::AngleAxisf(-M_PI * 0.5f + dir, Vector3f::UnitX())));
        break;
    case LEFT:
        dir = (float)M_PI;
    case RIGHT:
        m_arcball.set_state(Quaternionf(Eigen::AngleAxisf(- M_PI * 0.5f + dir, Vector3f::Unit_y())));
        break;
    case BACK:
        dir = (float)M_PI;
    case FRONT:
        m_arcball.set_state(Quaternionf(Eigen::AngleAxisf(dir, Vector3f::Unit_y())));
        break;
    }
}

Matrix4f BSDFCanvas::get_projection_matrix() const
{
    float near = 0.01f, far = 100.0f;
    float zoom_factor = (m_zoom + 10.0f) / 20.0f + 0.01f;
    float size_ratio = (float)m_size.x() / (float)m_size.y();
    if (m_ortho_mode)
    {
        zoom_factor = (1.02f - zoom_factor) * 2.0f;
        return ortho(-zoom_factor * size_ratio, zoom_factor * size_ratio,
                     -zoom_factor, zoom_factor,
                     near, far);
    }
    else {
        const float view_angle = 81.0f - zoom_factor * 80.0f;
        float f_h = tan(view_angle / 360.0f * M_PI) * near;
        float f_w = f_h * size_ratio;
        return frustum(-f_w, f_w, -f_h, f_h, near, far);
    }
}

SelectionBox BSDFCanvas::get_selection_box() const
{
    SelectionBox res;
    res.top_left = { min(m_selection_region.first[0], m_selection_region.second[0]),
                    min(m_selection_region.first[1], m_selection_region.second[1]) };
    res.size    = { abs(m_selection_region.first[0] - m_selection_region.second[0]),
                    abs(m_selection_region.first[1] - m_selection_region.second[1]) };
    return res;
}

TEKARI_NAMESPACE_END