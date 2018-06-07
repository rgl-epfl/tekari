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
	:	Theme(ctx)
	{
		mDropShadow = nanogui::Color(0, 128);
		mTransparent = nanogui::Color(255, 0);
		mBorderDark = nanogui::Color(163, 255);
		mBorderLight = nanogui::Color(226, 255);
		mBorderMedium = nanogui::Color(220, 255);
		mTextColor = nanogui::Color(0, 180);
		mDisabledTextColor = nanogui::Color(0, 80);
		mTextColorShadow = nanogui::Color(255, 160);
		mIconColor = mTextColor;

		mButtonGradientTopFocused	= nanogui::Color(207, 255);
		mButtonGradientBotFocused	= nanogui::Color(191, 255);
		mButtonGradientTopUnfocused = nanogui::Color(197, 255);
		mButtonGradientBotUnfocused = nanogui::Color(181, 255);
		mButtonGradientTopPushed	= nanogui::Color(216, 255);
		mButtonGradientBotPushed	= nanogui::Color(224, 255);

		/* Window-related */
		mWindowFillUnfocused = nanogui::Color(255, 230);
		mWindowFillFocused = nanogui::Color(255, 230);
		mWindowTitleUnfocused = nanogui::Color(35, 160);
		mWindowTitleFocused = nanogui::Color(0, 190);

		mWindowHeaderGradientTop = mButtonGradientTopUnfocused;
		mWindowHeaderGradientBot = mButtonGradientBotUnfocused;
		mWindowHeaderSepTop = mBorderLight;
		mWindowHeaderSepBot = mBorderDark;

		mWindowPopup = nanogui::Color(205, 255);
		mWindowPopupTransparent = nanogui::Color(205, 0);
	}
};

TEKARI_NAMESPACE_END
