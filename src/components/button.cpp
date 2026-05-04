#include "../../includes/components/button.hpp"
#include "../../settings.hpp"
#ifdef  XJ380
#include "../../platforms/xj380.hpp"
#endif
#ifdef STARDUSTUI_WINDOWS
#include "../../platforms/windows.hpp"
#endif
#ifdef STARDUSTUI_LINUX
#include "../../platforms/linux.hpp"
#endif

namespace {
int max_int(int a, int b) {
    return a > b ? a : b;
}
}

Button::Button(const stardustui::string& text, int width, int height)
    : text(text) {
    this->set_bounds(0, 0, width, height);
}

Button::Button(const stardustui::string& text, int width, int height, const SytelRules& style)
    : text(text) {
    this->set_bounds(0, 0, width, height);
    this->set_style_rules(style);
}

Button::~Button() = default;

void Button::draw(unsigned long long handle) {
    const Sytel style = this->resolve_style();
    const unsigned int background_color = style.get_background_color(0xE6E6E6FF);
    const unsigned int border_color = style.get_border_color(0x7A7A7AFF);
    const unsigned int border_width = style.get_border_width(1);
    const unsigned int text_color = style.get_color(0x000000FF);
    const unsigned int text_size = style.get_size(16);
    const unsigned int padding = style.get_padding(12);

    draw_rect(handle, static_cast<int>(this->x), static_cast<int>(this->y), this->get_width(), this->get_height(), border_color);

    const int inner_x = static_cast<int>(this->x) + static_cast<int>(border_width);
    const int inner_y = static_cast<int>(this->y) + static_cast<int>(border_width);
    const int inner_width = this->get_width() - static_cast<int>(border_width * 2);
    const int inner_height = this->get_height() - static_cast<int>(border_width * 2);
    if (inner_width > 0 && inner_height > 0) {
        draw_rect(handle, inner_x, inner_y, inner_width, inner_height, background_color);
    }

    const int text_width = static_cast<int>(calc_text_width(this->text, text_size));
    const int text_x = static_cast<int>(this->x) + max_int(static_cast<int>(padding), (this->get_width() - text_width) / 2);
    const int text_y = static_cast<int>(this->y) + max_int(0, (this->get_height() - static_cast<int>(text_size)) / 2);
    draw_text(handle, text_x, text_y, text_color, text_size, this->text);
}

int Button::get_preferred_width() const {
    return this->get_width();
}

int Button::get_preferred_height() const {
    return this->get_height();
}

void Button::set_text(const stardustui::string& text) {
    this->text = text;
    this->request_redraw();
}

const stardustui::string& Button::get_text() const {
    return this->text;
}
