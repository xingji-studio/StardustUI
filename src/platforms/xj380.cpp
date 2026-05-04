#include "../../platforms/xj380.hpp"
#include "../../includes/string.hpp"
#include "../../platforms/xj380/xapi/xguiapi.h"
#include "../../platforms/xj380/xapi/xtuiapi.h"
#include "../../platforms/xj380/xapi/liballoc/alloc.h"

using namespace stardustui;

using operator_size_t = decltype(sizeof(0));
namespace {
char kDebugLogPath[] = "/system/stardustui.log";
char kDebugLogBuffer[4096];
unsigned long long kDebugLogLength = 0;
window_message_proc g_window_message_proc = nullptr;

void dispatch_xj380_message(unsigned long long type, unsigned long long h_data, unsigned long long l_data)
{
    if (g_window_message_proc == nullptr) {
        return;
    }

    if (type == MSG_MOVE) {
        g_window_message_proc(kWindowMessageMove, h_data, l_data);
        return;
    }

    if (type == MSG_LBUTTON) {
        g_window_message_proc(kWindowMessageLeftButtonClick, h_data, l_data);
        return;
    }

    if (type == MSG_CHAR) {
        g_window_message_proc(kWindowMessageChar, 0, l_data);
        return;
    }

    if (type == MSG_SPCHAR) {
        g_window_message_proc(kWindowMessageSpecialChar, 0, l_data);
    }
}
}

void *operator new(operator_size_t size)
{
    return malloc(size);
}

void *operator new[](operator_size_t size)
{
    return malloc(size);
}

void operator delete(void *ptr) noexcept
{
    free(ptr);
}

void operator delete[](void *ptr) noexcept
{
    free(ptr);
}

void operator delete(void *ptr, operator_size_t) noexcept
{
    free(ptr);
}

void operator delete[](void *ptr, operator_size_t) noexcept
{
    free(ptr);
}

bool create_window(char *title, int width, int height, unsigned long long *handle)
{
    if (title == nullptr || handle == nullptr || width <= 0 || height <= 0) return false;

    XWINDOW xwin{};
    xwin.width  = width;
    xwin.height = height;
    xwin.title  = title;
    xwin.sets   = XWIN_NORMAL;

    HDLE native_handle{};
    xapi_CreateWindow(&native_handle, &xwin);
    *handle = native_handle;
    return true;
}

void print_error(const char *message)
{
    static char kErrorFormat[] = "Error: %s";
    xapi_Printf(kErrorFormat, message);
}

void log_serial(const char *message)
{
    xapi_OutputSerial((char *)message);
}

void append_debug_log(const char *message)
{
    if (message == nullptr) return;

    unsigned long long index = 0;
    while (message[index] != '\0' && kDebugLogLength + index + 1 < sizeof(kDebugLogBuffer)) {
        kDebugLogBuffer[kDebugLogLength + index] = message[index];
        ++index;
    }

    if (index == 0) return;

    kDebugLogLength += index;
    kDebugLogBuffer[kDebugLogLength] = '\0';
    xapi_CreateFile(kDebugLogPath);
    xapi_WriteFile(kDebugLogPath, kDebugLogBuffer, kDebugLogLength, 0);
    log_serial(message);
}

void refresh_window(unsigned long long handle)
{
    xapi_RefreshWindow(handle);
}

void set_window_message_processor(unsigned long long handle, window_message_proc proc)
{
    g_window_message_proc = proc;
    SetMsgPrcor(handle, dispatch_xj380_message);
}

void wait_window()
{
    while (true) {
        __asm__ __volatile__("pause");
    }
}

void pump_window_events()
{
}

bool is_window_open(unsigned long long handle)
{
    return handle != 0;
}

bool delete_window(unsigned long long handle)
{
    if (handle == 0) return false;

    xapi_CloseWindow(handle);
    return true;
}
void draw_pixel(unsigned long long handle, int x, int y, unsigned int color)
{
    xapi_DrawPoint(handle, x, y, color);
}

void draw_rect(unsigned long long handle, int x, int y, int width, int height, unsigned int color)
{
    if (width <= 0 || height <= 0) return;

    xapi_DrawRect(handle,
                  static_cast<UINT32>(x),
                  static_cast<UINT32>(y),
                  static_cast<UINT32>(x + width - 1),
                  static_cast<UINT32>(y + height - 1),
                  color,
                  true);
}

void clear_draw_commands(unsigned long long)
{
}

void draw_text(unsigned long long handle, int x, int y, unsigned int color, unsigned int size, const stardustui::string& text)
{
    xapi_DrawText(handle, x, y, (char*)text.c_str(), size, color);
}

unsigned int calc_text_width(const stardustui::string& text, unsigned int size)
{
    return static_cast<unsigned int>(xapi_CalcTextWidth((char*)text.c_str(), size));
}

void sleep_ms(unsigned long long ms)
{
    xapi_Sleep(ms);
}
