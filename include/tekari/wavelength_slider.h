#pragma once

#include <tekari/common.h>
#include <nanogui/slider.h>

TEKARI_NAMESPACE_BEGIN

class WavelengthSlider : public nanogui::Slider
{
public: 
    WavelengthSlider(Widget *parent, const float* wavelengths, size_t n_wavelengths);
    ~WavelengthSlider();

    int wavelength_index() const;

    virtual void draw(NVGcontext* ctx) override;

    // disable set range
    void set_range(std::pair<float, float> range) = delete;

private:

    Color current_color() const;

    nanogui::GLShader m_wavelength_slider_shader;
    size_t m_n_wavelengths;
    vector<float> m_wavelengths;
    vector<Color> m_colors;
};

TEKARI_NAMESPACE_END