#include "empire.h"

#include "city/data_private.h"
#include "city/military.h"
#include "city/warning.h"
#include "core/image_group.h"
#include "empire/empire.h"
#include "empire/object.h"
#include "game/settings.h"
#include "game/time.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/scroll.h"
#include "scenario/data.h"
#include "scenario/editor_events.h"
#include "window/advisors.h"
#include "window/city.h"
#include "window/message_dialog.h"
#include "window/popup_dialog.h"
#include "window/resource_settings.h"
#include "window/trade_opened.h"

#define MAX_WIDTH 2032
#define MAX_HEIGHT 1136

static void button_help(int param1, int param2);
static void button_return_to_city(int param1, int param2);
static void button_advisor(int advisor, int param2);
static void button_open_trade(int param1, int param2);
static void button_show_resource_window(int resource, int param2);

static image_button image_button_help[] = {
    {0, 0, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help, button_none, 0, 0, 1, 0, 0, 0}
};
static image_button image_button_return_to_city[] = {
    {0, 0, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_return_to_city, button_none, 0, 0, 1, 0, 0, 0}
};
static image_button image_button_advisor[] = {
    {-4, 0, 24, 24, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 12, button_advisor, button_none, ADVISOR_TRADE, 0, 1, 0, 0, 0}
};
static generic_button generic_button_trade_resource[] = {
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_WHEAT, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_VEGETABLES , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_FRUIT , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_OLIVES , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_VINES , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_MEAT, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_WINE , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_OIL , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_IRON , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_TIMBER, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_CLAY, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_MARBLE, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_WEAPONS, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_FURNITURE, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_POTTERY, 0}
};
static generic_button generic_button_open_trade[] = {
    {30, 56, 440, 26, button_open_trade, button_none, 0, 0}
};

static struct {
    struct empire_object_t *selected_object;
    int selected_button;
    int x_min, x_max, y_min, y_max;
    int x_draw_offset, y_draw_offset;
    int focus_button_id;
    int focus_resource;
} data;

static void init(void)
{
    data.selected_button = 0;
    data.selected_object = 0;
    data.focus_button_id = 0;
}

static void draw_paneling(void)
{
    int image_base = image_group(GROUP_EMPIRE_PANELS);
    // bottom panel background
    graphics_set_clip_rectangle(data.x_min, data.y_min, data.x_max - data.x_min, data.y_max - data.y_min);
    for (int x = data.x_min; x < data.x_max; x += 70) {
        image_draw(image_base + 3, x, data.y_max - 120);
        image_draw(image_base + 3, x, data.y_max - 80);
        image_draw(image_base + 3, x, data.y_max - 40);
    }

    // horizontal bar borders
    for (int x = data.x_min; x < data.x_max; x += 86) {
        image_draw(image_base + 1, x, data.y_min);
        image_draw(image_base + 1, x, data.y_max - 120);
        image_draw(image_base + 1, x, data.y_max - 16);
    }

    // vertical bar borders
    for (int y = data.y_min + 16; y < data.y_max; y += 86) {
        image_draw(image_base, data.x_min, y);
        image_draw(image_base, data.x_max - 16, y);
    }

    // crossbars
    image_draw(image_base + 2, data.x_min, data.y_min);
    image_draw(image_base + 2, data.x_min, data.y_max - 120);
    image_draw(image_base + 2, data.x_min, data.y_max - 16);
    image_draw(image_base + 2, data.x_max - 16, data.y_min);
    image_draw(image_base + 2, data.x_max - 16, data.y_max - 120);
    image_draw(image_base + 2, data.x_max - 16, data.y_max - 16);

    graphics_reset_clip_rectangle();
}

static void draw_trade_resource(resource_type resource, int trade_max, int x_offset, int y_offset)
{
    graphics_draw_inset_rect(x_offset, y_offset, 26, 26);

    int image_id = resource + image_group(GROUP_EMPIRE_RESOURCES);
    int resource_offset = resource_image_offset(resource, RESOURCE_IMAGE_ICON);
    image_draw(image_id + resource_offset, x_offset + 1, y_offset + 1);

    if (data.focus_resource == resource) {
        button_border_draw(x_offset - 2, y_offset - 2, 101 + 4, 30, 1);
    }

    switch (trade_max) {
        case 15:
            image_draw(image_group(GROUP_TRADE_AMOUNT), x_offset + 21, y_offset - 1);
            break;
        case 25:
            image_draw(image_group(GROUP_TRADE_AMOUNT) + 1, x_offset + 17, y_offset - 1);
            break;
        case 40:
            image_draw(image_group(GROUP_TRADE_AMOUNT) + 2, x_offset + 13, y_offset - 1);
            break;
    }
}

static void draw_trade_city_info(void)
{
    int x_offset = (data.x_min + data.x_max - 500) / 2;
    int y_offset = data.y_max - 113;
    if (data.selected_object->trade_route_open) {
        // city sells
        lang_text_draw(47, 10, x_offset, y_offset + 40, FONT_NORMAL_GREEN);
        int index = 0;
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (!data.selected_object->resource_sell_limit[r]) {
                continue;
            }
            int trade_max = data.selected_object->resource_sell_limit[r];
            draw_trade_resource(r, trade_max, x_offset + 104 * index + 76, y_offset + 31);
            int trade_now = data.selected_object->resource_sold[r];
            if (trade_now > trade_max) {
                trade_max = trade_now;
            }
            int text_width = text_draw_number(trade_now, '@', "",
                x_offset + 104 * index + 106, y_offset + 40, FONT_NORMAL_GREEN);
            text_width += lang_text_draw(47, 11,
                x_offset + 104 * index + 104 + text_width, y_offset + 40, FONT_NORMAL_GREEN);
            text_draw_number(trade_max, '@', "",
                x_offset + 104 * index + 94 + text_width, y_offset + 40, FONT_NORMAL_GREEN);
            index++;
        }
        // city buys
        lang_text_draw(47, 9, x_offset, y_offset + 71, FONT_NORMAL_GREEN);
        index = 0;
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (!data.selected_object->resource_buy_limit[r]) {
                continue;
            }
            int trade_max = data.selected_object->resource_buy_limit[r];
            draw_trade_resource(r, trade_max, x_offset + 104 * index + 76, y_offset + 62);
            int trade_now = data.selected_object->resource_bought[r];
            if (trade_now > trade_max) {
                trade_max = trade_now;
            }
            int text_width = text_draw_number(trade_now, '@', "",
                                             x_offset + 104 * index + 106, y_offset + 71, FONT_NORMAL_GREEN);
            text_width += lang_text_draw(47, 11,
                                        x_offset + 104 * index + 104 + text_width, y_offset + 71, FONT_NORMAL_GREEN);
            text_draw_number(trade_max, '@', "",
                             x_offset + 104 * index + 94 + text_width, y_offset + 71, FONT_NORMAL_GREEN);
            index++;
        }
    } else { // trade is closed
        int index = lang_text_draw(47, 5, x_offset + 35, y_offset + 42, FONT_NORMAL_GREEN);
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (!data.selected_object->resource_sell_limit[r]) {
                continue;
            }
            int trade_max = data.selected_object->resource_sell_limit[r];
            draw_trade_resource(r, trade_max, x_offset + index + 45, y_offset + 33);
            index += 32;
        }
        index += lang_text_draw(47, 4, x_offset + index + 85, y_offset + 42, FONT_NORMAL_GREEN);
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (!data.selected_object->resource_buy_limit[r]) {
                continue;
            }
            int trade_max = data.selected_object->resource_buy_limit[r];
            draw_trade_resource(r, trade_max, x_offset + index + 95, y_offset + 33);
            index += 32;
        }
        index = lang_text_draw_amount(8, 0, data.selected_object->trade_route_cost,
            x_offset + 40, y_offset + 73, FONT_NORMAL_GREEN);
        lang_text_draw(47, 6, x_offset + index + 40, y_offset + 73, FONT_NORMAL_GREEN);
        int image_id = image_group(GROUP_EMPIRE_TRADE_ROUTE_TYPE) + 1 - data.selected_object->is_sea_trade;
        image_draw(image_id, x_offset + 430, y_offset + 65 + 2 * data.selected_object->is_sea_trade);
    }
}

static void draw_city_name(void)
{
    int image_base = image_group(GROUP_EMPIRE_PANELS);
    image_draw(image_base + 6, data.x_min + 2, data.y_max - 199);
    image_draw(image_base + 7, data.x_max - 84, data.y_max - 199);
    image_draw(image_base + 8, (data.x_min + data.x_max - 332) / 2, data.y_max - 181);
    lang_text_draw_centered(21, data.selected_object->city_name_id,
        (data.x_min + data.x_max - 332) / 2 + 64, data.y_max - 118, 268, FONT_LARGE_BLACK);

}

static void draw_city_info(void)
{
    int x_offset = (data.x_min + data.x_max - 240) / 2;
    int y_offset = data.y_max - 66;

    switch (data.selected_object->city_type) {
        case EMPIRE_CITY_FUTURE_TRADE:
        case EMPIRE_CITY_DISTANT_ROMAN:
            lang_text_draw_centered(47, 12, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
            break;
        case EMPIRE_CITY_VULNERABLE_ROMAN:
            if (city_data.distant_battle.city_foreign_months_left <= 0) {
                lang_text_draw_centered(47, 12, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
            } else {
                lang_text_draw_centered(47, 13, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
            }
            break;
        case EMPIRE_CITY_DISTANT_FOREIGN:
        case EMPIRE_CITY_FUTURE_ROMAN:
            lang_text_draw_centered(47, 0, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
            break;
        case EMPIRE_CITY_OURS:
            lang_text_draw_centered(47, 1, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
            break;
        case EMPIRE_CITY_TRADE:
            draw_trade_city_info();
            break;
    }
}

static void draw_roman_army_info(void)
{
    int x_offset = (data.x_min + data.x_max - 240) / 2;
    int y_offset = data.y_max - 68;
    int text_id;
    if (city_data.distant_battle.roman_months_to_travel_forth) {
        text_id = 15;
    } else {
        text_id = 16;
    }
    lang_text_draw_multiline(47, text_id, x_offset, y_offset, 240, FONT_NORMAL_GREEN);

}

static void draw_enemy_army_info(void)
{
    lang_text_draw_multiline(47, 14,
        (data.x_min + data.x_max - 240) / 2,
        data.y_max - 68,
        240, FONT_NORMAL_GREEN);
}

static void draw_object_info(void)
{
    if (data.selected_object) {
        switch (data.selected_object->type) {
            case EMPIRE_OBJECT_CITY:
                draw_city_name();
                draw_city_info();
                break;
            case EMPIRE_OBJECT_ROMAN_ARMY:
                if (city_military_distant_battle_roman_army_is_traveling()) {
                    if (city_data.distant_battle.roman_months_traveled == data.selected_object->distant_battle_travel_months) {
                        draw_roman_army_info();
                        break;
                    }
                }
                /* fall through */
            case EMPIRE_OBJECT_ENEMY_ARMY:
                if (city_data.distant_battle.months_until_battle) {
                    if (city_data.distant_battle.enemy_months_traveled == data.selected_object->distant_battle_travel_months) {
                        draw_enemy_army_info();
                        break;
                    }
                }
                /* fall through */
            default:
                lang_text_draw_centered(47, 8, data.x_min, data.y_max - 65, data.x_max - data.x_min, FONT_NORMAL_GREEN);
        }
    } else {
        lang_text_draw_centered(47, 8, data.x_min, data.y_max - 65, data.x_max - data.x_min, FONT_NORMAL_GREEN);
    }
}

static void draw_background(void)
{
    int s_width = screen_width();
    int s_height = screen_height();
    data.x_min = s_width <= MAX_WIDTH ? 0 : (s_width - MAX_WIDTH) / 2;
    data.x_max = s_width <= MAX_WIDTH ? s_width : data.x_min + MAX_WIDTH;
    data.y_min = s_height <= MAX_HEIGHT ? 0 : (s_height - MAX_HEIGHT) / 2;
    data.y_max = s_height <= MAX_HEIGHT ? s_height : data.y_min + MAX_HEIGHT;

    if (data.x_min || data.y_min) {
        graphics_clear_screen();
    }
}

static void draw_empire_objects(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use) {
            // don't draw trade routes that aren't open
            if (empire_objects[i].type == EMPIRE_OBJECT_LAND_TRADE_ROUTE || empire_objects[i].type == EMPIRE_OBJECT_SEA_TRADE_ROUTE) {
                struct empire_object_t *trade_city = get_trade_city_by_trade_route(empire_objects[i].trade_route_id);
                if (!trade_city->trade_route_open) {
                    continue;
                }
            }
            if (empire_objects[i].type == EMPIRE_OBJECT_BATTLE_ICON) {
                for (int j = 0; j < MAX_INVASIONS; j++) {
                    int battle_icon_year_abs = game_time_year() + empire_objects[i].invasion_years;
                    int invasion_year_abs = scenario.start_year + scenario.invasions[j].year_offset;
                    // check that invasion is yet to come
                    if (scenario.invasions[j].type == INVASION_TYPE_ENEMY_ARMY
                    && (game_time_year() < invasion_year_abs
                        || (game_time_year() == invasion_year_abs && game_time_month() < scenario.invasions[j].month))) {
                        // draw up to 3 battle icons per invasion, 1 per year
                        if (empire_objects[i].invasion_path_id == (j % 3) + 1
                        && (battle_icon_year_abs > invasion_year_abs
                            || (battle_icon_year_abs == invasion_year_abs && game_time_month() >= scenario.invasions[j].month))
                        ) {
                            image_draw(empire_objects[i].image_id, data.x_draw_offset + empire_objects[i].x, data.y_draw_offset + empire_objects[i].y);
                        }
                    }

                }
                continue;
            }
            if (empire_objects[i].type == EMPIRE_OBJECT_ENEMY_ARMY) {
                if (!city_data.distant_battle.months_until_battle) {
                    continue;
                }
                if (city_data.distant_battle.enemy_months_traveled != empire_objects[i].distant_battle_travel_months) {
                    continue;
                }
            }
            if (empire_objects[i].type == EMPIRE_OBJECT_ROMAN_ARMY) {
                if (!city_military_distant_battle_roman_army_is_traveling()) {
                    continue;
                }
                if (city_data.distant_battle.roman_months_traveled != empire_objects[i].distant_battle_travel_months) {
                    continue;
                }
            }
            int x, y, image_id;
            if (scenario.empire.is_expanded) {
                x = empire_objects[i].expanded.x;
                y = empire_objects[i].expanded.y;
                image_id = empire_objects[i].expanded.image_id;
            } else {
                x = empire_objects[i].x;
                y = empire_objects[i].y;
                image_id = empire_objects[i].image_id;
            }
            if (empire_objects[i].city_type == EMPIRE_CITY_FUTURE_TRADE) {
                // Fix case where future trade city (as specified in the editor) is drawn as a trade city before expansion
                image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
            }
            image_draw(image_id, data.x_draw_offset + x, data.y_draw_offset + y);
            const image *img = image_get(image_id);
            if (img->animation_speed_id) {
                image_draw(image_id + empire_object_update_animation(&empire_objects[i], image_id),
                    data.x_draw_offset + x + img->sprite_offset_x,
                    data.y_draw_offset + y + img->sprite_offset_y);
            }
        }
    }
}

static void draw_map(void)
{
    graphics_set_clip_rectangle(data.x_min + 16, data.y_min + 16,
        data.x_max - data.x_min - 32, data.y_max - data.y_min - 136);

    empire_set_viewport(data.x_max - data.x_min - 32, data.y_max - data.y_min - 136);

    data.x_draw_offset = data.x_min + 16;
    data.y_draw_offset = data.y_min + 16;
    empire_adjust_scroll(&data.x_draw_offset, &data.y_draw_offset);
    image_draw(image_group(GROUP_EMPIRE_MAP), data.x_draw_offset, data.y_draw_offset);

    draw_empire_objects();

    graphics_reset_clip_rectangle();
}

static void draw_panel_buttons(void)
{
    image_buttons_draw(data.x_min + 20, data.y_max - 44, image_button_help, 1);
    image_buttons_draw(data.x_max - 44, data.y_max - 44, image_button_return_to_city, 1);
    image_buttons_draw(data.x_max - 44, data.y_max - 100, image_button_advisor, 1);
    if (data.selected_object) {
        if (data.selected_object->city_type == EMPIRE_CITY_TRADE && !data.selected_object->trade_route_open) {
            button_border_draw((data.x_min + data.x_max - 500) / 2 + 30, data.y_max - 49, 440,
                26, data.selected_button);
        }
    }
}

static void draw_foreground(void)
{
    draw_map();
    draw_paneling();
    draw_panel_buttons();
    draw_object_info();
}

static int is_outside_map(int x, int y)
{
    return (x < data.x_min + 16 || x >= data.x_max - 16 ||
            y < data.y_min + 16 || y >= data.y_max - 120);
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    pixel_offset position;
    if (scroll_get_delta(m, &position, SCROLL_TYPE_EMPIRE)) {
        empire_scroll_map(position.x, position.y);
    }
    data.focus_button_id = 0;
    data.focus_resource = 0;
    int button_id;
    image_buttons_handle_mouse(m, data.x_min + 20, data.y_max - 44, image_button_help, 1, &button_id);
    if (button_id) {
        data.focus_button_id = 1;
    }
    image_buttons_handle_mouse(m, data.x_max - 44, data.y_max - 44, image_button_return_to_city, 1, &button_id);
    if (button_id) {
        data.focus_button_id = 2;
    }
    image_buttons_handle_mouse(m, data.x_max - 44, data.y_max - 100, image_button_advisor, 1, &button_id);
    if (button_id) {
        data.focus_button_id = 3;
    }
    button_id = 0;

    if (h->show_last_advisor) {
        window_advisors_show(setting_last_advisor());
        return;
    }

    if (h->show_empire_map) {
        window_city_show();
        return;
    }

    if (m->left.went_up && !is_outside_map(m->x, m->y)) {
        data.selected_object = empire_select_object(m->x - data.x_min - 16, m->y - data.y_min - 16);
    }
    if (data.selected_object) {
        window_invalidate();
        if (data.selected_object->city_type == EMPIRE_CITY_TRADE) {
            if (data.selected_object->trade_route_open) {
                int x_offset = (data.x_min + data.x_max - 500) / 2;
                int y_offset = data.y_max - 113;
                int index_sell = 0;
                int index_buy = 0;

                // we only want to handle resource buttons that the selected city trades
                for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
                    if (data.selected_object->resource_sell_limit[r]) {
                        generic_buttons_handle_mouse(m, x_offset + 75 + 104 * index_sell, y_offset + 31,
                            generic_button_trade_resource + r - 1, 1, &button_id);
                        index_sell++;
                    } else if (data.selected_object->resource_buy_limit[r]) {
                        generic_buttons_handle_mouse(m, x_offset + 75 + 104 * index_buy, y_offset + 62,
                            generic_button_trade_resource + r - 1, 1, &button_id);
                        index_buy++;
                    }

                    if (button_id) {
                        data.focus_resource = r;
                        // if we're focusing any button we can skip further checks
                        break;
                    }
                }
            } else {
                generic_buttons_handle_mouse(
                    m, (data.x_min + data.x_max - 500) / 2, data.y_max - 105,
                    generic_button_open_trade, 1, &data.selected_button);
            }
        }
        // allow de-selection only for objects that are currently selected/drawn
        if (m->right.went_up || h->escape_pressed) {
            switch (data.selected_object->type) {
                case EMPIRE_OBJECT_CITY:
                    data.selected_object = 0;
                    window_invalidate();
                    break;
                case EMPIRE_OBJECT_ROMAN_ARMY:
                    if (city_military_distant_battle_roman_army_is_traveling()) {
                        if (city_data.distant_battle.roman_months_traveled == data.selected_object->distant_battle_travel_months) {
                            data.selected_object = 0;
                            window_invalidate();
                            break;
                        }
                    }
                    /* fall through */
                case EMPIRE_OBJECT_ENEMY_ARMY:
                    if (city_data.distant_battle.months_until_battle) {
                        if (city_data.distant_battle.enemy_months_traveled == data.selected_object->distant_battle_travel_months) {
                            data.selected_object = 0;
                            window_invalidate();
                            break;
                        }
                    }
                    /* fall through */
                default:
                    window_go_back();
            }
        }
    } else if (m->right.went_up || h->escape_pressed) {
        window_go_back();
        if (window_is(WINDOW_TRADE_OPENED)) {
            window_city_show();
        }
    }
}

static int is_mouse_hit(tooltip_context *c, int x, int y, int size)
{
    int mx = c->mouse_x;
    int my = c->mouse_y;
    return x <= mx && mx < x + size && y <= my && my < y + size;
}

static int get_tooltip_resource(tooltip_context *c)
{
    // we only want to check tooltips on our own closed cities.
    // open city resource tooltips are handled by their respective buttons directly
    if (data.selected_object->city_type != EMPIRE_CITY_TRADE || data.selected_object->trade_route_open) {
        return 0;
    }
    int x_offset = (data.x_min + data.x_max - 500) / 2;
    int y_offset = data.y_max - 113;

    int item_offset = lang_text_get_width(47, 5, FONT_NORMAL_GREEN);
    for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (data.selected_object->resource_sell_limit[r]) {
            if (is_mouse_hit(c, x_offset + 45 + item_offset, y_offset + 33, 26)) {
                return r;
            }
            item_offset += 32;
        }
    }
    item_offset += lang_text_get_width(47, 4, FONT_NORMAL_GREEN);
    for (int r = RESOURCE_MIN; r <= RESOURCE_MAX; r++) {
        if (data.selected_object->resource_buy_limit[r]) {
            if (is_mouse_hit(c, x_offset + 95 + item_offset, y_offset + 33, 26)) {
                return r;
            }
            item_offset += 32;
        }
    }

    return 0;
}

static void get_tooltip_trade_route_type(tooltip_context *c)
{
    if (!data.selected_object || data.selected_object->city_type != EMPIRE_OBJECT_CITY) {
        return;
    }

    if (data.selected_object->city_type != EMPIRE_CITY_TRADE || data.selected_object->trade_route_open) {
        return;
    }

    int x_offset = (data.x_min + data.x_max + 300) / 2;
    int y_offset = data.y_max - 41;
    int y_offset_max = y_offset + 22 - 2 * data.selected_object->is_sea_trade;
    if (c->mouse_x >= x_offset && c->mouse_x < x_offset + 32 &&
        c->mouse_y >= y_offset && c->mouse_y < y_offset_max) {
        c->type = TOOLTIP_BUTTON;
        c->text_group = 44;
        c->text_id = 28 + data.selected_object->is_sea_trade;
    }
}

static void get_tooltip(tooltip_context *c)
{
    if (data.focus_button_id) {
        c->type = TOOLTIP_BUTTON;
        switch (data.focus_button_id) {
            case 1: c->text_id = 1; break;
            case 2: c->text_id = 2; break;
            case 3: c->text_id = 69; break;
        }
    }
    if (!data.selected_object) {
        return;
    }
    int resource = data.focus_resource ? data.focus_resource : get_tooltip_resource(c);
    if (resource) {
        c->type = TOOLTIP_BUTTON;
        c->text_id = 131 + resource;
    } else {
        get_tooltip_trade_route_type(c);
    }
}

static void button_help(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_EMPIRE_MAP, 0);
}

static void button_return_to_city(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_city_show();
}

static void button_advisor(int advisor, __attribute__((unused)) int param2)
{
    window_advisors_show(advisor);
}

static void button_show_resource_window(int resource, __attribute__((unused)) int param2)
{
    window_resource_settings_show(resource);
}

static void confirmed_open_trade(void)
{
    city_finance_process_construction(data.selected_object->trade_route_cost);
    data.selected_object->trade_route_open = 1;
    window_trade_opened_show(data.selected_object);
}

static void button_open_trade(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_popup_dialog_show(POPUP_DIALOG_OPEN_TRADE, confirmed_open_trade, 2);
}

void window_empire_show(void)
{
    window_type window = {
        WINDOW_EMPIRE,
        draw_background,
        draw_foreground,
        handle_input,
        get_tooltip
    };
    init();
    window_show(&window);
}
