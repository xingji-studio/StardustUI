#include "../../platforms/linux.hpp"
#include "../../includes/vector.hpp"

#include <SDL.h>
#include <SDL_ttf.h>
#include <cstdio>
#include <cstdlib>
#include <time.h>

namespace {
struct DrawCommand {
    enum Type {
        Pixel,
        Rect,
        Text
    };

    Type type;
    int x;
    int y;
    int width;
    int height;
    unsigned int color;
    unsigned int size;
    stardustui::string text;

    DrawCommand() : type(Pixel), x(0), y(0), width(0), height(0), color(0), size(0), text() {}
};

struct FontEntry {
    unsigned int size;
    TTF_Font *font;

    FontEntry() : size(0), font(nullptr) {}
};

struct WindowState {
    SDL_Window *window;
    SDL_Renderer *renderer;
    Uint32 window_id;
    window_message_proc message_proc;
    stardustui::vector<DrawCommand> commands;
    stardustui::vector<FontEntry> fonts;

    WindowState()
        : window(nullptr),
          renderer(nullptr),
          window_id(0),
          message_proc(nullptr),
          commands(),
          fonts() {}
};

stardustui::vector<WindowState*> g_windows;
WindowState g_measurement_state;
char g_last_error[256];
bool g_sdl_ready = false;
bool g_ttf_ready = false;

int max_int(int a, int b)
{
    return a > b ? a : b;
}

void set_last_error(const char *message)
{
    if (message == nullptr) {
        g_last_error[0] = '\0';
        return;
    }

    int index = 0;
    while (message[index] != '\0' && index + 1 < static_cast<int>(sizeof(g_last_error))) {
        g_last_error[index] = message[index];
        ++index;
    }
    g_last_error[index] = '\0';
}

WindowState *to_state(unsigned long long handle)
{
    return reinterpret_cast<WindowState*>(handle);
}

unsigned long long from_state(WindowState *state)
{
    return reinterpret_cast<unsigned long long>(state);
}

bool has_state(WindowState *state)
{
    for (int index = 0; index < g_windows.size(); ++index) {
        if (g_windows[index] == state) {
            return true;
        }
    }

    return false;
}

WindowState *find_state_by_window_id(Uint32 window_id)
{
    for (int index = 0; index < g_windows.size(); ++index) {
        WindowState *state = g_windows[index];
        if (state != nullptr && state->window_id == window_id) {
            return state;
        }
    }

    return nullptr;
}

void remove_state(WindowState *state)
{
    for (int index = 0; index < g_windows.size(); ++index) {
        if (g_windows[index] == state) {
            g_windows[index] = nullptr;
            return;
        }
    }
}

SDL_Color to_sdl_color(unsigned int color)
{
    SDL_Color result{};
    result.r = static_cast<Uint8>((color >> 24) & 0xFF);
    result.g = static_cast<Uint8>((color >> 16) & 0xFF);
    result.b = static_cast<Uint8>((color >> 8) & 0xFF);
    result.a = static_cast<Uint8>(color & 0xFF);
    if (result.a == 0) {
        result.a = 0xFF;
    }
    return result;
}

bool ensure_sdl()
{
    if (!g_sdl_ready) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            set_last_error(SDL_GetError());
            return false;
        }
        g_sdl_ready = true;
    }

    if (!g_ttf_ready) {
        if (TTF_Init() != 0) {
            set_last_error(TTF_GetError());
            return false;
        }
        g_ttf_ready = true;
    }

    SDL_StartTextInput();

    return true;
}

const char *font_paths[] = {
    "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf",
    "/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Regular.ttf",
    "/usr/share/fonts/TTF/JetBrainsMonoNLNerdFont-Regular.ttf",
    "/usr/share/fonts/TTF/Hack-Regular.ttf",
    "/usr/share/fonts/TTF/AgaveNerdFont-Regular.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"
};

constexpr unsigned int kFontDpi = 96;
constexpr unsigned int kPointsPerInch = 72;
constexpr unsigned int kLinuxFontVisualScaleNum = 5;
constexpr unsigned int kLinuxFontVisualScaleDen = 4;

int abs_int(int value)
{
    return value < 0 ? -value : value;
}

TTF_Font *open_font(unsigned int size)
{
    const int point_size = static_cast<int>(size == 0 ? 12 : size);
    const int base_pixel_height = static_cast<int>((point_size * static_cast<int>(kFontDpi) + static_cast<int>(kPointsPerInch / 2)) /
                                                   static_cast<int>(kPointsPerInch));
    const int target_pixel_height =
        static_cast<int>((base_pixel_height * static_cast<int>(kLinuxFontVisualScaleNum) +
                          static_cast<int>(kLinuxFontVisualScaleDen / 2)) /
                         static_cast<int>(kLinuxFontVisualScaleDen));

    for (unsigned int index = 0; index < sizeof(font_paths) / sizeof(font_paths[0]); ++index) {
        int candidate_point_size = point_size;
        TTF_Font *best_font = nullptr;
        int best_diff = 1 << 30;

        for (int iteration = 0; iteration < 5; ++iteration) {
            if (candidate_point_size <= 0) {
                candidate_point_size = 1;
            }

            TTF_Font *font = TTF_OpenFontDPI(font_paths[index], candidate_point_size, kFontDpi, kFontDpi);
            if (font == nullptr) {
                break;
            }

            const int actual_height = TTF_FontHeight(font);
            const int diff = abs_int(actual_height - target_pixel_height);
            if (diff < best_diff) {
                if (best_font != nullptr) {
                    TTF_CloseFont(best_font);
                }
                best_font = font;
                best_diff = diff;
            } else {
                TTF_CloseFont(font);
            }

            if (diff <= 1 || actual_height <= 0) {
                break;
            }

            int next_point_size = candidate_point_size * target_pixel_height / actual_height;
            if (next_point_size == candidate_point_size) {
                next_point_size += (actual_height < target_pixel_height) ? 1 : -1;
            }
            candidate_point_size = next_point_size;
        }

        if (best_font != nullptr) {
            set_last_error(nullptr);
            return best_font;
        }
    }

    set_last_error(TTF_GetError());
    return nullptr;
}

FontEntry *load_font(WindowState *state, unsigned int size)
{
    if (state == nullptr || !ensure_sdl()) {
        return nullptr;
    }

    const unsigned int pixel_size = size == 0 ? 12 : size;
    for (int index = 0; index < state->fonts.size(); ++index) {
        if (state->fonts[index].size == pixel_size) {
            return &state->fonts[index];
        }
    }

    FontEntry entry;
    entry.size = pixel_size;
    entry.font = open_font(pixel_size);
    state->fonts.push_back(entry);

    if (state->fonts.size() == 0) {
        return nullptr;
    }
    return &state->fonts[state->fonts.size() - 1];
}

FontEntry *load_measurement_font(unsigned int size)
{
    return load_font(&g_measurement_state, size);
}

void draw_command(WindowState *state, const DrawCommand& command)
{
    if (state == nullptr || state->renderer == nullptr) {
        return;
    }

    SDL_Color color = to_sdl_color(command.color);

    if (command.type == DrawCommand::Pixel) {
        SDL_SetRenderDrawColor(state->renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawPoint(state->renderer, command.x, command.y);
        return;
    }

    if (command.type == DrawCommand::Rect) {
        SDL_SetRenderDrawColor(state->renderer, color.r, color.g, color.b, color.a);
        SDL_Rect rect{};
        rect.x = command.x;
        rect.y = command.y;
        rect.w = command.width;
        rect.h = command.height;
        SDL_RenderFillRect(state->renderer, &rect);
        return;
    }

    FontEntry *font_entry = load_font(state, command.size);
    if (font_entry == nullptr || font_entry->font == nullptr) {
        log_serial("stardustui: SDL_ttf failed to load font for draw_text\n");
        return;
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(font_entry->font, command.text.c_str(), color);
    if (surface == nullptr) {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(state->renderer, surface);
    if (texture != nullptr) {
        SDL_Rect destination{};
        destination.x = command.x;
        destination.y = command.y;
        destination.w = surface->w;
        destination.h = surface->h;
        SDL_RenderCopy(state->renderer, texture, nullptr, &destination);
        SDL_DestroyTexture(texture);
    }

    SDL_FreeSurface(surface);
}

void redraw(WindowState *state)
{
    if (state == nullptr || state->renderer == nullptr) {
        return;
    }

    SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, 255);
    SDL_RenderClear(state->renderer);

    for (int index = 0; index < state->commands.size(); ++index) {
        draw_command(state, state->commands[index]);
    }

    SDL_RenderPresent(state->renderer);
}

void dispatch_mouse_move(WindowState *state, const SDL_MouseMotionEvent& motion)
{
    if (state == nullptr || state->message_proc == nullptr) {
        return;
    }

    state->message_proc(kWindowMessageMove,
                        static_cast<unsigned long long>(motion.x),
                        static_cast<unsigned long long>(motion.y));
}

void dispatch_mouse_button(WindowState *state, bool pressed, const SDL_MouseButtonEvent& button)
{
    if (state == nullptr || state->message_proc == nullptr || button.button != SDL_BUTTON_LEFT) {
        return;
    }

    state->message_proc(pressed ? kWindowMessageLeftButtonDown : kWindowMessageLeftButtonUp,
                        static_cast<unsigned long long>(button.x),
                        static_cast<unsigned long long>(button.y));
}

void apply_window_outer_size(WindowState *state, int outer_width, int outer_height)
{
    if (state == nullptr || state->window == nullptr) {
        return;
    }

    int top = 0;
    int left = 0;
    int bottom = 0;
    int right = 0;
    if (SDL_GetWindowBordersSize(state->window, &top, &left, &bottom, &right) != 0) {
        return;
    }

    const int client_width = max_int(1, outer_width - left - right);
    const int client_height = max_int(1, outer_height - top - bottom);
    SDL_SetWindowSize(state->window, client_width, client_height);
}
}

bool create_window(char *title, int width, int height, unsigned long long *handle)
{
    if (title == nullptr || handle == nullptr || width <= 0 || height <= 0) {
        set_last_error("invalid window title, size, or handle output");
        return false;
    }

    if (!ensure_sdl()) {
        return false;
    }

    WindowState *state = new WindowState();
    if (state == nullptr) {
        set_last_error("failed to allocate window state");
        return false;
    }

    state->window = SDL_CreateWindow(title,
                                     SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED,
                                     width,
                                     height,
                                     SDL_WINDOW_SHOWN);
    if (state->window == nullptr) {
        set_last_error(SDL_GetError());
        delete state;
        return false;
    }

    apply_window_outer_size(state, width, height);
    state->renderer = SDL_CreateRenderer(state->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (state->renderer == nullptr) {
        state->renderer = SDL_CreateRenderer(state->window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (state->renderer == nullptr) {
        set_last_error(SDL_GetError());
        SDL_DestroyWindow(state->window);
        delete state;
        return false;
    }

    state->window_id = SDL_GetWindowID(state->window);
    *handle = from_state(state);
    g_windows.push_back(state);
    set_last_error(nullptr);
    return true;
}

void print_error(const char *message)
{
    std::fprintf(stderr, "Error: %s\n", message == nullptr ? "Unknown error" : message);
    if (g_last_error[0] != '\0') {
        std::fprintf(stderr, "Linux platform detail: %s\n", g_last_error);
    }
}

void log_serial(const char *message)
{
    if (message != nullptr) {
        std::fputs(message, stderr);
    }
}

void append_debug_log(const char *message)
{
    log_serial(message);
}

void refresh_window(unsigned long long handle)
{
    redraw(to_state(handle));
}

void wait_window()
{
    while (!g_windows.empty()) {
        pump_window_events();
        sleep_ms(16);
    }
}

void set_window_message_processor(unsigned long long handle, window_message_proc proc)
{
    WindowState *state = to_state(handle);
    if (state != nullptr) {
        state->message_proc = proc;
    }
}

void pump_window_events()
{
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            for (int index = 0; index < g_windows.size(); ++index) {
                if (g_windows[index] != nullptr) {
                    delete_window(from_state(g_windows[index]));
                }
            }
            break;
        }

        Uint32 window_id = 0;
        if (event.type == SDL_MOUSEMOTION) {
            window_id = event.motion.windowID;
        } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            window_id = event.button.windowID;
        } else if (event.type == SDL_TEXTINPUT) {
            window_id = event.text.windowID;
        } else if (event.type == SDL_KEYDOWN) {
            window_id = event.key.windowID;
        } else if (event.type == SDL_WINDOWEVENT) {
            window_id = event.window.windowID;
        }

        WindowState *state = find_state_by_window_id(window_id);
        if (state == nullptr) {
            continue;
        }

        if (event.type == SDL_MOUSEMOTION) {
            dispatch_mouse_move(state, event.motion);
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            dispatch_mouse_button(state, true, event.button);
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            dispatch_mouse_button(state, false, event.button);
        } else if (event.type == SDL_TEXTINPUT && state->message_proc != nullptr) {
            const char* text = event.text.text;
            for (int index = 0; text[index] != '\0'; ++index) {
                state->message_proc(kWindowMessageChar, 0, static_cast<unsigned long long>(static_cast<unsigned char>(text[index])));
            }
        } else if (event.type == SDL_KEYDOWN && state->message_proc != nullptr) {
            if (event.key.keysym.sym == SDLK_BACKSPACE) {
                state->message_proc(kWindowMessageSpecialChar, 0, static_cast<unsigned long long>('\b'));
            } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                state->message_proc(kWindowMessageSpecialChar, 0, static_cast<unsigned long long>('\n'));
            }
        } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED) {
            redraw(state);
        } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
            delete_window(from_state(state));
        }
    }
}

bool is_window_open(unsigned long long handle)
{
    WindowState *state = to_state(handle);
    return state != nullptr && has_state(state);
}

bool delete_window(unsigned long long handle)
{
    WindowState *state = to_state(handle);
    if (state == nullptr || !has_state(state)) {
        return false;
    }

    remove_state(state);

    for (int index = 0; index < state->fonts.size(); ++index) {
        if (state->fonts[index].font != nullptr) {
            TTF_CloseFont(state->fonts[index].font);
        }
    }

    if (state->renderer != nullptr) {
        SDL_DestroyRenderer(state->renderer);
    }
    if (state->window != nullptr) {
        SDL_DestroyWindow(state->window);
    }

    delete state;

    bool any_window = false;
    for (int index = 0; index < g_windows.size(); ++index) {
        if (g_windows[index] != nullptr) {
            any_window = true;
            break;
        }
    }

    if (!any_window) {
        for (int index = 0; index < g_measurement_state.fonts.size(); ++index) {
            if (g_measurement_state.fonts[index].font != nullptr) {
                TTF_CloseFont(g_measurement_state.fonts[index].font);
                g_measurement_state.fonts[index].font = nullptr;
            }
        }
        g_measurement_state.fonts.clear();
        SDL_StopTextInput();
        if (g_ttf_ready) {
            TTF_Quit();
            g_ttf_ready = false;
        }
        if (g_sdl_ready) {
            SDL_Quit();
            g_sdl_ready = false;
        }
    }

    return true;
}

void draw_pixel(unsigned long long handle, int x, int y, unsigned int color)
{
    WindowState *state = to_state(handle);
    if (state == nullptr) {
        return;
    }

    DrawCommand command;
    command.type = DrawCommand::Pixel;
    command.x = x;
    command.y = y;
    command.color = color;
    state->commands.push_back(command);
}

void draw_rect(unsigned long long handle, int x, int y, int width, int height, unsigned int color)
{
    WindowState *state = to_state(handle);
    if (state == nullptr || width <= 0 || height <= 0) {
        return;
    }

    DrawCommand command;
    command.type = DrawCommand::Rect;
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.color = color;
    state->commands.push_back(command);
}

void clear_draw_commands(unsigned long long handle)
{
    WindowState *state = to_state(handle);
    if (state != nullptr) {
        state->commands.clear();
    }
}

void draw_text(unsigned long long handle, int x, int y, unsigned int color, unsigned int size, const stardustui::string& text)
{
    WindowState *state = to_state(handle);
    if (state == nullptr) {
        return;
    }

    DrawCommand command;
    command.type = DrawCommand::Text;
    command.x = x;
    command.y = y;
    command.color = color;
    command.size = size;
    command.text = text;
    state->commands.push_back(command);
}

unsigned int calc_text_width(const stardustui::string& text, unsigned int size)
{
    if (!ensure_sdl()) {
        return static_cast<unsigned int>(text.length() * (size == 0 ? 12 : size));
    }

    FontEntry *font_entry = load_measurement_font(size == 0 ? 12 : size);
    if (font_entry == nullptr || font_entry->font == nullptr) {
        log_serial("stardustui: SDL_ttf failed to load font for calc_text_width\n");
        return static_cast<unsigned int>(text.length() * (size == 0 ? 12 : size));
    }

    int width = 0;
    int height = 0;
    TTF_SizeUTF8(font_entry->font, text.c_str(), &width, &height);
    return static_cast<unsigned int>(width);
}

void sleep_ms(unsigned long long ms)
{
    SDL_Delay(static_cast<Uint32>(ms));
}
