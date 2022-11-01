#include "hold_festival.h"

#include "city/constants.h"
#include "city/data_private.h"
#include "city/festival.h"
#include "city/finance.h"
#include "city/gods.h"
#include "core/image_group.h"
#include "game/resource.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/window.h"
#include "window/advisors.h"
#include "window/message_dialog.h"

static void button_select_god(int god, int param2);
static void button_select_festival_size(int size, int param2);
static void button_help_resource_settings(int param1, int param2);

static generic_button buttons_select_god[] = {
    {101, 76, 81, 91, button_select_god, button_none, 0, 0},
    {192, 76, 81, 91, button_select_god, button_none, 1, 0},
    {283, 76, 81, 91, button_select_god, button_none, 2, 0},
    {374, 76, 81, 91, button_select_god, button_none, 3, 0},
    {465, 76, 81, 91, button_select_god, button_none, 4, 0},
};

static generic_button buttons_festival_size[] = {
    {100, 183, 448, 26, button_select_festival_size, button_none, 1, 0},
    {100, 213, 448, 26, button_select_festival_size, button_none, 2, 0},
    {100, 243, 448, 26, button_select_festival_size, button_none, 3, 0},
};

static image_button button_help_hold_festival[] = {
    {68, 257, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help_resource_settings, button_none, 0, 0, 1, 0, 0, 0},
};

static int focus_id_gods_button;
static int focus_id_festival_size_button;
static int focus_help_button_id;

static void draw_buttons(void)
{
    // Small festival
    button_border_draw(100, 183, 448, 26, focus_id_festival_size_button == 1);
    int width = lang_text_draw(58, 31, 116, 191, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 0, city_data.festival.small_cost, 116 + width, 191, FONT_NORMAL_BLACK);

    // Large festival
    button_border_draw(100, 213, 448, 26, focus_id_festival_size_button == 2);
    width = lang_text_draw(58, 32, 116, 221, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 0, city_data.festival.large_cost, 116 + width, 221, FONT_NORMAL_BLACK);

    // Grand festival
    button_border_draw(100, 243, 448, 26, focus_id_festival_size_button == 3);
    width = lang_text_draw(58, 33, 116, 251, FONT_NORMAL_BLACK);
    width += lang_text_draw_amount(8, 0, city_data.festival.grand_cost, 116 + width, 251, FONT_NORMAL_BLACK);
    width += lang_text_draw_amount(8, 10, city_data.festival.grand_wine, 126 + width, 251, FONT_NORMAL_BLACK);
    image_draw(image_group(GROUP_RESOURCE_ICONS) + RESOURCE_WINE, 126 + width, 246);

    // greying out of buttons
    if (city_finance_out_of_money()) {
        graphics_shade_rect(102, 185, 444, 22, 0);
        graphics_shade_rect(102, 215, 444, 22, 0);
        graphics_shade_rect(102, 245, 444, 22, 0);
    } else if (city_data.festival.not_enough_wine) {
        graphics_shade_rect(102, 245, 444, 22, 0);
    }
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(52, 12, 34, 18);

    // Hold festival to [selected god]
    lang_text_draw_centered(58, 25 + city_data.festival.selected.god, 52, 28, 544, FONT_LARGE_BLACK);

    for (int god = 0; god < MAX_GODS; god++) {
        if (god == city_data.festival.selected.god) {
            image_draw(image_group(GROUP_PANEL_WINDOWS) + god + 21, 91 * god + 101, 76);
        } else {
            image_draw(image_group(GROUP_PANEL_WINDOWS) + god + 16, 91 * god + 101, 76);
        }
    }
    draw_buttons();
    image_buttons_draw(0, 0, button_help_hold_festival, 1);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_advisors_show(ADVISOR_ENTERTAINMENT);
        return;
    }

    const mouse *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_select_god, sizeof(buttons_select_god) / sizeof(generic_button), &focus_id_gods_button)) {
        return;
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_festival_size, sizeof(buttons_festival_size) / sizeof(generic_button), &focus_id_festival_size_button)) {
        return;
    }
    if (image_buttons_handle_mouse(m_dialog, 0, 0, button_help_hold_festival, 1, &focus_help_button_id)) {
        return;
    }

    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 52 || m_dialog->y < 12 || m_dialog->x > 596 || m_dialog->y > 300)) {
        window_go_back();
        return;
    }
}

static void button_select_god(int god, __attribute__((unused)) int param2)
{
    city_data.festival.selected.god = god;
    window_invalidate();
}

static void button_select_festival_size(int size, __attribute__((unused)) int param2)
{
    if (city_finance_out_of_money() || (size == FESTIVAL_GRAND && city_data.festival.not_enough_wine)) {
        return;
    } else {
        city_data.festival.selected.size = size;
        city_festival_schedule();
        window_advisors_show(ADVISOR_ENTERTAINMENT);
    }
}

static void button_help_resource_settings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_ADVISOR_ENTERTAINMENT, 0);
}

static void get_tooltip(tooltip_context *c)
{
    if (!focus_help_button_id && !focus_id_gods_button) {
        return;
    }
    c->type = TOOLTIP_BUTTON;
    switch (focus_id_gods_button) {
        case 1: c->text_id = 115; return;
        case 2: c->text_id = 116; return;
        case 3: c->text_id = 117; return;
        case 4: c->text_id = 118; return;
        case 5: c->text_id = 119; return;
    }
    if (focus_help_button_id) {
        c->text_id = 1;
        return;
    }
}

void window_hold_festival_show(void)
{
    window_type window = {
        WINDOW_HOLD_FESTIVAL,
        window_advisors_draw_dialog_background,
        draw_foreground,
        handle_input,
        get_tooltip
    };
    window_show(&window);
}
