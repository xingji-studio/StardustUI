#include "../../includes/window.hpp"
#include "../../includes/components/lable.hpp"
#include "../../includes/sytle.hpp"

static int helloworld_main_impl(int, char**, char**) {
    append_debug_log("helloworld: main enter\n");
    Window window("Hello, World!", 400, 300);
    append_debug_log("helloworld: window constructed\n");

    Sytel base_style;
    base_style.set_color(0x000000FF);
    base_style.set_size(24);

    Sytel hover_style;
    hover_style.set_color(0xFF0000FF);

    SytelRules rules;
    rules.set_base_sytel(base_style);
    rules.set_on_hover_sytel(hover_style);

    Lable hello_label("Hello, World!", 24, 0x000000FF);
    hello_label.set_style_rules(rules);
    hello_label.set_pos(100, 100);
    window.addComponent(hello_label);
    window.show();
    append_debug_log("helloworld: window.show returned\n");
    return 0;
}

#if defined(STARDUSTUI_CLEONOS)
extern "C" int stardust_helloworld_entry(int argc, char *argv[], char *envp[])
{
    return helloworld_main_impl(argc, argv, envp);
}
#elif defined(STARDUSTUI_WINDOWS) || defined(STARDUSTUI_LINUX)
int main(int argc, char *argv[], char *envp[])
{
    return helloworld_main_impl(argc, argv, envp);
}
#else
extern "C" int helloworld_main_cpp(int argc, char *argv[], char *envp[]) asm("_Z4mainiPPcS0_");
extern "C" int helloworld_main_cpp(int argc, char *argv[], char *envp[])
{
    return helloworld_main_impl(argc, argv, envp);
}
#endif
