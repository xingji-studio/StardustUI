#pragma once
#include "../settings.hpp"
#ifdef  XJ380
#include "../platforms/xj380.hpp"
#endif
#ifdef STARDUSTUI_WINDOWS
#include "../platforms/windows.hpp"
#endif
#ifdef STARDUSTUI_LINUX
#include "../platforms/linux.hpp"
#endif
#ifdef STARDUSTUI_CLEONOS
#include "../platforms/cleonos.hpp"
#endif
#include "components/base.hpp"
#include "string.hpp"
#include "vector.hpp"

class Window {
  public:
	Window(const char* title, int width, int height);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	void show();
	void hide();
	int getWidth();
	int getHeight();
	const char* getTitle();
	void error(const char*);
	void handle_message(unsigned long long type, unsigned long long h_data, unsigned long long l_data);
	void addComponent(base_component& component);
	void addComponent(base_component* component);
private:
	void draw_components();
	stardustui::string title;
	int width;
	int height;
	unsigned long long handle;
	bool needs_redraw;
	bool background_needs_clear;
	stardustui::vector<base_component*> components; 
};
