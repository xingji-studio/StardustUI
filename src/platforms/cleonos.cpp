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

struct WindowState {
    unsigned long long window_id;
    int width;
    int height;
    unsigned int *pixels;
    window_message_proc message_proc;
    bool open;

    WindowState()
        : window_id(0),
          width(0),
          height(0),
          pixels(nullptr),
          message_proc(nullptr),
          open(false) {}
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

int clamp_int(int value, int min_value, int max_value) {
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

void write_pixel(WindowState *state, int x, int y, unsigned int color) {
    if (state == nullptr || state->pixels == nullptr || x < 0 || y < 0 || x >= state->width || y >= state->height) {
        return;
    }
    state->pixels[(y * state->width) + x] = to_cleonos_color(color);
}

void fill_rect(WindowState *state, int x, int y, int width, int height, unsigned int color) {
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

    const unsigned int out_color = to_cleonos_color(color);
    for (int row = top; row < bottom; ++row) {
        unsigned int *dst = state->pixels + (row * state->width);
        for (int col = left; col < right; ++col) {
            dst[col] = out_color;
        }
    }
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

void dispatch_event(WindowState *state, const cleonos_wm_event& event) {
    if (state == nullptr || state->message_proc == nullptr) {
        return;
    }

    if (event.type == CLEONOS_WM_EVENT_MOUSE_MOVE) {
        state->message_proc(kWindowMessageMove, event.arg2, event.arg3);
    } else if (event.type == CLEONOS_WM_EVENT_MOUSE_BUTTON) {
        state->message_proc(kWindowMessageButton, event.arg0, event.arg1);
    } else if (event.type == CLEONOS_WM_EVENT_KEY) {
        state->message_proc(kWindowMessageKey, event.arg0, 0ULL);
    } else if (event.type == CLEONOS_WM_EVENT_FOCUS_LOST) {
        state->message_proc(kWindowMessageFocus, 0ULL, 0ULL);
    } else if (event.type == CLEONOS_WM_EVENT_FOCUS_GAINED) {
        state->message_proc(kWindowMessageFocus, 1ULL, 0ULL);
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

    state->pixels = static_cast<unsigned int *>(calloc(static_cast<unsigned long>(width) * static_cast<unsigned long>(height),
                                                       sizeof(unsigned int)));
    if (state->pixels == nullptr) {
        delete state;
        set_last_error("failed to allocate window pixels");
        return false;
    }

    cleonos_wm_create_req req;
    req.x = 96ULL;
    req.y = 72ULL;
    req.width = static_cast<unsigned long long>(width);
    req.height = static_cast<unsigned long long>(height);
    req.flags = 0ULL;

    const unsigned long long window_id = cleonos_sys_wm_create(&req);
    if (window_id == 0ULL) {
        free(state->pixels);
        delete state;
        set_last_error("cleonos wm_create failed");
        return false;
    }

    state->window_id = window_id;
    state->width = width;
    state->height = height;
    state->open = true;
    *handle = from_state(state);
    if (!g_windows.push_back(state)) {
        (void)cleonos_sys_wm_destroy(window_id);
        free(state->pixels);
        delete state;
        set_last_error("failed to register window state");
        return false;
    }

    (void)title;
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

    remove_state(state);
    if (state->window_id != 0ULL) {
        (void)cleonos_sys_wm_destroy(state->window_id);
    }
    free(state->pixels);
    delete state;
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
