#pragma once

#include "common.h"

#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/entypo.h>

TEKARI_NAMESPACE_BEGIN

class LightTheme : public nanogui::Theme
{
public:
    LightTheme(NVGcontext *ctx)
    :    Theme(ctx)
    {
        m_drop_shadow = nanogui::Color(0, 128);
        m_transparent = nanogui::Color(255, 0);
        m_border_dark = nanogui::Color(163, 255);
        m_border_light = nanogui::Color(226, 255);
        m_border_medium = nanogui::Color(220, 255);
        m_text_color = nanogui::Color(0, 180);
        m_disabled_text_color = nanogui::Color(0, 80);
        m_text_color_shadow = nanogui::Color(255, 160);
        m_icon_color = m_text_color;

        m_button_gradient_top_focused    = nanogui::Color(207, 255);
        m_button_gradient_bot_focused    = nanogui::Color(191, 255);
        m_button_gradient_top_unfocused = nanogui::Color(197, 255);
        m_button_gradient_bot_unfocused = nanogui::Color(181, 255);
        m_button_gradient_top_pushed    = nanogui::Color(216, 255);
        m_button_gradient_bot_pushed    = nanogui::Color(224, 255);

        /* Window-related */
        m_window_fill_unfocused = nanogui::Color(255, 230);
        m_window_fill_focused = nanogui::Color(255, 230);
        m_window_title_unfocused = nanogui::Color(35, 160);
        m_window_title_focused = nanogui::Color(0, 190);

        m_window_header_gradient_top = m_button_gradient_top_unfocused;
        m_window_header_gradient_bot = m_button_gradient_bot_unfocused;
        m_window_header_sep_top = m_border_light;
        m_window_header_sep_bot = m_border_dark;

        m_window_popup = nanogui::Color(205, 255);
        m_window_popup_transparent = nanogui::Color(205, 0);
    }
};

TEKARI_NAMESPACE_END
