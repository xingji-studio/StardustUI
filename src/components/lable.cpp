#include "../../includes/components/lable.hpp"
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
#ifdef STARDUSTUI_CLEONOS
#include "../../platforms/cleonos.hpp"
#endif
Lable::Lable(const stardustui::string& text, unsigned int size, unsigned int color)
    : text(text), size(size), color(color) {}

Lable::Lable(const stardustui::string& text, unsigned int size, const SytelRules& style)
    : text(text), size(size), color(0) {
    this->set_style_rules(style);
}

Lable::~Lable() = default;

bool Lable::contains(int x, int y) const {
    const Sytel style = this->resolve_style();
    const unsigned int resolved_size = style.get_size(this->size);
    const unsigned int width = calc_text_width(this->text, resolved_size);
    const unsigned int height = resolved_size;

    return x >= static_cast<int>(this->x) &&
           y >= static_cast<int>(this->y) &&
           x < static_cast<int>(this->x + width) &&
           y < static_cast<int>(this->y + height);
}

int Lable::get_preferred_width() const {
    const Sytel style = this->resolve_style();
    const unsigned int resolved_size = style.get_size(this->size);
    return static_cast<int>(calc_text_width(this->text, resolved_size));
}

int Lable::get_preferred_height() const {
    const Sytel style = this->resolve_style();
    return static_cast<int>(style.get_size(this->size));
}

void Lable::set_text(const stardustui::string& text) {
    this->text = text;
    this->request_redraw();
}

const stardustui::string& Lable::get_text() const {
    return this->text;
}

void Lable::draw(unsigned long long handle)
{
    const Sytel style = this->resolve_style();
    const unsigned int resolved_color = style.get_color(this->color);
    const unsigned int resolved_size = style.get_size(this->size);

    draw_text(handle, this->x, this->y, resolved_color, resolved_size, this->text);
}
