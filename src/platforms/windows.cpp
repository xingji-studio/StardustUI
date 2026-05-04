#include "../../platforms/windows.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

namespace {
const wchar_t kWindowClassName[] = L"StardustUIWindow";

enum DrawCommandType {
    DrawCommandPixel,
    DrawCommandRect,
    DrawCommandText
};

struct DrawCommand {
    DrawCommandType type;
    int x;
    int y;
    int width;
    int height;
    unsigned int color;
    unsigned int size;
    stardustui::string text;

    DrawCommand() : type(DrawCommandPixel), x(0), y(0), width(0), height(0), color(0), size(0), text() {}
};

struct WindowState {
    HWND handle;
    window_message_proc message_proc;
    stardustui::vector<DrawCommand> commands;

    WindowState() : handle(nullptr), message_proc(nullptr), commands() {}
};

stardustui::vector<WindowState*> g_windows;

HWND to_hwnd(unsigned long long handle)
{
    return reinterpret_cast<HWND>(handle);
}

unsigned long long from_hwnd(HWND handle)
{
    return reinterpret_cast<unsigned long long>(handle);
}

HINSTANCE get_instance()
{
    return GetModuleHandleW(nullptr);
}

void to_wide(const char *text, wchar_t *buffer, int buffer_size)
{
    if (buffer == nullptr || buffer_size <= 0) {
        return;
    }

    buffer[0] = L'\0';
    if (text == nullptr) {
        return;
    }

    int written = MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer, buffer_size);
    if (written > 0) {
        return;
    }

    written = MultiByteToWideChar(CP_ACP, 0, text, -1, buffer, buffer_size);
    if (written > 0) {
        return;
    }

    int index = 0;
    while (text[index] != '\0' && index + 1 < buffer_size) {
        buffer[index] = static_cast<unsigned char>(text[index]);
        ++index;
    }
    buffer[index] = L'\0';
}

COLORREF to_colorref(unsigned int color)
{
    unsigned int red = (color >> 24) & 0xFF;
    unsigned int green = (color >> 16) & 0xFF;
    unsigned int blue = (color >> 8) & 0xFF;
    return RGB(red, green, blue);
}

WindowState *find_state(HWND handle)
{
    for (int index = 0; index < g_windows.size(); ++index) {
        WindowState *state = g_windows[index];
        if (state != nullptr && state->handle == handle) {
            return state;
        }
    }

    return nullptr;
}

void remove_state(HWND handle)
{
    for (int index = 0; index < g_windows.size(); ++index) {
        WindowState *state = g_windows[index];
        if (state == nullptr || state->handle != handle) {
            continue;
        }

        delete state;
        g_windows[index] = nullptr;
        return;
    }
}

void render_command(HDC device_context, const DrawCommand& command)
{
    if (command.type == DrawCommandPixel) {
        SetPixel(device_context, command.x, command.y, to_colorref(command.color));
        return;
    }

    if (command.type == DrawCommandRect) {
        RECT rect{};
        rect.left = command.x;
        rect.top = command.y;
        rect.right = command.x + command.width;
        rect.bottom = command.y + command.height;
        HBRUSH brush = CreateSolidBrush(to_colorref(command.color));
        if (brush != nullptr) {
            FillRect(device_context, &rect, brush);
            DeleteObject(brush);
        }
        return;
    }

    wchar_t wide_text[1024];
    to_wide(command.text.c_str(), wide_text, static_cast<int>(sizeof(wide_text) / sizeof(wide_text[0])));

    HFONT font = CreateFontW(
        -static_cast<int>(command.size == 0 ? 12 : command.size),
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");

    HGDIOBJ old_font = nullptr;
    if (font != nullptr) {
        old_font = SelectObject(device_context, font);
    }

    SetBkMode(device_context, TRANSPARENT);
    SetTextColor(device_context, to_colorref(command.color));
    TextOutW(device_context, command.x, command.y, wide_text, lstrlenW(wide_text));

    if (old_font != nullptr) {
        SelectObject(device_context, old_font);
    }
    if (font != nullptr) {
        DeleteObject(font);
    }
}

void render_all_commands(HDC device_context, WindowState *state, const RECT *paint_rect)
{
    if (device_context == nullptr || state == nullptr) {
        return;
    }

    RECT client_rect{};
    GetClientRect(state->handle, &client_rect);

    const int width = client_rect.right - client_rect.left;
    const int height = client_rect.bottom - client_rect.top;
    if (width <= 0 || height <= 0) {
        return;
    }

    HDC memory_dc = CreateCompatibleDC(device_context);
    if (memory_dc == nullptr) {
        for (int index = 0; index < state->commands.size(); ++index) {
            render_command(device_context, state->commands[index]);
        }
        return;
    }

    HBITMAP bitmap = CreateCompatibleBitmap(device_context, width, height);
    if (bitmap == nullptr) {
        DeleteDC(memory_dc);
        for (int index = 0; index < state->commands.size(); ++index) {
            render_command(device_context, state->commands[index]);
        }
        return;
    }

    HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
    HBRUSH background = CreateSolidBrush(RGB(255, 255, 255));
    if (background != nullptr) {
        FillRect(memory_dc, &client_rect, background);
        DeleteObject(background);
    }

    for (int index = 0; index < state->commands.size(); ++index) {
        render_command(memory_dc, state->commands[index]);
    }

    RECT blit_rect = client_rect;
    if (paint_rect != nullptr) {
        blit_rect = *paint_rect;
    }

    const int blit_width = blit_rect.right - blit_rect.left;
    const int blit_height = blit_rect.bottom - blit_rect.top;
    if (blit_width > 0 && blit_height > 0) {
        BitBlt(device_context,
               blit_rect.left,
               blit_rect.top,
               blit_width,
               blit_height,
               memory_dc,
               blit_rect.left,
               blit_rect.top,
               SRCCOPY);
    }

    if (old_bitmap != nullptr) {
        SelectObject(memory_dc, old_bitmap);
    }
    DeleteObject(bitmap);
    DeleteDC(memory_dc);
}

HFONT create_font(unsigned int size)
{
    return CreateFontW(
        -static_cast<int>(size == 0 ? 12 : size),
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC device_context = BeginPaint(hwnd, &paint);
        WindowState *state = find_state(hwnd);
        if (state != nullptr) {
            render_all_commands(device_context, state, &paint.rcPaint);
        }
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE: {
        WindowState *state = find_state(hwnd);
        if (state != nullptr && state->message_proc != nullptr) {
            state->message_proc(
                kWindowMessageMove,
                static_cast<unsigned long long>(GET_X_LPARAM(lparam)),
                static_cast<unsigned long long>(GET_Y_LPARAM(lparam)));
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        WindowState *state = find_state(hwnd);
        if (state != nullptr && state->message_proc != nullptr) {
            SetCapture(hwnd);
            state->message_proc(
                kWindowMessageLeftButtonDown,
                static_cast<unsigned long long>(GET_X_LPARAM(lparam)),
                static_cast<unsigned long long>(GET_Y_LPARAM(lparam)));
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        WindowState *state = find_state(hwnd);
        if (state != nullptr && state->message_proc != nullptr) {
            ReleaseCapture();
            state->message_proc(
                kWindowMessageLeftButtonUp,
                static_cast<unsigned long long>(GET_X_LPARAM(lparam)),
                static_cast<unsigned long long>(GET_Y_LPARAM(lparam)));
        }
        return 0;
    }
    case WM_CHAR: {
        WindowState *state = find_state(hwnd);
        if (state != nullptr && state->message_proc != nullptr) {
            const char ch = static_cast<char>(wparam & 0xFF);
            if (ch == '\b' || ch == '\r' || ch == '\n') {
                state->message_proc(kWindowMessageSpecialChar, 0, static_cast<unsigned long long>(ch == '\r' ? '\n' : ch));
            } else {
                state->message_proc(kWindowMessageChar, 0, static_cast<unsigned long long>(ch));
            }
        }
        return 0;
    }
    case WM_DESTROY:
        remove_state(hwnd);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
}

bool register_window_class()
{
    static bool registered = false;
    if (registered) {
        return true;
    }

    WNDCLASSW window_class{};
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = get_instance();
    window_class.lpszClassName = kWindowClassName;
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.hbrBackground = nullptr;

    ATOM result = RegisterClassW(&window_class);
    registered = result != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    return registered;
}
}

bool create_window(char *title, int width, int height, unsigned long long *handle)
{
    if (title == nullptr || handle == nullptr || width <= 0 || height <= 0) {
        return false;
    }

    if (!register_window_class()) {
        return false;
    }

    wchar_t wide_title[512];
    to_wide(title, wide_title, static_cast<int>(sizeof(wide_title) / sizeof(wide_title[0])));

    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rect{0, 0, width, height};
    AdjustWindowRect(&rect, style, FALSE);

    HWND native_handle = CreateWindowExW(
        0,
        kWindowClassName,
        wide_title,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        get_instance(),
        nullptr);

    if (native_handle == nullptr) {
        return false;
    }

    *handle = from_hwnd(native_handle);
    WindowState *state = new WindowState();
    if (state != nullptr) {
        state->handle = native_handle;
        g_windows.push_back(state);
    }
    ShowWindow(native_handle, SW_SHOW);
    UpdateWindow(native_handle);
    return true;
}

void print_error(const char *message)
{
    MessageBoxA(nullptr, message == nullptr ? "Unknown error" : message, "StardustUI", MB_ICONERROR | MB_OK);
}

void log_serial(const char *message)
{
    if (message != nullptr) {
        OutputDebugStringA(message);
    }
}

void append_debug_log(const char *message)
{
    log_serial(message);
}

void refresh_window(unsigned long long handle)
{
    HWND window = to_hwnd(handle);
    if (window != nullptr) {
        InvalidateRect(window, nullptr, FALSE);
        UpdateWindow(window);
    }
}

void set_window_message_processor(unsigned long long handle, window_message_proc proc)
{
    HWND window = to_hwnd(handle);
    WindowState *state = find_state(window);
    if (state != nullptr) {
        state->message_proc = proc;
    }
}

void wait_window()
{
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

void pump_window_events()
{
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            break;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

bool is_window_open(unsigned long long handle)
{
    HWND window = to_hwnd(handle);
    return window != nullptr && IsWindow(window) != 0;
}

bool delete_window(unsigned long long handle)
{
    HWND window = to_hwnd(handle);
    if (window == nullptr || !IsWindow(window)) {
        return false;
    }

    return DestroyWindow(window) != 0;
}

void draw_pixel(unsigned long long handle, int x, int y, unsigned int color)
{
    HWND window = to_hwnd(handle);
    if (window == nullptr) {
        return;
    }

    DrawCommand command;
    command.type = DrawCommandPixel;
    command.x = x;
    command.y = y;
    command.color = color;
    WindowState *state = find_state(window);
    if (state != nullptr) {
        state->commands.push_back(command);
    }
}

void draw_rect(unsigned long long handle, int x, int y, int width, int height, unsigned int color)
{
    HWND window = to_hwnd(handle);
    if (window == nullptr || width <= 0 || height <= 0) {
        return;
    }

    DrawCommand command;
    command.type = DrawCommandRect;
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.color = color;

    WindowState *state = find_state(window);
    if (state != nullptr) {
        state->commands.push_back(command);
    }
}

void clear_draw_commands(unsigned long long handle)
{
    HWND window = to_hwnd(handle);
    WindowState *state = find_state(window);
    if (state != nullptr) {
        state->commands.clear();
    }
}

void draw_text(unsigned long long handle, int x, int y, unsigned int color, unsigned int size, const stardustui::string& text)
{
    HWND window = to_hwnd(handle);
    if (window == nullptr) {
        return;
    }

    DrawCommand command;
    command.type = DrawCommandText;
    command.x = x;
    command.y = y;
    command.color = color;
    command.size = size;
    command.text = text;
    WindowState *state = find_state(window);
    if (state != nullptr) {
        state->commands.push_back(command);
    }
}

unsigned int calc_text_width(const stardustui::string& text, unsigned int size)
{
    wchar_t wide_text[1024];
    to_wide(text.c_str(), wide_text, static_cast<int>(sizeof(wide_text) / sizeof(wide_text[0])));

    HDC device_context = GetDC(nullptr);
    if (device_context == nullptr) {
        return static_cast<unsigned int>(text.length() * (size == 0 ? 12 : size));
    }

    HFONT font = create_font(size);
    HGDIOBJ old_font = nullptr;
    if (font != nullptr) {
        old_font = SelectObject(device_context, font);
    }

    SIZE text_size{};
    GetTextExtentPoint32W(device_context, wide_text, lstrlenW(wide_text), &text_size);

    if (old_font != nullptr) {
        SelectObject(device_context, old_font);
    }
    if (font != nullptr) {
        DeleteObject(font);
    }
    ReleaseDC(nullptr, device_context);
    return static_cast<unsigned int>(text_size.cx);
}

void sleep_ms(unsigned long long ms)
{
    Sleep(static_cast<DWORD>(ms));
}
