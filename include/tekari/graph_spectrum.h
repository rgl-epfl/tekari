#pragma once

#include <tekari/common.h>
#include <nanogui/graph.h>
#include <tekari_resources.h>

TEKARI_NAMESPACE_BEGIN

class GraphSpectrum: public nanogui::Graph
{
public:
    GraphSpectrum(Widget *parent,
        const vector<Color>& wavelengths_colors,
        const std::string &caption = "Untitled")
    : Graph(parent, caption)
    , m_n_wavelengths(wavelengths_colors.size())
    {
        m_graph_shader.init("wavelength_slider", VERTEX_SHADER_STR(wavelength_slider), FRAGMENT_SHADER_STR(wavelength_slider));

        // Construct colored mesh
        vector<Vector2f> vertices(2 * m_n_wavelengths);
        vector<Color> colors(2 * m_n_wavelengths);

        // construct central mesh body (rectangle)
        auto add_central_point = [&](size_t col, float x, Color c) {
            vertices[2 * col]       = Vector2f{ x, 0.0f };
            vertices[2 * col + 1]   = Vector2f{ x, 1.0f };

            colors[2 * col]      = c * Color(0.8f, 0.5f);
            colors[2 * col + 1]  = c * Color(1.0f, 0.5f);
        };
        // spectral part (spectrum gradient)
        for (size_t i = 0; i < m_n_wavelengths; ++i) {
            float x = float(i) / (m_n_wavelengths-1);
            add_central_point(i, x, wavelengths_colors[i]);
        }

        // upload shader attibutes
        m_graph_shader.bind();
        m_graph_shader.upload_attrib("in_pos", (float*)vertices.data(), 2, vertices.size());
        m_graph_shader.upload_attrib("in_color", (float*)colors.data(), 4, colors.size());

        m_background_color = Color(0.0f);
    }

    // disable set values
    void set_values(const std::vector<float> &values) = delete;

    virtual void draw(NVGcontext* ctx) override
    {
        Widget::draw(ctx);
        nvgEndFrame(ctx); // Flush the Nano_v_g draw stack, not necessary to call nvg_begin_frame afterwards.

        const nanogui::Screen* screen = dynamic_cast<const nanogui::Screen*>(this->window()->parent());
        assert(screen);
        Vector2f screen_size = Vector2f(screen->size());
        Vector2f scale_factor = Vector2f(m_size) / screen_size;
        Vector2f image_position = Vector2f(absolute_position()) / screen_size;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_graph_shader.bind();
        m_graph_shader.set_uniform("scale", scale_factor);
        m_graph_shader.set_uniform("offset", image_position);
        m_graph_shader.draw_array(GL_TRIANGLE_STRIP, 0, m_n_wavelengths*2);
        glDisable(GL_BLEND);

        Graph::draw(ctx);
    }
private:
    nanogui::GLShader m_graph_shader;
    size_t m_n_wavelengths;
};

TEKARI_NAMESPACE_END