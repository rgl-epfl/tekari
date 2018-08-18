#include <tekari/wavelength_slider.h>

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <tekari/cie1931.h>
#include <tekari_resources.h>

TEKARI_NAMESPACE_BEGIN

#define VERTEX_PER_SEMI_CIRCLE 10
#define LUMINANCE_WIDTH 0.05f
#define SPECTRUM_WIDTH (1.0f - LUMINANCE_WIDTH)

WavelengthSlider::WavelengthSlider(Widget* parent, const float* wavelengths, size_t n_wavelengths)
:   Slider(parent)
,   m_n_wavelengths(n_wavelengths)
,   m_wavelengths(n_wavelengths)
,   m_colors(n_wavelengths)
{
    m_wavelength_slider_shader.init("wavelength_slider", VERTEX_SHADER_STR(wavelength_slider), FRAGMENT_SHADER_STR(wavelength_slider));

    // compute rgb colors
    for(size_t w = 0; w < m_n_wavelengths; ++w)
    {
        float lambda = wavelengths[w];

        Vector3f XYZ = Vector3f{ cie_interp(cie_x, lambda), cie_interp(cie_y, lambda), cie_interp(cie_z, lambda) };
            // * cie_interp(cie_d65, lambda) * (1.f / (CIE_LAMBDA_MAX - CIE_LAMBDA_MIN));

        Color rgb{ 0.0f, 1.0f };
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                rgb[i] += xyz_to_srgb[i][j] * XYZ[j];

        rgb[0] = to_srgb(rgb[0]);// * lambda);
        rgb[1] = to_srgb(rgb[1]);// * lambda);
        rgb[2] = to_srgb(rgb[2]);// * lambda);
        
        rgb = enoki::max(enoki::min(rgb, Color{ 1.0f }), Color{ 0.0f });

        m_colors[w] = rgb;
        m_wavelengths[w] = lambda;
    }

    // Construct colored mesh
    size_t center_vertex_count = 4 * (m_n_wavelengths + 3);
    size_t caps_vertex_count = 2 * (VERTEX_PER_SEMI_CIRCLE + 1);
    vector<Vector2f> vertices(center_vertex_count + caps_vertex_count);
    vector<Color> colors(center_vertex_count + caps_vertex_count);

    // construct central mesh body (rectangle)
    auto add_central_point = [&](size_t col, float x, Color c) {
        size_t i_offset = center_vertex_count / 2;
        vertices[2 * col]                   = Vector2f{ x, 0.0f };
        vertices[2 * col + 1]               = Vector2f{ x, 0.5f };
        vertices[2 * col + i_offset]        = Vector2f{ x, 0.5f };
        vertices[2 * col + i_offset + 1]    = Vector2f{ x, 1.0f };

        colors[2 * col]      = colors[2 * col + i_offset + 1] = c * Color(0.5f, 1.0f);
        colors[2 * col + 1]  = colors[2 * col + i_offset] = c;
    };
    // luminance part (white)
    add_central_point(0, 0.0f, Color{ 1.0f });
    add_central_point(1, LUMINANCE_WIDTH, Color{ 1.0f });
    add_central_point(2, LUMINANCE_WIDTH, Color{ 1.0f });

    // spectral part (spectrum gradient)
    for (size_t i = 0; i < m_n_wavelengths; ++i) {
        float x = LUMINANCE_WIDTH + ( SPECTRUM_WIDTH * i / (m_n_wavelengths-1));
        add_central_point(i + 3, x, m_colors[i]);
    }

    // construct end caps
    size_t index = center_vertex_count;
    auto add_point = [&vertices, &colors, &index](const Vector2f& vertex, const Color& color) {
        vertices[index] = vertex;
        colors[index] = color;
        ++index;   
    };

    // left end cap
    add_point(Vector2f{ 0.0f, 0.5f }, Color{ 1.0f });
    for (size_t i = 0; i < VERTEX_PER_SEMI_CIRCLE; ++i) {
        float angle = static_cast<float>(i * M_PI / (VERTEX_PER_SEMI_CIRCLE - 1) + M_PI*0.5f);
        add_point(Vector2f{ cos(angle), sin(angle) + 1.0f } * 0.5f, Color{ 1.0f } * Color(0.5f, 1.0f));
    }
    // right end cap
    add_point(Vector2f{ 0.0f, 0.5f }, m_colors[m_n_wavelengths-1]);
    for (size_t i = 0; i < VERTEX_PER_SEMI_CIRCLE; ++i) {
        float angle = static_cast<float>(i * M_PI / (VERTEX_PER_SEMI_CIRCLE - 1) - M_PI*0.5f);
        add_point(Vector2f{ cos(angle), sin(angle) + 1.0f } * 0.5f, m_colors[m_n_wavelengths-1] * Color(0.5f, 1.0f));
    }

    // upload shader attibutes
    m_wavelength_slider_shader.bind();
    m_wavelength_slider_shader.upload_attrib("in_pos", (float*)vertices.data(), 2, vertices.size());
    m_wavelength_slider_shader.upload_attrib("in_color", (float*)colors.data(), 4, colors.size());
}

WavelengthSlider::~WavelengthSlider()
{
    m_wavelength_slider_shader.free();
}

Color WavelengthSlider::current_color() const
{
    if (m_value <= LUMINANCE_WIDTH)
        return Color{ 1.0f };

    float value = (m_value - LUMINANCE_WIDTH) / SPECTRUM_WIDTH;
    float step = 1.0f / (m_colors.size() - 1);

    size_t first_index = static_cast<size_t>(value / step);
    size_t second_index = first_index == m_colors.size()-1 ? first_index : first_index + 1;
    float t = (value - first_index * step) / step;

    return m_colors[first_index] * (1-t) + m_colors[second_index] * t;
}


int WavelengthSlider::wavelength_index() const
{
    if (m_value <= LUMINANCE_WIDTH)
        return 0;
    float value = (m_value - LUMINANCE_WIDTH) / SPECTRUM_WIDTH;
    return round(value * m_wavelengths.size());
}

void WavelengthSlider::draw(NVGcontext* ctx)
{
    Widget::draw(ctx);
    nvgEndFrame(ctx); // Flush the Nano_v_g draw stack, not necessary to call nvg_begin_frame afterwards.

    Vector2f center = Vector2f(m_pos) + Vector2f(m_size) * 0.5f;
    float kr = (int) (m_size.y() * 0.6f), kshadow = 3;

    float start_x = kr + kshadow + m_pos.x();
    float width_x = m_size.x() - 2*(kr+kshadow);

    const nanogui::Screen* screen = dynamic_cast<const nanogui::Screen*>(this->window()->parent());
    assert(screen);
    Vector2f screen_size = Vector2f(screen->size());
    float screen_ratio = screen_size.x() / screen_size.y();
    Vector2f scale_factor = Vector2f(width_x, m_size.y() * 0.5f) / screen_size;
    Vector2f position_in_screen = Vector2f(absolute_position()) + Vector2f{ kr + kshadow, m_size.y() * 0.25f };
    Vector2f image_position = position_in_screen / screen_size;

    size_t i_offset = (m_n_wavelengths + 3) * 2;

    m_wavelength_slider_shader.bind();
    m_wavelength_slider_shader.set_uniform("scale", scale_factor);
    m_wavelength_slider_shader.set_uniform("offset", image_position);

    m_wavelength_slider_shader.draw_array(GL_TRIANGLE_STRIP, 0, i_offset);
    m_wavelength_slider_shader.draw_array(GL_TRIANGLE_STRIP, i_offset, i_offset);
    // draw two end caps
    m_wavelength_slider_shader.set_uniform("scale", Vector2f{ scale_factor.y() / screen_ratio, scale_factor.y() });
    m_wavelength_slider_shader.set_uniform("offset", image_position);
    m_wavelength_slider_shader.draw_array(GL_TRIANGLE_FAN, 2 * i_offset, VERTEX_PER_SEMI_CIRCLE + 1);
    m_wavelength_slider_shader.set_uniform("offset", image_position + Vector2f{width_x / screen_size.x(), 0.0f });
    m_wavelength_slider_shader.draw_array(GL_TRIANGLE_FAN, 2 * i_offset + VERTEX_PER_SEMI_CIRCLE + 1, VERTEX_PER_SEMI_CIRCLE + 1);


    Vector2f knob_pos(start_x + (m_value - m_range.first) /
            (m_range.second - m_range.first) * width_x,
            center.y() + 0.5f);

//     NVGpaint bg = nvgBoxGradient(
//         ctx, start_x, center.y() - 3 + 1, width_x, 6, 3, 3,
//         Color(0, m_enabled ? 32 : 10), Color(0, m_enabled ? 128 : 210));

//     nvgBeginPath(ctx);
//     nvgRoundedRect(ctx, start_x, center.y() - 3 + 1, width_x, 6, 2);
//     nvgFillPaint(ctx, bg);
//     nvgFill(ctx);

//     if (m_highlighted_range.second != m_highlighted_range.first) {
//         nvgBeginPath(ctx);
//         nvgRoundedRect(ctx, start_x + m_highlighted_range.first * m_size.x(),
//                        center.y() - kshadow + 1,
//                        width_x *
//                            (m_highlighted_range.second - m_highlighted_range.first),
//                        kshadow * 2, 2);
//         nvgFillColor(ctx, m_highlight_color);
//         nvgFill(ctx);
//     }

    NVGpaint knob_shadow =
        nvgRadialGradient(ctx, knob_pos.x(), knob_pos.y(), kr - kshadow,
                          kr + kshadow, Color(0, 64), m_theme->m_transparent);

    nvgBeginPath(ctx);
    nvgRect(ctx, knob_pos.x() - kr - 5, knob_pos.y() - kr - 5, kr * 2 + 10,
            kr * 2 + 10 + kshadow);
    nvgCircle(ctx, knob_pos.x(), knob_pos.y(), kr);
    nvgPathWinding(ctx, NVG_HOLE);
    nvgFillPaint(ctx, knob_shadow);
    nvgFill(ctx);

    NVGpaint knob = nvgLinearGradient(ctx,
        m_pos.x(), center.y() - kr, m_pos.x(), center.y() + kr,
        m_theme->m_border_light, m_theme->m_border_medium);
    NVGpaint knob_reverse = nvgLinearGradient(ctx,
        m_pos.x(), center.y() - kr, m_pos.x(), center.y() + kr,
        m_theme->m_border_medium,
        m_theme->m_border_light);

    nvgBeginPath(ctx);
    nvgCircle(ctx, knob_pos.x(), knob_pos.y(), kr);
    nvgStrokeColor(ctx, m_theme->m_border_dark);
    nvgFillPaint(ctx, knob);
    nvgStroke(ctx);
    nvgFill(ctx);
    nvgBeginPath(ctx);
    nvgCircle(ctx, knob_pos.x(), knob_pos.y(), kr/2);
    Vector4f curr_color = current_color();
    nvgFillColor(ctx, Color(curr_color.x(), curr_color.y(), curr_color.z(), m_enabled ? 1.0f : 0.4f));
    nvgStrokePaint(ctx, knob_reverse);
    nvgStroke(ctx);
    nvgFill(ctx);
}



TEKARI_NAMESPACE_END