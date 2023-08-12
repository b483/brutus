#include "empire.h"

#include "core/image_group_editor.h"
#include "empire/empire.h"
#include "empire/object.h"
#include "graphics/arrow_button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/scroll.h"
#include "scenario/scenario.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"

#define MAX_WIDTH 2032
#define MAX_HEIGHT 1136

static void button_change_empire(int value, int param2);
static void set_city_type(int param1, int param2);
static void set_resource_sell_limit(int resource, int param2);
static void set_resource_buy_limit(int resource, int param2);
static void set_trade_route_cost(int param1, int param2);
static void set_expansion_year_offset(int param1, int param2);

static struct arrow_button_t arrow_buttons_empire[] = {
    {28, -52, 17, 24, button_change_empire, -1, 0, 0, 0},
    {52, -52, 15, 24, button_change_empire, 1, 0, 0, 0}
};
static struct arrow_button_t arrow_buttons_set_city_type[] = {
    {0, 0, 21, 24, set_city_type, 0, 0, 0, 0}
};
static struct generic_button_t button_toggle_sell_resource_limit[] = {
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_sell_limit, button_none, 0, 0},
};
static struct generic_button_t button_toggle_buy_resource_limit[] = {
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
    {0, 0, 26, 26, set_resource_buy_limit, button_none, 0, 0},
};
static struct generic_button_t button_set_trade_route_cost[] = {
    {0, 0, 65, 26, set_trade_route_cost, button_none, 0, 0},
};
static struct generic_button_t button_set_expansion_year[] = {
    {0, 0, 38, 26, set_expansion_year_offset, button_none, 0, 0},
};

static struct {
    struct empire_object_t *selected_object;
    int x_min, x_max, y_min, y_max;
    int x_draw_offset, y_draw_offset;
    int focus_trade_route_cost_button_id;
    int focus_expansion_year_button_id;
    int show_battle_objects;
} data;

static void init(void)
{
    data.selected_object = 0;
    data.focus_trade_route_cost_button_id = 0;
    data.focus_expansion_year_button_id = 0;
}

static int map_viewport_width(void)
{
    return data.x_max - data.x_min - 32;
}

static int map_viewport_height(void)
{
    return data.y_max - data.y_min - 136;
}

static void draw_paneling(void)
{
    int image_base = image_group(GROUP_EDITOR_EMPIRE_PANELS);
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
    draw_paneling();
}

static void draw_shadowed_number(int value, int x, int y, color_t color)
{
    text_draw_number_colored(value, '@', " ", x + 1, y - 1, FONT_SMALL_PLAIN, COLOR_BLACK);
    text_draw_number_colored(value, '@', " ", x, y, FONT_SMALL_PLAIN, color);
}

static void draw_empire_objects(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use) {
            // don't draw trade route if trade city switched to non-trade city
            if (empire_objects[i].type == EMPIRE_OBJECT_LAND_TRADE_ROUTE || empire_objects[i].type == EMPIRE_OBJECT_SEA_TRADE_ROUTE) {
                struct empire_object_t *trade_city = get_trade_city_by_trade_route(empire_objects[i].trade_route_id);
                if (!trade_city) {
                    continue;
                }
            }

            if (!data.show_battle_objects && (
                empire_objects[i].type == EMPIRE_OBJECT_BATTLE_ICON ||
                empire_objects[i].type == EMPIRE_OBJECT_ROMAN_ARMY ||
                empire_objects[i].type == EMPIRE_OBJECT_ENEMY_ARMY)) {
                continue;
            }

            if (empire_objects[i].type == EMPIRE_OBJECT_BATTLE_ICON) {
                draw_shadowed_number(empire_objects[i].invasion_path_id, data.x_draw_offset + empire_objects[i].x - 9, data.y_draw_offset + empire_objects[i].y - 9, COLOR_WHITE);
                draw_shadowed_number(empire_objects[i].invasion_years, data.x_draw_offset + empire_objects[i].x + 15, data.y_draw_offset + empire_objects[i].y - 9, COLOR_FONT_RED);
            } else if (empire_objects[i].type == EMPIRE_OBJECT_ROMAN_ARMY || empire_objects[i].type == EMPIRE_OBJECT_ENEMY_ARMY) {
                draw_shadowed_number(empire_objects[i].distant_battle_travel_months, data.x_draw_offset + empire_objects[i].x + 7, data.y_draw_offset + empire_objects[i].y - 9,
                    empire_objects[i].type == EMPIRE_OBJECT_ROMAN_ARMY ? COLOR_WHITE : COLOR_FONT_RED);
            }
            image_draw(empire_objects[i].image_id, data.x_draw_offset + empire_objects[i].x, data.y_draw_offset + empire_objects[i].y);
            const struct image_t *img = image_get(empire_objects[i].image_id);
            if (img->animation_speed_id) {
                image_draw(empire_objects[i].image_id + empire_object_update_animation(&empire_objects[i], empire_objects[i].image_id),
                    data.x_draw_offset + empire_objects[i].x + img->sprite_offset_x,
                    data.y_draw_offset + empire_objects[i].y + img->sprite_offset_y);
            }
        }
    }
}

static void draw_map(void)
{
    int viewport_width = map_viewport_width();
    int viewport_height = map_viewport_height();
    graphics_set_clip_rectangle(data.x_min + 16, data.y_min + 16, viewport_width, viewport_height);

    empire_set_viewport(viewport_width, viewport_height);

    data.x_draw_offset = data.x_min + 16;
    data.y_draw_offset = data.y_min + 16;
    empire_adjust_scroll(&data.x_draw_offset, &data.y_draw_offset);
    image_draw(image_group(GROUP_EDITOR_EMPIRE_MAP), data.x_draw_offset, data.y_draw_offset);

    draw_empire_objects();

    graphics_reset_clip_rectangle();
}

static void draw_resource_trade_city(int resource, int trade_max, int x_offset, int y_offset)
{
    int image_id = resource_images[resource].editor_empire_icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON);
    switch (trade_max) {
        case 15:
            image_draw(image_id, x_offset, y_offset);
            image_draw(image_group(GROUP_EDITOR_TRADE_AMOUNT), x_offset + 17, y_offset);
            break;
        case 25:
            image_draw(image_id, x_offset, y_offset);
            image_draw(image_group(GROUP_EDITOR_TRADE_AMOUNT) + 1, x_offset + 13, y_offset);
            break;
        case 40:
            image_draw(image_id, x_offset, y_offset);
            image_draw(image_group(GROUP_EDITOR_TRADE_AMOUNT) + 2, x_offset + 11, y_offset);
            break;
        default:
            image_draw(image_id, x_offset, y_offset);
    }
}


static void draw_trade_city_info(int x_offset, int y_offset, int width)
{
    // draw arrow buttons for city type switching
    arrow_buttons_set_city_type[0].x_offset = x_offset + 20 + width;
    arrow_buttons_draw(0, 0, arrow_buttons_set_city_type, 1);

    // draw "Sells" and the resources to sell
    width += lang_text_draw(47, 5, x_offset + 100 + width, y_offset, FONT_NORMAL_GREEN);
    int resource_x_offset = x_offset + 110 + width;
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        button_toggle_sell_resource_limit[r - 1].x = resource_x_offset;
        button_toggle_sell_resource_limit[r - 1].y = y_offset - 9;
        button_toggle_sell_resource_limit[r - 1].parameter1 = r;
        if (data.selected_object->resource_sell_limit[r]) {
            draw_resource_trade_city(r, data.selected_object->resource_sell_limit[r], resource_x_offset + 1, y_offset - 8);
        } else {
            image_draw_blend(871, resource_x_offset + 1, y_offset - 8, COLOR_MOUSE_DARK_GRAY);
        }
        resource_x_offset += 32;
    }

    resource_x_offset += 30;
    // draw "Buys" and the resources to buy
    resource_x_offset += lang_text_draw(47, 4, resource_x_offset, y_offset, FONT_NORMAL_GREEN);
    resource_x_offset += 10;
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        button_toggle_buy_resource_limit[r - 1].x = resource_x_offset;
        button_toggle_buy_resource_limit[r - 1].y = y_offset - 9;
        button_toggle_buy_resource_limit[r - 1].parameter1 = r;
        if (data.selected_object->resource_buy_limit[r]) {
            draw_resource_trade_city(r, data.selected_object->resource_buy_limit[r], resource_x_offset + 1, y_offset - 8);
        } else {
            image_draw_blend(871, resource_x_offset + 1, y_offset - 8, COLOR_MOUSE_DARK_GRAY);
        }
        resource_x_offset += 32;
    }

    // draw the trade route cost
    button_set_trade_route_cost->x = resource_x_offset + 255;
    button_set_trade_route_cost->y = y_offset - 8;
    text_draw("Cost to open trade route: ", resource_x_offset + 50, y_offset, FONT_NORMAL_GREEN, 0);
    button_border_draw(button_set_trade_route_cost->x, button_set_trade_route_cost->y, button_set_trade_route_cost->width, 24, data.focus_trade_route_cost_button_id == 1);
    text_draw_number_centered(data.selected_object->trade_route_cost, button_set_trade_route_cost->x, y_offset, button_set_trade_route_cost->width, FONT_NORMAL_GREEN);
}


static void draw_city_info(void)
{
    int x_offset = data.x_min + 28;
    int y_offset = data.y_max - 85;

    int width = lang_text_draw(21, data.selected_object->city_name_id, x_offset, y_offset, FONT_NORMAL_WHITE);
    arrow_buttons_set_city_type[0].y_offset = y_offset - 8;

    switch (data.selected_object->city_type) {
        case EMPIRE_CITY_DISTANT_ROMAN:
        case EMPIRE_CITY_VULNERABLE_ROMAN:
            width += lang_text_draw(47, 12, x_offset + 20 + width, y_offset, FONT_NORMAL_GREEN);
            if (data.selected_object->trade_route_id) {
                arrow_buttons_set_city_type[0].x_offset = x_offset + 20 + width;
                arrow_buttons_draw(0, 0, arrow_buttons_set_city_type, 1);
            }
            break;
        case EMPIRE_CITY_FUTURE_TRADE:
            width += text_draw("A future trade city", x_offset + 20 + width, y_offset, FONT_NORMAL_GREEN, 0);
            draw_trade_city_info(x_offset, y_offset, width);
            // draw empire expansion year (offset from scenario start year)
            text_draw("Year offset for empire expansion: ", x_offset + 350, y_offset + 40, FONT_NORMAL_GREEN, 0);
            button_border_draw(x_offset + 620, y_offset + 32, button_set_expansion_year->width, button_set_expansion_year->height, data.focus_expansion_year_button_id == 1);
            text_draw_number_centered(scenario.empire.expansion_year, x_offset + 620, y_offset + 40, button_set_expansion_year->width, FONT_NORMAL_GREEN);
            break;
        case EMPIRE_CITY_DISTANT_FOREIGN:
        case EMPIRE_CITY_FUTURE_ROMAN:
            lang_text_draw(47, 0, x_offset + 20 + width, y_offset, FONT_NORMAL_GREEN);
            break;
        case EMPIRE_CITY_OURS:
        {
            // draw "Our city!"
            width += lang_text_draw(47, 1, x_offset + 20 + width, y_offset, FONT_NORMAL_GREEN);
            // draw icons for available resources based on the "Buildings allowed" menu
            int resource_x_offset = x_offset + 30 + width;
            for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                if (data.selected_object->resource_sell_limit[r]) {
                    graphics_draw_inset_rect(resource_x_offset, y_offset - 9, 26, 26);
                    int image_id = resource_images[r].editor_empire_icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON);
                    image_draw(image_id, resource_x_offset + 1, y_offset - 8);
                    resource_x_offset += 32;
                }
            }
            break;
        }
        case EMPIRE_CITY_TRADE:
        {
            width += text_draw("A trade city", x_offset + 20 + width, y_offset, FONT_NORMAL_GREEN, 0);
            draw_trade_city_info(x_offset, y_offset, width);
            break;
        }
    }
}

static void draw_panel_buttons(void)
{
    arrow_buttons_draw(data.x_min, data.y_max, arrow_buttons_empire, 2);
    if (data.selected_object && data.selected_object->type == EMPIRE_OBJECT_CITY) {
        draw_city_info();
    } else {
        lang_text_draw_centered(150, scenario.empire.id,
            data.x_min, data.y_max - 85, data.x_max - data.x_min, FONT_NORMAL_GREEN);
    }
}

static void draw_foreground(void)
{
    draw_map();
    draw_panel_buttons();
}

static int is_outside_map(int x, int y)
{
    return (x < data.x_min + 16 || x >= data.x_max - 16 ||
            y < data.y_min + 16 || y >= data.y_max - 120);
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    struct pixel_view_coordinates_t position;
    if (scroll_get_delta(m, &position, SCROLL_TYPE_EMPIRE)) {
        empire_scroll_map(position.x, position.y);
    }
    if (h->toggle_editor_battle_info) {
        data.show_battle_objects = !data.show_battle_objects;
    }
    if (h->show_empire_map) {
        window_editor_map_show();
    }
    if (arrow_buttons_handle_mouse(m, data.x_min, data.y_max, arrow_buttons_empire, 2, 0)) {
        return;
    }
    if (m->left.went_up && !is_outside_map(m->x, m->y)) {
        data.selected_object = empire_select_object(m->x - data.x_min - 16, m->y - data.y_min - 16);
        // place border flag on empty map spot
        if (!data.selected_object
        || !data.selected_object->in_use
        || (data.selected_object->type == EMPIRE_OBJECT_ORNAMENT && data.selected_object->image_id != 3323)) {
            for (int i = 0; i < MAX_OBJECTS; i++) {
                if (!empire_objects[i].in_use) {
                    empire_objects[i].in_use = 1;
                    empire_objects[i].image_id = 3323;
                    empire_objects[i].x = m->x - data.x_min - 16;
                    empire_objects[i].y = m->y - data.y_min - 16;
                    return;
                }
            }
        }
        // if border flag selected, remove it
        if (data.selected_object && data.selected_object->image_id == 3323) {
            data.selected_object->in_use = 0;
        }
        window_invalidate();
    }
    if (data.selected_object && data.selected_object->type == EMPIRE_OBJECT_CITY) {
        if (data.selected_object->trade_route_id) {
            arrow_buttons_handle_mouse(m, 0, 0, arrow_buttons_set_city_type, 1, 0);
            if (generic_buttons_handle_mouse(m, 0, 0, button_toggle_sell_resource_limit, sizeof(button_toggle_sell_resource_limit) / sizeof(struct generic_button_t), 0)) {
                return;
            }
            if (generic_buttons_handle_mouse(m, 0, 0, button_toggle_buy_resource_limit, sizeof(button_toggle_buy_resource_limit) / sizeof(struct generic_button_t), 0)) {
                return;
            }
            if (generic_buttons_handle_mouse(m, 0, 0, button_set_trade_route_cost, 1, &data.focus_trade_route_cost_button_id)) {
                return;
            }
            if (data.selected_object->city_type == EMPIRE_CITY_FUTURE_TRADE) {
                if (generic_buttons_handle_mouse(m, data.x_min + 648, data.y_max - 53, button_set_expansion_year, 1, &data.focus_expansion_year_button_id)) {
                    return;
                }
            }
        }
        if (m->right.went_up || h->escape_pressed) {
            data.selected_object = 0;
            window_invalidate();
        }
    } else if (m->right.went_up || h->escape_pressed) {
        window_editor_map_show();
    }
}

static void button_change_empire(int value, __attribute__((unused)) int param2)
{
    if (scenario.empire.id == 39 && value == 1) {
        scenario.empire.id = 0;
    } else if (scenario.empire.id == 0 && value == -1) {
        scenario.empire.id = 39;
    } else {
        scenario.empire.id += value;
    }
    scenario.is_saved = 0;
    empire_load_editor(scenario.empire.id, map_viewport_width(), map_viewport_height());

    // reset demand changes to prevent possible city/resource mixups
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        scenario.demand_changes[i].trade_city_id = 0;
    }

    window_request_refresh();
}

static void set_city_type(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (data.selected_object->city_type == EMPIRE_CITY_DISTANT_ROMAN) {
        data.selected_object->city_type = EMPIRE_CITY_TRADE;
    } else if (data.selected_object->city_type == EMPIRE_CITY_TRADE) {
        data.selected_object->city_type = EMPIRE_CITY_FUTURE_TRADE;
    } else {
        data.selected_object->city_type = EMPIRE_CITY_DISTANT_ROMAN;
    }

    // fix graphics for city sprite and flag color when changing city types
    if (data.selected_object->city_type == EMPIRE_CITY_TRADE || data.selected_object->city_type == EMPIRE_CITY_FUTURE_TRADE) {
        data.selected_object->image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
        data.selected_object->expanded.image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
    } else if (data.selected_object->city_type == EMPIRE_CITY_DISTANT_ROMAN) {
        data.selected_object->image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
        data.selected_object->expanded.image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
    }

    window_request_refresh();
}

static void set_resource_sell_limit(int resource, __attribute__((unused)) int param2)
{
    switch (data.selected_object->resource_sell_limit[resource]) {
        case 0:
            data.selected_object->resource_sell_limit[resource] = 15;
            break;
        case 15:
            data.selected_object->resource_sell_limit[resource] = 25;
            break;
        case 25:
            data.selected_object->resource_sell_limit[resource] = 40;
            break;
        default:
            data.selected_object->resource_sell_limit[resource] = 0;
    }
    // if resource to buy already enabled, disable
    data.selected_object->resource_buy_limit[resource] = 0;
}

static void set_resource_buy_limit(int resource, __attribute__((unused)) int param2)
{
    switch (data.selected_object->resource_buy_limit[resource]) {
        case 0:
            data.selected_object->resource_buy_limit[resource] = 15;
            break;
        case 15:
            data.selected_object->resource_buy_limit[resource] = 25;
            break;
        case 25:
            data.selected_object->resource_buy_limit[resource] = 40;
            break;
        default:
            data.selected_object->resource_buy_limit[resource] = 0;
    }
    // if resource to sell already enabled, disable
    data.selected_object->resource_sell_limit[resource] = 0;
}

static void set_trade_route_cost_callback(int value)
{
    data.selected_object->trade_route_cost = value;
}

static void set_trade_route_cost(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(button_set_trade_route_cost->x - 150, button_set_trade_route_cost->y - 150, 5, 99999, set_trade_route_cost_callback);
}


static void set_expansion_year_offset_callback(int value)
{
    scenario.empire.expansion_year = value;
}

static void set_expansion_year_offset(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(data.x_min + 500, data.y_max - 250, 3, 500, set_expansion_year_offset_callback);
}

void window_editor_empire_show(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EMPIRE,
        draw_background,
        draw_foreground,
        handle_input,
    };
    init();
    window_show(&window);
}
