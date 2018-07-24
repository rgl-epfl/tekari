#include <tekari/slider_2d.h>

TEKARI_NAMESPACE_BEGIN

Slider2D::Slider2D(Widget *parent)
: Widget(parent)
, m_value(0.0f, 0.0f)
, m_range(Vector2i(0.0f, 0.0f), Vector2i(1.0f, 1.0f))
{
    m_highlight_color = Color(255, 80, 80, 70);
}

Vector2i Slider2D::preferred_size(NVGcontext *ctx) const
{
    return Vector2i(70, 70);
}
bool Slider2D::mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers)
{
	if (!m_enabled)
        return false;

	float kr = std::max(std::min(m_size.x(), m_size.y()) * 0.05f, 3.0f), kshadow = 3;
    Vector2f start = Vector2f(m_pos) + kr + kshadow;
    Vector2f size = Vector2f(m_size) - 2 * (kr + kshadow);

    Vector2f value = (p - start) / size, old_value = m_value;
    value = value * (m_range.second - m_range.first) + m_range.first;
    m_value = enoki::min(enoki::max(value, m_range.first), m_range.second);
    if (m_callback && m_value != old_value)
        m_callback(m_value);
    return true;
}
bool Slider2D::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers)
{
    if (!m_enabled)
        return false;

	float kr = std::max(std::min(m_size.x(), m_size.y()) * 0.05f, 3.0f), kshadow = 3;
    Vector2f start = Vector2f(m_pos) + kr + kshadow;
    Vector2f size = Vector2f(m_size) - 2 * (kr + kshadow);

    Vector2f value = (p - start) / size, old_value = m_value;
    value = value * (m_range.second - m_range.first) + m_range.first;
    m_value = enoki::min(enoki::max(value, m_range.first), m_range.second);
    if (m_callback && m_value != old_value)
        m_callback(m_value);
    if (m_final_callback && !down)
        m_final_callback(m_value);
    return true;
}
void Slider2D::draw(NVGcontext* ctx)
{
	float kr = std::max(std::min(m_size.x(), m_size.y()) * 0.05f, 3.0f), kshadow = 3;;
	Vector2f center = Vector2f(m_pos) + Vector2f(m_size) * 0.5f;

    Vector2f start = Vector2f(m_pos) + kr + kshadow;
    Vector2f size = Vector2f(m_size) - 2 * (kr + kshadow);

    Vector2f knob_pos(start + (m_value - m_range.first) /
            (m_range.second - m_range.first) * size);

    NVGpaint bg = nvgBoxGradient(
        ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), 3, 3,
        Color(0, m_enabled ? 32 : 10), Color(0, m_enabled ? 128 : 210));

    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), 2);
    nvgFillPaint(ctx, bg);
    nvgFill(ctx);

    Vector2f div = size / 10.0f;
    nvgBeginPath(ctx);
    for (int i = 0; i <= 10; ++i)
    {
    	Vector2f offset = start + i * div;
    	nvgMoveTo(ctx, start.x(), offset.y());
    	nvgLineTo(ctx, start.x() + size.x(), offset.y());

    	nvgMoveTo(ctx, offset.x(), start.y());
    	nvgLineTo(ctx, offset.x(), start.y() + size.y());
    }
    nvgStrokeWidth(ctx, 0.5f);
    nvgStrokeColor(ctx, Color(255, 100));
    nvgStroke(ctx);

    NVGpaint knob_shadow =
        nvgRadialGradient(ctx, knob_pos.x(), knob_pos.y(), kr - kshadow,
                          kr + kshadow, Color(0, 64), m_theme->m_transparent);

    nvgBeginPath(ctx);
    nvgRect(ctx, knob_pos.x() - kr - 5, knob_pos.y() - kr - 5, kr * 2 + 10,
            kr * 2 + 10);
    nvgCircle(ctx, knob_pos.x(), knob_pos.y(), kr);
    nvgPathWinding(ctx, NVG_HOLE);
    nvgFillPaint(ctx, knob_shadow);
    nvgFill(ctx);

    NVGpaint knob = nvgLinearGradient(ctx,
        knob_pos.x() - kr, knob_pos.y() - kr, knob_pos.x() + kr, knob_pos.y() + kr,
        m_theme->m_border_light, m_theme->m_border_medium);

    NVGpaint knob_reverse = nvgLinearGradient(ctx,
        knob_pos.x() - kr, knob_pos.y() - kr, knob_pos.x() + kr, knob_pos.y() + kr,
        m_theme->m_border_medium, m_theme->m_border_light);

    nvgBeginPath(ctx);
    nvgCircle(ctx, knob_pos.x(), knob_pos.y(), kr);
    nvgStrokeColor(ctx, m_theme->m_border_dark);
    nvgFillPaint(ctx, knob);
    nvgStroke(ctx);
    nvgFill(ctx);

    nvgBeginPath(ctx);
    nvgCircle(ctx, knob_pos.x(), knob_pos.y(), kr*0.5f);
    nvgFillColor(ctx, Color(150, m_enabled ? 255 : 100));
    nvgStrokePaint(ctx, knob_reverse);
    nvgStroke(ctx);
    nvgFill(ctx);
}

TEKARI_NAMESPACE_END