#pragma once

#include <tekari/common.h>
#include <nanogui/glutil.h>
#include <vector>

TEKARI_NAMESPACE_BEGIN

class RadialGrid
{
public:
    RadialGrid();
    ~RadialGrid();

    void draw_gl(
        const Matrix4f& mvp);

    void draw(NVGcontext* ctx,
        const Vector2i& canvas_size,
        const Matrix4f& mvp);

    void set_visible(bool visible) { m_visible = visible; }
    bool visible() const { return m_visible; }

    void set_show_degrees(bool show_degrees) { m_show_degrees = show_degrees; }
    bool show_degrees() const { return m_show_degrees; }

    const nanogui::Color& color() const { return m_color; }
    void set_color(const nanogui::Color& new_color) { m_color = new_color; }

    const float& alpha() const { return m_color[3]; }
    void set_alpha(float alpha) { m_color[3] = alpha; }

private:
    static constexpr size_t CIRCLE_COUNT = 10;
    static constexpr size_t VERTEX_PER_CIRCLE_COUNT = 100;
    static constexpr size_t LINE_COUNT = 18;
    static constexpr size_t VERTEX_PER_LINE_COUNT = 2;

    nanogui::GLShader m_shader;
    nanogui::Color m_color;
    vector<std::pair<string, Vector3f>> m_phi_labels;
    vector<std::pair<string, Vector3f>> m_theta_labels;
    bool m_visible;
    bool m_show_degrees;
};

TEKARI_NAMESPACE_END