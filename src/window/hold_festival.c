#include "hold_festival.h"

#include "building/warehouse.h"
#include "city/constants.h"
#include "city/data.h"
#include "city/finance.h"
#include "city/gods.h"
#include "core/image.h"
#include "core/image_group.h"
#include "city/resource.h"
#include "graphics/graphics.h"
#include "window/advisors.h"
#include "window/message_dialog.h"

static void button_select_god(int god, int param2);
static void button_throw_festival(int size, int cost);
static void button_help_resource_settings(int param1, int param2);

static struct generic_button_t buttons_select_god[] = {
    {101, 76, 81, 91, button_select_god, button_none, 0, 0},
    {192, 76, 81, 91, button_select_god, button_none, 1, 0},
    {283, 76, 81, 91, button_select_god, button_none, 2, 0},
    {374, 76, 81, 91, button_select_god, button_none, 3, 0},
    {465, 76, 81, 91, button_select_god, button_none, 4, 0},
};

static struct generic_button_t buttons_festival_size[] = {
    {100, 183, 448, 26, button_throw_festival, button_none, 1, 0},
    {100, 213, 448, 26, button_throw_festival, button_none, 2, 0},
    {100, 243, 448, 26, button_throw_festival, button_none, 3, 0},
};

static struct image_button_t button_help_hold_festival[] = {
    {68, 257, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help_resource_settings, button_none, 0, 0, 1, 0, 0, 0},
};

static int focus_id_gods_button;
static int focus_id_festival_size_button;
static int focus_help_button_id;

static void init(void)
{
    buttons_festival_size[0].parameter2 = city_data.population.population / 20 + 10;
    buttons_festival_size[1].parameter2 = city_data.population.population / 10 + 20;
    buttons_festival_size[2].parameter2 = city_data.population.population / 5 + 40;
}

static void draw_buttons(void)
{
    // Small festival
    button_border_draw(100, 183, 448, 26, focus_id_festival_size_button == 1);
    int width = lang_text_draw(58, 31, 116, 191, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 0, city_data.population.population / 20 + 10, 116 + width, 191, FONT_NORMAL_BLACK);

    // Large festival
    button_border_draw(100, 213, 448, 26, focus_id_festival_size_button == 2);
    width = lang_text_draw(58, 32, 116, 221, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 0, city_data.population.population / 10 + 20, 116 + width, 221, FONT_NORMAL_BLACK);

    // Grand festival
    button_border_draw(100, 243, 448, 26, focus_id_festival_size_button == 3);
    width = lang_text_draw(58, 33, 116, 251, FONT_NORMAL_BLACK);
    width += lang_text_draw_amount(8, 0, city_data.population.population / 5 + 40, 116 + width, 251, FONT_NORMAL_BLACK);
    width += lang_text_draw_amount(8, 10, city_data.population.population / 500 + 1, 126 + width, 251, FONT_NORMAL_BLACK);
    image_draw(resource_images[RESOURCE_WINE].icon_img_id, 126 + width, 246);

    // greying out of buttons
    if (!city_finance_can_afford(city_data.festival.cost)) {
        graphics_shade_rect(102, (city_data.festival.size * 30 - 30) + 185, 444, 22, 0);
    } else if (city_data.resource.stored_in_warehouses[RESOURCE_WINE] < city_data.population.population / 500 + 1) {
        graphics_shade_rect(102, 245, 444, 22, 0);
    }
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(52, 12, 34, 18);

    // Hold festival to [selected god]
    lang_text_draw_centered(58, 25 + city_data.festival.god, 52, 28, 544, FONT_LARGE_BLACK);

    for (int god = 0; god < MAX_GODS; god++) {
        if (god == city_data.festival.god) {
            image_draw(image_group(GROUP_PANEL_WINDOWS) + god + 21, 91 * god + 101, 76);
        } else {
            image_draw(image_group(GROUP_PANEL_WINDOWS) + god + 16, 91 * god + 101, 76);
        }
    }
    draw_buttons();
    image_buttons_draw(0, 0, button_help_hold_festival, 1);

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_advisors_show(ADVISOR_ENTERTAINMENT);
        return;
    }

    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_select_god, sizeof(buttons_select_god) / sizeof(struct generic_button_t), &focus_id_gods_button)) {
        return;
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_festival_size, sizeof(buttons_festival_size) / sizeof(struct generic_button_t), &focus_id_festival_size_button)) {
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
    city_data.festival.god = god;
    window_invalidate();
}

static void button_throw_festival(int size, int cost)
{
    if (!city_finance_can_afford(cost) || (size == FESTIVAL_GRAND && city_data.resource.stored_in_warehouses[RESOURCE_WINE] < city_data.population.population / 500 + 1)) {
        city_data.festival.size = FESTIVAL_NONE;
        city_data.festival.cost = 0;
        return;
    } else {
        city_data.festival.size = size;
        city_data.festival.cost = cost;
        city_data.festival.months_to_go = city_data.festival.size + 1;
        city_finance_process_sundry(city_data.festival.cost);
        if (city_data.festival.size == FESTIVAL_GRAND) {
            building_warehouses_remove_resource(RESOURCE_WINE, city_data.population.population / 500 + 1);
        }

        window_advisors_show(ADVISOR_ENTERTAINMENT);
    }
}

static void button_help_resource_settings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_ADVISOR_ENTERTAINMENT, 0);
}

void window_hold_festival_show(void)
{
    struct window_type_t window = {
        WINDOW_HOLD_FESTIVAL,
        window_advisors_draw_dialog_background,
        draw_foreground,
        handle_input,
    };
    init();
    window_show(&window);
}
