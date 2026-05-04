#include "../../includes/window.hpp"
#include "../../includes/components/button.hpp"
#include "../../includes/components/canvas.hpp"
#include "../../includes/components/flex.hpp"
#include "../../includes/components/lable.hpp"
#include "../../includes/components/textbox.hpp"
#include "../../includes/sytle.hpp"

namespace {
TextBox* g_history_box = nullptr;
TextBox* g_input_box = nullptr;
Lable* g_status_label = nullptr;

void append_text(stardustui::string& target, const char* text) {
    if (text != nullptr) {
        target.append(text);
    }
}

void append_line(stardustui::string& target, const char* text) {
    append_text(target, text);
    target.push_char('\n');
}

void draw_sidebar_avatar(Canvas& canvas, unsigned int color, unsigned int accent) {
    canvas.fill_rect(0, 0, canvas.get_width(), canvas.get_height(), color);
    canvas.fill_rect(4, 4, canvas.get_width() - 8, canvas.get_height() - 8, accent);
}

void draw_avatar_one(Canvas& canvas) {
    draw_sidebar_avatar(canvas, 0x2D6AE3FF, 0xAFC9FFFF);
}

void draw_avatar_two(Canvas& canvas) {
    draw_sidebar_avatar(canvas, 0x178F65FF, 0xA8E5CDFF);
}

void draw_avatar_three(Canvas& canvas) {
    draw_sidebar_avatar(canvas, 0xC6671FFF, 0xFFD0AEFF);
}

void paint_static_avatar(Canvas& canvas, void (*drawer)(Canvas&)) {
    canvas.clear();
    if (drawer != nullptr) {
        drawer(canvas);
    }
}

void on_send_click() {
    if (g_history_box == nullptr || g_input_box == nullptr || g_status_label == nullptr) {
        return;
    }

    const stardustui::string& input = g_input_box->get_text();
    if (input.length() <= 0) {
        g_status_label->set_text("Type something before sending");
        return;
    }

    stardustui::string next_history;
    const stardustui::string& current_history = g_history_box->get_text();
    if (current_history.length() > 0) {
        next_history.append(current_history.c_str());
        next_history.push_char('\n');
    }
    next_history.append("Me: ");
    next_history.append(input.c_str());

    g_history_box->set_text(next_history);
    g_input_box->set_text("");
    g_status_label->set_text("Message sent");
}
}

static int layout_main_impl(int, char**, char**) {
    Window window("StardustUI Chat Demo", 1040, 680);

    FlexLayout root(1000, 640);
    root.set_pos(20, 20);
    root.set_direction(FlexLayout::Row);
    root.set_gap(0);

    FlexLayout sidebar(260, 0);
    sidebar.set_direction(FlexLayout::Column);
    sidebar.set_gap(12);
    sidebar.set_padding(18);

    Lable sidebar_title("Conversations", 24, 0x1A2433FF);
    Lable sidebar_hint("Pinned teams and channels", 14, 0x657289FF);

    Sytel contact_base;
    contact_base.set_color(0x1A2433FF);
    contact_base.set_size(15);
    contact_base.set_background_color(0xFFFFFFFF);
    contact_base.set_border_color(0xD9E1EAFF);
    contact_base.set_border_width(1);
    contact_base.set_padding(12);

    Sytel contact_hover;
    contact_hover.set_background_color(0xDCEBFFFF);
    contact_hover.set_border_color(0x8BB8F8FF);

    Sytel contact_click;
    contact_click.set_background_color(0x2D6AE3FF);
    contact_click.set_color(0xFFFFFFFF);
    contact_click.set_border_color(0x2D6AE3FF);

    SytelRules contact_rules;
    contact_rules.set_base_sytel(contact_base);
    contact_rules.set_on_hover_sytel(contact_hover);
    contact_rules.set_on_click_sytel(contact_click);

    FlexLayout contact_one(0, 54);
    contact_one.set_direction(FlexLayout::Row);
    contact_one.set_gap(10);
    contact_one.set_align_items(FlexLayout::AlignCenter);
    Canvas avatar_one(28, 28);
    paint_static_avatar(avatar_one, draw_avatar_one);
    Button chat_one("Project Nebula", 0, 44, contact_rules);
    contact_one.addComponent(avatar_one, 0);
    contact_one.addComponent(chat_one, 1);

    FlexLayout contact_two(0, 54);
    contact_two.set_direction(FlexLayout::Row);
    contact_two.set_gap(10);
    contact_two.set_align_items(FlexLayout::AlignCenter);
    Canvas avatar_two(28, 28);
    paint_static_avatar(avatar_two, draw_avatar_two);
    Button chat_two("Rendering Squad", 0, 44, contact_rules);
    contact_two.addComponent(avatar_two, 0);
    contact_two.addComponent(chat_two, 1);

    FlexLayout contact_three(0, 54);
    contact_three.set_direction(FlexLayout::Row);
    contact_three.set_gap(10);
    contact_three.set_align_items(FlexLayout::AlignCenter);
    Canvas avatar_three(28, 28);
    paint_static_avatar(avatar_three, draw_avatar_three);
    Button chat_three("Design Review", 0, 44, contact_rules);
    contact_three.addComponent(avatar_three, 0);
    contact_three.addComponent(chat_three, 1);

    sidebar.addComponent(sidebar_title, 0);
    sidebar.addComponent(sidebar_hint, 0);
    sidebar.addComponent(contact_one, 0);
    sidebar.addComponent(contact_two, 0);
    sidebar.addComponent(contact_three, 0);

    FlexLayout main_column(0, 0);
    main_column.set_direction(FlexLayout::Column);
    main_column.set_gap(12);
    main_column.set_padding(12);

    FlexLayout header_content(0, 84);
    header_content.set_direction(FlexLayout::Row);
    header_content.set_padding(20);
    header_content.set_align_items(FlexLayout::AlignCenter);
    header_content.set_justify_content(FlexLayout::JustifySpaceBetween);

    Lable room_title("Project Nebula", 28, 0x152033FF);
    Lable room_meta("4 people online", 14, 0x68758BFF);

    Sytel action_base;
    action_base.set_color(0x1A2433FF);
    action_base.set_size(14);
    action_base.set_background_color(0xFFFFFFFF);
    action_base.set_border_color(0xD9E1EAFF);
    action_base.set_border_width(1);
    action_base.set_padding(10);

    Sytel action_hover;
    action_hover.set_background_color(0xF0F6FFFF);
    action_hover.set_border_color(0x8BB8F8FF);

    SytelRules action_rules;
    action_rules.set_base_sytel(action_base);
    action_rules.set_on_hover_sytel(action_hover);

    Button details_button("Details", 110, 40, action_rules);

    FlexLayout header_labels(0, 44);
    header_labels.set_direction(FlexLayout::Column);
    header_labels.set_gap(4);
    header_labels.addComponent(room_title, 0);
    header_labels.addComponent(room_meta, 0);

    header_content.addComponent(header_labels, 1);
    header_content.addComponent(details_button, 0);

    Sytel history_base;
    history_base.set_color(0x1F2937FF);
    history_base.set_size(16);
    history_base.set_background_color(0xFFFFFFFF);
    history_base.set_border_color(0xD8DFE9FF);
    history_base.set_border_width(1);
    history_base.set_padding(14);

    Sytel history_hover;
    history_hover.set_border_color(0x8BB8F8FF);

    SytelRules history_rules;
    history_rules.set_base_sytel(history_base);
    history_rules.set_on_hover_sytel(history_hover);

    TextBox history_box(0, 0, false, history_rules);
    stardustui::string history_text;
    append_line(history_text, "Mina: Morning. The new layout build is on the CI machine.");
    append_line(history_text, "Leo: I checked Linux and the flex sidebar is stable now.");
    append_line(history_text, "Rin: Windows still needs a flicker pass, but SDL behaves better.");
    append_line(history_text, "Mina: Good. Put the textbox scrollbar demo into the examples.");
    append_line(history_text, "Leo: Done. It wraps long messages and keeps the cursor blinking on focus.");
    append_line(history_text, "Rin: Nice. Next step is polishing the message composer spacing.");
    append_line(history_text, "Mina: Keep this chat window as the flex showcase.");
    append_line(history_text, "Leo: Sending a few longer lines here so the history box has enough content to scroll inside the conversation panel without needing any extra mock data.");
    history_box.set_text(history_text);
    g_history_box = &history_box;

    FlexLayout composer(0, 150);
    composer.set_direction(FlexLayout::Column);
    composer.set_gap(12);
    composer.set_padding(16);

    Lable composer_hint("Message", 14, 0x68758BFF);

    Sytel input_base = history_base;
    input_base.set_background_color(0xFCFDFFFF);

    Sytel input_click;
    input_click.set_border_color(0x2D6AE3FF);

    SytelRules input_rules;
    input_rules.set_base_sytel(input_base);
    input_rules.set_on_hover_sytel(history_hover);
    input_rules.set_on_click_sytel(input_click);

    FlexLayout composer_row(0, 92);
    composer_row.set_direction(FlexLayout::Row);
    composer_row.set_gap(12);
    composer_row.set_align_items(FlexLayout::AlignStretch);

    TextBox input_box(0, 92, true, input_rules);
    input_box.set_text("");
    g_input_box = &input_box;

    Sytel send_base;
    send_base.set_color(0xFFFFFFFF);
    send_base.set_size(16);
    send_base.set_background_color(0x2D6AE3FF);
    send_base.set_border_color(0x2D6AE3FF);
    send_base.set_border_width(1);
    send_base.set_padding(12);

    Sytel send_hover;
    send_hover.set_background_color(0x4A84F0FF);
    send_hover.set_border_color(0x4A84F0FF);

    Sytel send_click;
    send_click.set_background_color(0x1C54C3FF);
    send_click.set_border_color(0x1C54C3FF);

    SytelRules send_rules;
    send_rules.set_base_sytel(send_base);
    send_rules.set_on_hover_sytel(send_hover);
    send_rules.set_on_click_sytel(send_click);

    Button send_button("Send", 120, 92, send_rules);
    send_button.callback(on_send_click);

    Lable status_label("Flex chat demo ready", 13, 0x68758BFF);
    g_status_label = &status_label;

    composer_row.addComponent(input_box, 1);
    composer_row.addComponent(send_button, 0);
    composer.addComponent(composer_hint, 0);
    composer.addComponent(composer_row, 1);
    composer.addComponent(status_label, 0);

    main_column.addComponent(header_content, 0);
    main_column.addComponent(history_box, 1);
    main_column.addComponent(composer, 0);

    window.addComponent(root);
    root.addComponent(sidebar, 0);
    root.addComponent(main_column, 1);
    window.show();
    return 0;
}

#if defined(STARDUSTUI_CLEONOS)
extern "C" int stardust_layout_entry(int argc, char *argv[], char *envp[])
{
    return layout_main_impl(argc, argv, envp);
}
#elif defined(STARDUSTUI_WINDOWS) || defined(STARDUSTUI_LINUX)
int main(int argc, char *argv[], char *envp[])
{
    return layout_main_impl(argc, argv, envp);
}
#else
extern "C" int layout_main_cpp(int argc, char *argv[], char *envp[]) asm("_Z4mainiPPcS0_");
extern "C" int layout_main_cpp(int argc, char *argv[], char *envp[])
{
    return layout_main_impl(argc, argv, envp);
}
#endif
