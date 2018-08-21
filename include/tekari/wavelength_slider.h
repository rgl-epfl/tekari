#pragma once

#include <tekari/common.h>
#include <nanogui/slider.h>

TEKARI_NAMESPACE_BEGIN

class WavelengthSlider : public nanogui::Slider
{
public: 
    WavelengthSlider(Widget *parent, const VectorXf& wavelengths, const vector<Color>& wavelengths_colors);
    ~WavelengthSlider();

    int wavelength_index() const;

    virtual void draw(NVGcontext* ctx) override;

    // disable set range
    void set_range(std::pair<float, float> range) = delete;

private:

    Color current_color() const;

    nanogui::GLShader m_wavelength_slider_shader;
    VectorXf m_wavelengths;
    vector<Color> m_wavelengths_colors;
};

TEKARI_NAMESPACE_END