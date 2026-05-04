#pragma once
#include "../includes/string.hpp"

using window_message_proc = void (*)(unsigned long long type, unsigned long long h_data, unsigned long long l_data);

constexpr unsigned long long kWindowMessageMove = 1;
constexpr unsigned long long kWindowMessageLeftButtonDown = 2;
constexpr unsigned long long kWindowMessageLeftButtonUp = 3;
constexpr unsigned long long kWindowMessageLeftButtonClick = 4;
constexpr unsigned long long kWindowMessageChar = 5;
constexpr unsigned long long kWindowMessageSpecialChar = 6;

bool create_window(char *title, int width, int height, unsigned long long *handle);

void print_error(const char *message);

void log_serial(const char *message);

void append_debug_log(const char *message);

void refresh_window(unsigned long long handle);

void wait_window();

void set_window_message_processor(unsigned long long handle, window_message_proc proc);
void pump_window_events();
bool is_window_open(unsigned long long handle);

bool delete_window(unsigned long long handle);

void draw_pixel(unsigned long long handle, int x, int y, unsigned int color);
void draw_rect(unsigned long long handle, int x, int y, int width, int height, unsigned int color);
void clear_draw_commands(unsigned long long handle);

void draw_text(unsigned long long handle, int x, int y, unsigned int color, unsigned int size, const stardustui::string& text);

unsigned int calc_text_width(const stardustui::string& text, unsigned int size);

void sleep_ms(unsigned long long ms);
