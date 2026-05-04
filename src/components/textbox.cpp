#include "../../includes/components/textbox.hpp"
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

int clamp_int(int value, int minimum, int maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

}

TextBox::TextBox(int width, int height, bool input)
    : text(),
      scratch_text(),
      input_enabled(input),
      cursor_visible(input),
      cursor_blink_ticks(0),
      scroll_offset_lines(0),
      layout_dirty(true),
      ensure_cursor_visible_pending(false),
      cached_char_width_size(0),
      cached_char_width_valid(),
      cached_char_widths(),
      scrollbar(12, height) {
    for (int index = 0; index < 256; ++index) {
        this->cached_char_width_valid[index] = false;
        this->cached_char_widths[index] = 0;
    }
    this->set_bounds(0, 0, width, height);
}

TextBox::TextBox(int width, int height, bool input, const SytelRules& style)
    : text(),
      scratch_text(),
      input_enabled(input),
      cursor_visible(input),
      cursor_blink_ticks(0),
      scroll_offset_lines(0),
      layout_dirty(true),
      ensure_cursor_visible_pending(false),
      cached_char_width_size(0),
      cached_char_width_valid(),
      cached_char_widths(),
      scrollbar(12, height) {
    for (int index = 0; index < 256; ++index) {
        this->cached_char_width_valid[index] = false;
        this->cached_char_widths[index] = 0;
    }
    this->set_bounds(0, 0, width, height);
    this->set_style_rules(style);
}

TextBox::~TextBox() = default;

void TextBox::draw(unsigned long long handle) {
    this->update_layout();
    const Sytel style = this->resolve_style();
    const unsigned int background_color = style.get_background_color(0xFFFFFFFF);
    const unsigned int border_color = style.get_border_color(this->has_focus() ? 0x4A90E2FF : 0x7A7A7AFF);
    const unsigned int border_width = style.get_border_width(1);
    const unsigned int text_color = style.get_color(0x000000FF);
    const unsigned int text_size = style.get_size(16);
    const unsigned int padding = style.get_padding(8);
    const int line_height = this->get_line_height(text_size);

    draw_rect(handle, static_cast<int>(this->x), static_cast<int>(this->y), this->get_width(), this->get_height(), border_color);

    const int inner_x = static_cast<int>(this->x) + static_cast<int>(border_width);
    const int inner_y = static_cast<int>(this->y) + static_cast<int>(border_width);
    const int inner_width = this->get_width() - static_cast<int>(border_width * 2);
    const int inner_height = this->get_height() - static_cast<int>(border_width * 2);
    if (inner_width > 0 && inner_height > 0) {
        draw_rect(handle, inner_x, inner_y, inner_width, inner_height, background_color);
    }

    const int text_x = static_cast<int>(this->x) + static_cast<int>(border_width + padding);
    const int text_y = static_cast<int>(this->y) + static_cast<int>(border_width + padding);
    const int content_height = inner_height - static_cast<int>(padding * 2);
    const int visible_lines = max_int(1, content_height / max_int(1, line_height));

    for (int line_index = 0; line_index < visible_lines; ++line_index) {
        const int wrapped_index = this->scroll_offset_lines + line_index;
        if (wrapped_index < 0 || wrapped_index >= this->wrapped_lines.size()) {
            break;
        }

        const WrappedLine& line = this->wrapped_lines[wrapped_index];
        if (line.length <= 0) {
            continue;
        }

        this->scratch_text = this->build_text_slice(line.start, line.length);
        draw_text(handle,
                  text_x,
                  text_y + line_index * line_height,
                  text_color,
                  text_size,
                  this->scratch_text);
    }

    if (this->input_enabled && this->has_focus() && this->cursor_visible) {
        int cursor_line_index = this->wrapped_lines.size() - 1;
        if (cursor_line_index < 0) {
            cursor_line_index = 0;
        }

        if (cursor_line_index >= this->scroll_offset_lines &&
            cursor_line_index < this->scroll_offset_lines + visible_lines &&
            this->wrapped_lines.size() > 0) {
            const WrappedLine& cursor_line = this->wrapped_lines[cursor_line_index];
            const int relative_line = cursor_line_index - this->scroll_offset_lines;
            this->scratch_text = this->build_text_slice(cursor_line.start, cursor_line.length);
            const int cursor_x = text_x + static_cast<int>(calc_text_width(this->scratch_text, text_size)) + 1;
            const int cursor_y = text_y + relative_line * line_height;
            draw_rect(handle, cursor_x, cursor_y, 1, max_int(1, line_height - 2), text_color);
        }
    }

    if (this->wrapped_lines.size() > visible_lines) {
        this->scrollbar.draw(handle);
    }
}

void TextBox::update() {
    base_component::update();
    this->scrollbar.update();
    if (this->scrollbar.consume_redraw_request()) {
        this->request_redraw();
    }

    if (!this->input_enabled || !this->has_focus()) {
        if (this->cursor_visible) {
            this->cursor_visible = false;
            this->request_redraw();
        }
        return;
    }

#ifdef XJ380
    if (!this->cursor_visible) {
        this->cursor_visible = true;
        this->request_redraw();
    }
    this->cursor_blink_ticks = 0;
    return;
#endif

    ++this->cursor_blink_ticks;
    static constexpr int kCursorBlinkThreshold = 30;
    if (this->cursor_blink_ticks >= kCursorBlinkThreshold) {
        this->cursor_blink_ticks = 0;
        this->cursor_visible = !this->cursor_visible;
        this->request_redraw();
    }
}

int TextBox::get_preferred_width() const {
    return this->get_width();
}

int TextBox::get_preferred_height() const {
    return this->get_height();
}

void TextBox::set_bounds(int x, int y, int width, int height) {
    base_component::set_bounds(x, y, width, height);
    this->invalidate_layout();
}

bool TextBox::handle_pointer_move(int x, int y) {
    this->update_layout();

    const bool hovered = this->contains(x, y);
    bool changed = this->hover_active != hovered || this->mouse_active != hovered;
    this->set_hover_state(hovered);
    this->set_mouse_state(hovered);

    const int previous_scroll = this->scroll_offset_lines;
    if (this->scrollbar.handle_pointer_move(x, y)) {
        this->scroll_offset_lines = this->scrollbar.get_value();
        changed = true;
    } else if (previous_scroll != this->scrollbar.get_value()) {
        this->scroll_offset_lines = this->scrollbar.get_value();
        changed = true;
    }

    return changed;
}

bool TextBox::handle_left_button(bool pressed, int x, int y) {
    this->update_layout();

    const bool inside = this->contains(x, y);
    bool changed = false;

    if (pressed) {
        changed = this->set_focus(inside);
        this->click_feedback_frames = 0;
        this->set_click_state(inside);
        if (inside) {
            this->set_mouse_state(true);
            this->set_hover_state(true);
        }
    } else {
        changed = this->click_active;
        this->set_click_state(false);
    }

    const int previous_scroll = this->scroll_offset_lines;
    if (this->scrollbar.handle_left_button(pressed, x, y)) {
        this->scroll_offset_lines = this->scrollbar.get_value();
        changed = true;
    } else if (previous_scroll != this->scrollbar.get_value()) {
        this->scroll_offset_lines = this->scrollbar.get_value();
        changed = true;
    }

    return changed;
}

bool TextBox::handle_char_input(char ch, bool special) {
    if (!this->input_enabled || !this->has_focus()) {
        return false;
    }

    if (special) {
        if (ch == '\b') {
            if (!this->erase_last_character()) {
                return false;
            }
            this->invalidate_layout();
            this->ensure_cursor_visible_pending = true;
            this->reset_cursor_blink();
            this->request_redraw();
            return true;
        }
        if (ch == '\n' || ch == '\r') {
            if (!this->text.push_char('\n')) {
                return false;
            }
            this->invalidate_layout();
            this->ensure_cursor_visible_pending = true;
            this->reset_cursor_blink();
            this->request_redraw();
            return true;
        }
        return false;
    }

    if (ch < 32 || ch == 127) {
        return false;
    }

    if (this->text.push_char(ch)) {
        this->invalidate_layout();
        this->ensure_cursor_visible_pending = true;
        this->reset_cursor_blink();
        this->request_redraw();
        return true;
    }

    return false;
}

bool TextBox::set_focus(bool focused) {
    const bool changed = base_component::set_focus(focused);
    if (!changed) {
        return false;
    }

    if (focused && this->input_enabled) {
        this->cursor_visible = true;
        this->cursor_blink_ticks = 0;
    } else {
        this->cursor_visible = false;
    }
    this->request_redraw();
    return true;
}

void TextBox::set_text(const stardustui::string& text) {
    stardustui::string next_text;
    next_text.append(text.c_str());
    this->text = next_text;
    this->invalidate_layout();
    this->ensure_cursor_visible_pending = true;
    this->reset_cursor_blink();
    this->request_redraw();
}

const stardustui::string& TextBox::get_text() const {
    return this->text;
}

void TextBox::set_input_enabled(bool enabled) {
    if (this->input_enabled == enabled) {
        return;
    }

    this->input_enabled = enabled;
    if (!enabled) {
        this->cursor_visible = false;
    } else if (this->has_focus()) {
        this->cursor_visible = true;
    }
    this->cursor_blink_ticks = 0;
    this->request_redraw();
}

bool TextBox::is_input_enabled() const {
    return this->input_enabled;
}

void TextBox::reset_cursor_blink() {
    this->cursor_blink_ticks = 0;
    this->cursor_visible = this->input_enabled && this->has_focus();
}

void TextBox::invalidate_layout() {
    this->layout_dirty = true;
}

int TextBox::get_line_height(unsigned int text_size) const {
    const int height = static_cast<int>(text_size) + static_cast<int>(text_size / 2);
    return max_int(18, height);
}

int TextBox::get_scrollbar_width() const {
    return 12;
}

void TextBox::rebuild_wrapped_lines(unsigned int text_size, int content_width) {
    this->wrapped_lines.clear();

    const char* raw = this->text.c_str();
    const int length = this->text.length();
    if (length <= 0) {
        WrappedLine line;
        this->wrapped_lines.push_back(line);
        return;
    }

    if (content_width <= 0) {
        content_width = 1;
    }

    int line_start = 0;
    int line_length = 0;
#ifdef XJ380
    const int content_limit = max_int(1, content_width - 2);
    int line_width = 0;
#else
    stardustui::string current_line;
#endif

    for (int index = 0; index < length; ++index) {
        const char ch = raw[index];
        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            WrappedLine line;
            line.start = line_start;
            line.length = line_length;
            this->wrapped_lines.push_back(line);

            line_start = index + 1;
            line_length = 0;
#ifdef XJ380
            line_width = 0;
#else
            current_line.assign("");
#endif
            continue;
        }

#ifdef XJ380
        const int char_width = this->get_cached_char_width(text_size, static_cast<unsigned char>(ch));
        if (line_length > 0 && line_width + char_width > content_limit) {
            WrappedLine line;
            line.start = line_start;
            line.length = line_length;
            this->wrapped_lines.push_back(line);

            line_start = index;
            line_length = 1;
            line_width = char_width;
            continue;
        }

        line_width += char_width;
#else
        current_line.push_char(ch);
        if (line_length > 0 && static_cast<int>(calc_text_width(current_line, text_size)) > content_width) {
            WrappedLine line;
            line.start = line_start;
            line.length = line_length;
            this->wrapped_lines.push_back(line);

            line_start = index;
            line_length = 1;
            current_line.assign("");
            current_line.push_char(ch);
            continue;
        }
#endif

        ++line_length;
    }

    WrappedLine line;
    line.start = line_start;
    line.length = line_length;
    this->wrapped_lines.push_back(line);

    if (raw[length - 1] == '\n') {
        WrappedLine empty_line;
        empty_line.start = length;
        empty_line.length = 0;
        this->wrapped_lines.push_back(empty_line);
    }
}

void TextBox::update_layout() {
    const Sytel style = this->resolve_style();
    const unsigned int border_width = style.get_border_width(1);
    const unsigned int text_size = style.get_size(16);
    const unsigned int padding = style.get_padding(8);

    const int inner_width = this->get_width() - static_cast<int>(border_width * 2);
    const int inner_height = this->get_height() - static_cast<int>(border_width * 2);
    const int content_height = max_int(1, inner_height - static_cast<int>(padding * 2));
    const int line_height = this->get_line_height(text_size);
    const int visible_lines = max_int(1, content_height / max_int(1, line_height));
    const int scrollbar_width = this->get_scrollbar_width();
    const int content_width = max_int(1, inner_width - static_cast<int>(padding * 2) - scrollbar_width - 4);

    if (this->layout_dirty) {
        this->rebuild_wrapped_lines(text_size, content_width);
        this->layout_dirty = false;
    }

    const int scrollbar_x = static_cast<int>(this->x) + this->get_width() - static_cast<int>(border_width + padding) - scrollbar_width;
    const int scrollbar_y = static_cast<int>(this->y) + static_cast<int>(border_width + padding);
    this->scrollbar.set_bounds(scrollbar_x, scrollbar_y, scrollbar_width, content_height);
    this->sync_scroll_offset(visible_lines);
    this->scrollbar.set_range(this->wrapped_lines.size(), visible_lines);
    this->scrollbar.set_value(this->scroll_offset_lines);
    this->scroll_offset_lines = this->scrollbar.get_value();
}

void TextBox::sync_scroll_offset(int visible_lines) {
    int target_scroll = this->scroll_offset_lines;
    const int max_scroll = max_int(0, this->wrapped_lines.size() - visible_lines);
    if (this->ensure_cursor_visible_pending && this->wrapped_lines.size() > 0) {
        target_scroll = this->wrapped_lines.size() - visible_lines;
    }
    this->scroll_offset_lines = clamp_int(target_scroll, 0, max_scroll);
    this->ensure_cursor_visible_pending = false;
}

bool TextBox::erase_last_character() {
    const int length = this->text.length();
    if (length <= 0) {
        return false;
    }

    stardustui::string next_text;
    const char* current = this->text.c_str();
    for (int index = 0; index < length - 1; ++index) {
        if (!next_text.push_char(current[index])) {
            return false;
        }
    }

    this->text = next_text;
    return true;
}

stardustui::string TextBox::build_text_slice(int start, int length) const {
    stardustui::string result;
    if (length <= 0) {
        return result;
    }

    const char* raw = this->text.c_str();
    for (int index = 0; index < length; ++index) {
        result.push_char(raw[start + index]);
    }
    return result;
}

int TextBox::get_cached_char_width(unsigned int text_size, unsigned char ch) {
    if (this->cached_char_width_size != text_size) {
        this->cached_char_width_size = text_size;
        for (int index = 0; index < 256; ++index) {
            this->cached_char_width_valid[index] = false;
            this->cached_char_widths[index] = 0;
        }
    }

    if (this->cached_char_width_valid[ch]) {
        return static_cast<int>(this->cached_char_widths[ch]);
    }

    stardustui::string single_char_text;
    single_char_text.push_char(static_cast<char>(ch));
    unsigned int width = calc_text_width(single_char_text, text_size);
    if (width == 0) {
        width = text_size == 0 ? 1U : text_size / 2;
    }

    this->cached_char_width_valid[ch] = true;
    this->cached_char_widths[ch] = width;
    return static_cast<int>(width);
}
