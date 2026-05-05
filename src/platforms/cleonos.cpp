#include "../../platforms/cleonos.hpp"
#include "../../includes/vector.hpp"

extern "C" {
#include <cleonos_syscall.h>
#include <stdlib.h>
#include <string.h>
}

namespace {
#define SDUI_GLYPH7(r0, r1, r2, r3, r4, r5, r6) \
    (((unsigned long long)(r0) << 30U) | ((unsigned long long)(r1) << 25U) | ((unsigned long long)(r2) << 20U) | \
     ((unsigned long long)(r3) << 15U) | ((unsigned long long)(r4) << 10U) | ((unsigned long long)(r5) << 5U) | \
     (unsigned long long)(r6))

constexpr int kChromeTitleHeight = 32;
constexpr int kChromeControlWidth = 46;
constexpr unsigned int kColorWhite = 0x00FFFFFFU;
constexpr unsigned int kColorTitle = 0x000078D7U;
constexpr unsigned int kColorTitleInactive = 0x00F3F3F3U;
constexpr unsigned int kColorText = 0x00232323U;
constexpr unsigned int kColorBorder = 0x00D0D0D0U;
constexpr unsigned int kColorClose = 0x00E81123U;
constexpr unsigned int kColorControlActive = 0x001A5EA0U;
constexpr unsigned int kColorControlInactive = 0x00E5E5E5U;

struct WindowState {
    unsigned long long window_id;
    int width;
    int height;
    int client_width;
    int client_height;
    int x;
    int y;
    int screen_width;
    int screen_height;
    int drag_dx;
    int drag_dy;
    unsigned int *pixels;
    window_message_proc message_proc;
    bool open;
    bool focused;
    bool dragging;
    char title[96];

    WindowState()
        : window_id(0),
          width(0),
          height(0),
          client_width(0),
          client_height(0),
          x(96),
          y(72),
          screen_width(1280),
          screen_height(800),
          drag_dx(0),
          drag_dy(0),
          pixels(nullptr),
          message_proc(nullptr),
          open(false),
          focused(false),
          dragging(false),
          title{} {}
};

stardustui::vector<WindowState*> g_windows;
char g_last_error[160];

void set_last_error(const char *message) {
    if (message == nullptr) {
        g_last_error[0] = '\0';
        return;
    }

    unsigned long index = 0;
    while (message[index] != '\0' && index + 1 < sizeof(g_last_error)) {
        g_last_error[index] = message[index];
        ++index;
    }
    g_last_error[index] = '\0';
}

void append_last_error(const char *message) {
    if (message == nullptr) {
        return;
    }

    unsigned long pos = 0;
    while (g_last_error[pos] != '\0' && pos + 1 < sizeof(g_last_error)) {
        ++pos;
    }

    unsigned long index = 0;
    while (message[index] != '\0' && pos + 1 < sizeof(g_last_error)) {
        g_last_error[pos++] = message[index++];
    }
    g_last_error[pos] = '\0';
}

void append_u64_dec(unsigned long long value) {
    char buffer[32];
    int pos = 0;

    if (value == 0ULL) {
        append_last_error("0");
        return;
    }

    while (value != 0ULL && pos < static_cast<int>(sizeof(buffer))) {
        buffer[pos++] = static_cast<char>('0' + (value % 10ULL));
        value /= 10ULL;
    }
    while (pos > 0) {
        char ch[2];
        ch[0] = buffer[--pos];
        ch[1] = '\0';
        append_last_error(ch);
    }
}

WindowState *to_state(unsigned long long handle) {
    return reinterpret_cast<WindowState*>(handle);
}

unsigned long long from_state(WindowState *state) {
    return reinterpret_cast<unsigned long long>(state);
}

bool has_state(WindowState *state) {
    for (int index = 0; index < g_windows.size(); ++index) {
        if (g_windows[index] == state) {
            return true;
        }
    }
    return false;
}

void remove_state(WindowState *state) {
    for (int index = 0; index < g_windows.size(); ++index) {
        if (g_windows[index] == state) {
            g_windows[index] = nullptr;
            return;
        }
    }
}

unsigned int to_cleonos_color(unsigned int color) {
    return (color >> 8U) & 0x00FFFFFFU;
}

int u64_as_i32(unsigned long long raw) {
    return static_cast<int>(static_cast<long long>(raw));
}

int clamp_int(int value, int min_value, int max_value) {
    if (max_value < min_value) return min_value;
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

char upper_char(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        return static_cast<char>(ch - ('a' - 'A'));
    }
    return ch;
}

unsigned long long glyph_mask(char ch) {
    switch (upper_char(ch)) {
    case 'A': return SDUI_GLYPH7(14U, 17U, 17U, 31U, 17U, 17U, 17U);
    case 'B': return SDUI_GLYPH7(30U, 17U, 17U, 30U, 17U, 17U, 30U);
    case 'C': return SDUI_GLYPH7(14U, 17U, 16U, 16U, 16U, 17U, 14U);
    case 'D': return SDUI_GLYPH7(30U, 17U, 17U, 17U, 17U, 17U, 30U);
    case 'E': return SDUI_GLYPH7(31U, 16U, 16U, 30U, 16U, 16U, 31U);
    case 'F': return SDUI_GLYPH7(31U, 16U, 16U, 30U, 16U, 16U, 16U);
    case 'G': return SDUI_GLYPH7(14U, 17U, 16U, 23U, 17U, 17U, 15U);
    case 'H': return SDUI_GLYPH7(17U, 17U, 17U, 31U, 17U, 17U, 17U);
    case 'I': return SDUI_GLYPH7(31U, 4U, 4U, 4U, 4U, 4U, 31U);
    case 'J': return SDUI_GLYPH7(1U, 1U, 1U, 1U, 17U, 17U, 14U);
    case 'K': return SDUI_GLYPH7(17U, 18U, 20U, 24U, 20U, 18U, 17U);
    case 'L': return SDUI_GLYPH7(16U, 16U, 16U, 16U, 16U, 16U, 31U);
    case 'M': return SDUI_GLYPH7(17U, 27U, 21U, 21U, 17U, 17U, 17U);
    case 'N': return SDUI_GLYPH7(17U, 25U, 21U, 19U, 17U, 17U, 17U);
    case 'O': return SDUI_GLYPH7(14U, 17U, 17U, 17U, 17U, 17U, 14U);
    case 'P': return SDUI_GLYPH7(30U, 17U, 17U, 30U, 16U, 16U, 16U);
    case 'Q': return SDUI_GLYPH7(14U, 17U, 17U, 17U, 21U, 18U, 13U);
    case 'R': return SDUI_GLYPH7(30U, 17U, 17U, 30U, 20U, 18U, 17U);
    case 'S': return SDUI_GLYPH7(15U, 16U, 16U, 14U, 1U, 1U, 30U);
    case 'T': return SDUI_GLYPH7(31U, 4U, 4U, 4U, 4U, 4U, 4U);
    case 'U': return SDUI_GLYPH7(17U, 17U, 17U, 17U, 17U, 17U, 14U);
    case 'V': return SDUI_GLYPH7(17U, 17U, 17U, 17U, 17U, 10U, 4U);
    case 'W': return SDUI_GLYPH7(17U, 17U, 17U, 21U, 21U, 21U, 10U);
    case 'X': return SDUI_GLYPH7(17U, 17U, 10U, 4U, 10U, 17U, 17U);
    case 'Y': return SDUI_GLYPH7(17U, 17U, 10U, 4U, 4U, 4U, 4U);
    case 'Z': return SDUI_GLYPH7(31U, 1U, 2U, 4U, 8U, 16U, 31U);
    case '0': return SDUI_GLYPH7(14U, 17U, 19U, 21U, 25U, 17U, 14U);
    case '1': return SDUI_GLYPH7(4U, 12U, 4U, 4U, 4U, 4U, 14U);
    case '2': return SDUI_GLYPH7(14U, 17U, 1U, 2U, 4U, 8U, 31U);
    case '3': return SDUI_GLYPH7(30U, 1U, 1U, 14U, 1U, 1U, 30U);
    case '4': return SDUI_GLYPH7(2U, 6U, 10U, 18U, 31U, 2U, 2U);
    case '5': return SDUI_GLYPH7(31U, 16U, 16U, 30U, 1U, 1U, 30U);
    case '6': return SDUI_GLYPH7(14U, 16U, 16U, 30U, 17U, 17U, 14U);
    case '7': return SDUI_GLYPH7(31U, 1U, 2U, 4U, 8U, 8U, 8U);
    case '8': return SDUI_GLYPH7(14U, 17U, 17U, 14U, 17U, 17U, 14U);
    case '9': return SDUI_GLYPH7(14U, 17U, 17U, 15U, 1U, 1U, 14U);
    case '-': return SDUI_GLYPH7(0U, 0U, 0U, 31U, 0U, 0U, 0U);
    case '_': return SDUI_GLYPH7(0U, 0U, 0U, 0U, 0U, 0U, 31U);
    case '.': return SDUI_GLYPH7(0U, 0U, 0U, 0U, 0U, 12U, 12U);
    case ':': return SDUI_GLYPH7(0U, 12U, 12U, 0U, 12U, 12U, 0U);
    case '/': return SDUI_GLYPH7(1U, 1U, 2U, 4U, 8U, 16U, 16U);
    case '\\': return SDUI_GLYPH7(16U, 16U, 8U, 4U, 2U, 1U, 1U);
    case '+': return SDUI_GLYPH7(0U, 4U, 4U, 31U, 4U, 4U, 0U);
    case '=': return SDUI_GLYPH7(0U, 0U, 31U, 0U, 31U, 0U, 0U);
    case '<': return SDUI_GLYPH7(1U, 2U, 4U, 8U, 4U, 2U, 1U);
    case '>': return SDUI_GLYPH7(16U, 8U, 4U, 2U, 4U, 8U, 16U);
    case '[': return SDUI_GLYPH7(14U, 8U, 8U, 8U, 8U, 8U, 14U);
    case ']': return SDUI_GLYPH7(14U, 2U, 2U, 2U, 2U, 2U, 14U);
    case '(': return SDUI_GLYPH7(2U, 4U, 8U, 8U, 8U, 4U, 2U);
    case ')': return SDUI_GLYPH7(8U, 4U, 2U, 2U, 2U, 4U, 8U);
    case '|': return SDUI_GLYPH7(4U, 4U, 4U, 4U, 4U, 4U, 4U);
    case '!': return SDUI_GLYPH7(4U, 4U, 4U, 4U, 4U, 0U, 4U);
    case '?': return SDUI_GLYPH7(14U, 17U, 1U, 2U, 4U, 0U, 4U);
    case '*': return SDUI_GLYPH7(0U, 21U, 14U, 31U, 14U, 21U, 0U);
    case '^': return SDUI_GLYPH7(4U, 10U, 17U, 0U, 0U, 0U, 0U);
    case ',': return SDUI_GLYPH7(0U, 0U, 0U, 0U, 0U, 4U, 8U);
    case ';': return SDUI_GLYPH7(0U, 4U, 4U, 0U, 0U, 4U, 8U);
    case '#': return SDUI_GLYPH7(10U, 31U, 10U, 10U, 31U, 10U, 10U);
    case '$': return SDUI_GLYPH7(4U, 15U, 20U, 14U, 5U, 30U, 4U);
    case '%': return SDUI_GLYPH7(24U, 25U, 2U, 4U, 8U, 19U, 3U);
    case '&': return SDUI_GLYPH7(12U, 18U, 20U, 8U, 21U, 18U, 13U);
    case '@': return SDUI_GLYPH7(14U, 17U, 23U, 21U, 23U, 16U, 15U);
    case '~': return SDUI_GLYPH7(0U, 0U, 8U, 21U, 2U, 0U, 0U);
    case '"': return SDUI_GLYPH7(10U, 10U, 10U, 0U, 0U, 0U, 0U);
    case '\'': return SDUI_GLYPH7(4U, 4U, 8U, 0U, 0U, 0U, 0U);
    default: return 0ULL;
    }
}

void write_pixel_raw(WindowState *state, int x, int y, unsigned int color) {
    if (state == nullptr || state->pixels == nullptr || x < 0 || y < 0 || x >= state->width || y >= state->height) {
        return;
    }
    state->pixels[(y * state->width) + x] = color & 0x00FFFFFFU;
}

void fill_rect_raw(WindowState *state, int x, int y, int width, int height, unsigned int color) {
    if (state == nullptr || state->pixels == nullptr || width <= 0 || height <= 0) {
        return;
    }

    const int left = clamp_int(x, 0, state->width);
    const int top = clamp_int(y, 0, state->height);
    const int right = clamp_int(x + width, 0, state->width);
    const int bottom = clamp_int(y + height, 0, state->height);
    if (left >= right || top >= bottom) {
        return;
    }

    const unsigned int out_color = color & 0x00FFFFFFU;
    for (int row = top; row < bottom; ++row) {
        unsigned int *dst = state->pixels + (row * state->width);
        for (int col = left; col < right; ++col) {
            dst[col] = out_color;
        }
    }
}

void stroke_rect_raw(WindowState *state, int x, int y, int width, int height, unsigned int color) {
    fill_rect_raw(state, x, y, width, 1, color);
    fill_rect_raw(state, x, y + height - 1, width, 1, color);
    fill_rect_raw(state, x, y, 1, height, color);
    fill_rect_raw(state, x + width - 1, y, 1, height, color);
}

void write_pixel(WindowState *state, int x, int y, unsigned int color) {
    write_pixel_raw(state, x, y + kChromeTitleHeight, to_cleonos_color(color));
}

void fill_rect(WindowState *state, int x, int y, int width, int height, unsigned int color) {
    fill_rect_raw(state, x, y + kChromeTitleHeight, width, height, to_cleonos_color(color));
}

int font_scale(unsigned int size) {
    if (size >= 30U) return 4;
    if (size >= 22U) return 3;
    if (size >= 15U) return 2;
    return 1;
}

void draw_char(WindowState *state, int x, int y, char ch, int scale, unsigned int color) {
    const unsigned long long mask = glyph_mask(ch);
    if (mask == 0ULL || scale <= 0) {
        return;
    }

    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            const unsigned int bit = static_cast<unsigned int>((6 - row) * 5 + (4 - col));
            if ((mask & (1ULL << bit)) != 0ULL) {
                fill_rect(state, x + (col * scale), y + (row * scale), scale, scale, color);
            }
        }
    }
}

void draw_char_raw(WindowState *state, int x, int y, char ch, int scale, unsigned int color) {
    const unsigned long long mask = glyph_mask(ch);
    if (mask == 0ULL || scale <= 0) {
        return;
    }

    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            const unsigned int bit = static_cast<unsigned int>((6 - row) * 5 + (4 - col));
            if ((mask & (1ULL << bit)) != 0ULL) {
                fill_rect_raw(state, x + (col * scale), y + (row * scale), scale, scale, color);
            }
        }
    }
}

void draw_text_raw(WindowState *state, int x, int y, const char *text, int scale, unsigned int color, int max_x) {
    if (state == nullptr || text == nullptr || scale <= 0) {
        return;
    }

    int cursor_x = x;
    for (unsigned long index = 0; text[index] != '\0'; ++index) {
        if (max_x > 0 && cursor_x + (5 * scale) > max_x) {
            break;
        }
        if (text[index] != ' ') {
            draw_char_raw(state, cursor_x, y, text[index], scale, color);
        }
        cursor_x += 6 * scale;
    }
}

void draw_control_button(WindowState *state, int x, int kind) {
    const bool is_close = (kind == 2);
    const unsigned int bg = is_close ? kColorClose : (state->focused ? kColorControlActive : kColorControlInactive);
    const unsigned int fg = (is_close || state->focused) ? kColorWhite : kColorText;
    const int cy = kChromeTitleHeight / 2;
    const int cx = x + (kChromeControlWidth / 2);

    fill_rect_raw(state, x, 0, kChromeControlWidth, kChromeTitleHeight, bg);
    if (kind == 0) {
        fill_rect_raw(state, cx - 6, cy + 4, 12, 1, fg);
    } else if (kind == 1) {
        stroke_rect_raw(state, cx - 6, cy - 6, 12, 12, fg);
        fill_rect_raw(state, cx - 6, cy - 6, 12, 2, fg);
    } else {
        for (int i = 0; i < 11; ++i) {
            fill_rect_raw(state, cx - 5 + i, cy - 5 + i, 1, 1, fg);
            fill_rect_raw(state, cx + 5 - i, cy - 5 + i, 1, 1, fg);
        }
    }
}

void draw_chrome(WindowState *state) {
    if (state == nullptr || state->pixels == nullptr) {
        return;
    }

    const unsigned int title_bg = state->focused ? kColorTitle : kColorTitleInactive;
    const unsigned int title_fg = state->focused ? kColorWhite : kColorText;
    fill_rect_raw(state, 0, 0, state->width, kChromeTitleHeight, title_bg);
    fill_rect_raw(state, 0, kChromeTitleHeight, state->width, 1, kColorBorder);
    stroke_rect_raw(state, 0, 0, state->width, state->height, kColorBorder);

    const int title_max_x = state->width - (kChromeControlWidth * 3) - 8;
    draw_text_raw(state, 14, 12, state->title, 1, title_fg, title_max_x);
    draw_control_button(state, state->width - (kChromeControlWidth * 3), 0);
    draw_control_button(state, state->width - (kChromeControlWidth * 2), 1);
    draw_control_button(state, state->width - kChromeControlWidth, 2);
}

void destroy_state(WindowState *state) {
    if (state == nullptr) {
        return;
    }
    remove_state(state);
    if (state->window_id != 0ULL) {
        (void)cleonos_sys_wm_destroy(state->window_id);
        state->window_id = 0ULL;
    }
    free(state->pixels);
    state->pixels = nullptr;
    delete state;
}

void move_window(WindowState *state, int target_x, int target_y) {
    if (state == nullptr || state->window_id == 0ULL) {
        return;
    }

    state->x = clamp_int(target_x, 0, state->screen_width - state->width);
    state->y = clamp_int(target_y, 24, state->screen_height - 40);

    cleonos_wm_move_req req;
    req.window_id = state->window_id;
    req.x = static_cast<unsigned long long>(static_cast<long long>(state->x));
    req.y = static_cast<unsigned long long>(static_cast<long long>(state->y));
    (void)cleonos_sys_wm_move(&req);
}

void present_state(WindowState *state) {
    if (state == nullptr || state->pixels == nullptr || state->window_id == 0ULL) {
        return;
    }

    draw_chrome(state);
    cleonos_wm_present_req req;
    req.window_id = state->window_id;
    req.pixels_ptr = reinterpret_cast<unsigned long long>(state->pixels);
    req.src_width = static_cast<unsigned long long>(state->width);
    req.src_height = static_cast<unsigned long long>(state->height);
    req.src_pitch_bytes = static_cast<unsigned long long>(state->width * 4);
    if (cleonos_sys_wm_present(&req) == 0ULL) {
        state->open = false;
    }
}

void dispatch_event(WindowState *state, const cleonos_wm_event& event) {
    if (state == nullptr) {
        return;
    }

    if (event.type == CLEONOS_WM_EVENT_MOUSE_MOVE) {
        const int local_x = u64_as_i32(event.arg2);
        const int local_y = u64_as_i32(event.arg3);
        if (state->dragging) {
            move_window(state, u64_as_i32(event.arg0) - state->drag_dx, u64_as_i32(event.arg1) - state->drag_dy);
            return;
        }
        if (state->message_proc != nullptr && local_y >= kChromeTitleHeight) {
            state->message_proc(kWindowMessageMove,
                                static_cast<unsigned long long>(local_x),
                                static_cast<unsigned long long>(local_y - kChromeTitleHeight));
        }
    } else if (event.type == CLEONOS_WM_EVENT_MOUSE_BUTTON) {
        const unsigned long long buttons = event.arg0;
        const unsigned long long changed = event.arg1;
        const int local_x = u64_as_i32(event.arg2);
        const int local_y = u64_as_i32(event.arg3);
        const bool left_changed = (changed & 0x1ULL) != 0ULL;
        const bool left_down = (buttons & 0x1ULL) != 0ULL;

        if (left_changed && !left_down) {
            state->dragging = false;
        }
        if (left_changed && left_down && local_y >= 0 && local_y < kChromeTitleHeight) {
            if (local_x >= state->width - kChromeControlWidth) {
                state->open = false;
                return;
            }
            if (local_x < state->width - (kChromeControlWidth * 3)) {
                state->dragging = true;
                state->drag_dx = local_x;
                state->drag_dy = local_y;
            }
            return;
        }
        if (state->message_proc != nullptr && local_y >= kChromeTitleHeight) {
            const unsigned long long client_x = static_cast<unsigned long long>(local_x);
            const unsigned long long client_y = static_cast<unsigned long long>(local_y - kChromeTitleHeight);
            if (left_changed) {
                state->message_proc(left_down ? kWindowMessageLeftButtonDown : kWindowMessageLeftButtonUp,
                                    client_x,
                                    client_y);
            }
        }
    } else if (event.type == CLEONOS_WM_EVENT_KEY) {
        if (state->message_proc != nullptr) {
            const unsigned char ch = static_cast<unsigned char>(event.arg0 & 0xFFULL);
            if (ch == '\b' || ch == 127U) {
                state->message_proc(kWindowMessageSpecialChar, 0ULL, static_cast<unsigned long long>('\b'));
            } else if (ch == '\r' || ch == '\n') {
                state->message_proc(kWindowMessageSpecialChar, 0ULL, static_cast<unsigned long long>('\n'));
            } else if (ch >= 32U) {
                state->message_proc(kWindowMessageChar, 0ULL, static_cast<unsigned long long>(ch));
            }
        }
    } else if (event.type == CLEONOS_WM_EVENT_FOCUS_LOST) {
        state->focused = false;
        state->dragging = false;
        present_state(state);
    } else if (event.type == CLEONOS_WM_EVENT_FOCUS_GAINED) {
        state->focused = true;
        present_state(state);
    }
}
}

bool create_window(char *title, int width, int height, unsigned long long *handle) {
    if (title == nullptr || handle == nullptr || width <= 0 || height <= 0) {
        set_last_error("invalid window title, size, or handle output");
        return false;
    }

    WindowState *state = new WindowState();
    if (state == nullptr) {
        set_last_error("failed to allocate window state");
        return false;
    }

    cleonos_display_info display_info;
    memset(&display_info, 0, sizeof(display_info));
    if (cleonos_sys_display_info(CLEONOS_DISPLAY_TARGET_WM, &display_info) != 0ULL &&
        display_info.logical_width > 0ULL && display_info.logical_height > 0ULL &&
        display_info.logical_width <= 4096ULL && display_info.logical_height <= 4096ULL) {
        state->screen_width = static_cast<int>(display_info.logical_width);
        state->screen_height = static_cast<int>(display_info.logical_height);
    }

    state->client_width = width;
    state->client_height = height;
    state->width = width;
    state->height = height + kChromeTitleHeight;
    state->x = (state->screen_width > state->width) ? ((state->screen_width - state->width) / 2) : 0;
    state->y = (state->screen_height > state->height) ? ((state->screen_height - state->height) / 2) : 24;
    state->focused = true;
    for (int index = 0; title[index] != '\0' && index + 1 < static_cast<int>(sizeof(state->title)); ++index) {
        state->title[index] = title[index];
        state->title[index + 1] = '\0';
    }

    state->pixels = static_cast<unsigned int *>(calloc(static_cast<unsigned long>(state->width) * static_cast<unsigned long>(state->height),
                                                       sizeof(unsigned int)));
    if (state->pixels == nullptr) {
        delete state;
        set_last_error("failed to allocate window pixels");
        return false;
    }

    cleonos_wm_create_req req;
    req.x = static_cast<unsigned long long>(static_cast<long long>(state->x));
    req.y = static_cast<unsigned long long>(static_cast<long long>(state->y));
    req.width = static_cast<unsigned long long>(state->width);
    req.height = static_cast<unsigned long long>(state->height);
    req.flags = 0ULL;

    const unsigned long long window_id = cleonos_sys_wm_create(&req);
    if (window_id == 0ULL) {
        const int fail_screen_width = state->screen_width;
        const int fail_screen_height = state->screen_height;
        free(state->pixels);
        delete state;
        set_last_error("cleonos wm_create failed");
        append_last_error(" req=");
        append_u64_dec(req.width);
        append_last_error("x");
        append_u64_dec(req.height);
        append_last_error(" screen=");
        append_u64_dec(static_cast<unsigned long long>(fail_screen_width));
        append_last_error("x");
        append_u64_dec(static_cast<unsigned long long>(fail_screen_height));
        return false;
    }

    state->window_id = window_id;
    state->open = true;
    (void)cleonos_sys_wm_set_focus(window_id);
    *handle = from_state(state);
    if (!g_windows.push_back(state)) {
        (void)cleonos_sys_wm_destroy(window_id);
        free(state->pixels);
        delete state;
        set_last_error("failed to register window state");
        return false;
    }

    set_last_error(nullptr);
    return true;
}

void print_error(const char *message) {
    if (message == nullptr) {
        message = "unknown StardustUI error";
    }
    (void)cleonos_sys_log_write(message, static_cast<unsigned long long>(strlen(message)));
    (void)cleonos_sys_log_write("\n", 1ULL);
    if (g_last_error[0] != '\0') {
        (void)cleonos_sys_log_write(g_last_error, static_cast<unsigned long long>(strlen(g_last_error)));
        (void)cleonos_sys_log_write("\n", 1ULL);
    }
}

void log_serial(const char *message) {
    if (message != nullptr) {
        (void)cleonos_sys_log_write(message, static_cast<unsigned long long>(strlen(message)));
    }
}

void append_debug_log(const char *message) {
    log_serial(message);
}

void refresh_window(unsigned long long handle) {
    WindowState *state = to_state(handle);
    if (state == nullptr || state->pixels == nullptr || state->window_id == 0ULL) {
        return;
    }

    present_state(state);
}

void wait_window() {
    bool any_open = true;
    while (any_open) {
        any_open = false;
        pump_window_events();
        for (int index = 0; index < g_windows.size(); ++index) {
            if (g_windows[index] != nullptr && g_windows[index]->open) {
                any_open = true;
                break;
            }
        }
        sleep_ms(16ULL);
    }
}

void set_window_message_processor(unsigned long long handle, window_message_proc proc) {
    WindowState *state = to_state(handle);
    if (state != nullptr) {
        state->message_proc = proc;
    }
}

void pump_window_events() {
    for (int index = 0; index < g_windows.size(); ++index) {
        WindowState *state = g_windows[index];
        if (state == nullptr || !state->open || state->window_id == 0ULL) {
            continue;
        }

        for (int budget = 0; budget < 64; ++budget) {
            cleonos_wm_event event;
            if (cleonos_sys_wm_poll_event(state->window_id, &event) == 0ULL) {
                break;
            }
            dispatch_event(state, event);
        }
    }
}

bool is_window_open(unsigned long long handle) {
    WindowState *state = to_state(handle);
    return state != nullptr && has_state(state) && state->open;
}

bool delete_window(unsigned long long handle) {
    WindowState *state = to_state(handle);
    if (state == nullptr || !has_state(state)) {
        return false;
    }

    destroy_state(state);
    return true;
}

void draw_pixel(unsigned long long handle, int x, int y, unsigned int color) {
    write_pixel(to_state(handle), x, y, color);
}

void draw_rect(unsigned long long handle, int x, int y, int width, int height, unsigned int color) {
    fill_rect(to_state(handle), x, y, width, height, color);
}

void clear_draw_commands(unsigned long long handle) {
    WindowState *state = to_state(handle);
    if (state != nullptr && state->pixels != nullptr) {
        memset(state->pixels, 0xFF, static_cast<unsigned long>(state->width) * static_cast<unsigned long>(state->height) * sizeof(unsigned int));
    }
}

void draw_text(unsigned long long handle, int x, int y, unsigned int color, unsigned int size, const stardustui::string& text) {
    WindowState *state = to_state(handle);
    if (state == nullptr) {
        return;
    }

    const int scale = font_scale(size);
    int cursor_x = x;
    for (int index = 0; index < text.length(); ++index) {
        const char ch = text[index];
        if (ch != ' ') {
            draw_char(state, cursor_x, y, ch, scale, color);
        }
        cursor_x += 6 * scale;
    }
}

unsigned int calc_text_width(const stardustui::string& text, unsigned int size) {
    const int scale = font_scale(size);
    return static_cast<unsigned int>(text.length() * 6 * scale);
}

void sleep_ms(unsigned long long ms) {
    (void)cleonos_sys_sleep_ms(ms);
}
