#include "figures.h"

#include "building/building.h"
#include "city/view.h"
#include "empire/object.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/phrase.h"
#include "figure/trader.h"
#include "game/custom_strings.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "widget/city.h"

static void select_figure(int index, int param2);

static const int FIGURE_TYPE_TO_BIG_FIGURE_IMAGE[] = {
8, // FIGURE_NONE
13, // FIGURE_IMMIGRANT
13, // FIGURE_EMIGRANT
9, // FIGURE_HOMELESS
58, // FIGURE_PATRICIAN
4, // FIGURE_CART_PUSHER
13, // FIGURE_LABOR_SEEKER
2, // FIGURE_BARBER
3, // FIGURE_BATHHOUSE_WORKER
6, // FIGURE_DOCTOR
6, // FIGURE_SURGEON
14, // FIGURE_PRIEST
5, // FIGURE_SCHOOL_CHILD
52, // FIGURE_TEACHER
52, // FIGURE_LIBRARIAN
53, // FIGURE_MISSIONARY
1, // FIGURE_ACTOR
33, // FIGURE_GLADIATOR
10, // FIGURE_LION_TAMER
11, // FIGURE_CHARIOTEER
8, // FIGURE_HIPPODROME_HORSES
16, // FIGURE_TAX_COLLECTOR
7, // FIGURE_ENGINEER
60, // FIGURE_FISHING_BOAT
8, // FIGURE_FISH_GULLS
62, // FIGURE_SHIPWRECK
17, // FIGURE_DOCKER
8, // FIGURE_FLOTSAM
8, // FIGURE_BALLISTA
8, // FIGURE_BOLT
50, // FIGURE_TOWER_SENTRY
8, // FIGURE_JAVELIN
18, // FIGURE_PREFECT
8, // FIGURE_FORT_STANDARD
42, // FIGURE_FORT_JAVELIN
26, // FIGURE_FORT_MOUNTED
41, // FIGURE_FORT_LEGIONARY
12, // FIGURE_MARKET_BUYER
12, // FIGURE_MARKET_TRADER
38, // FIGURE_DELIVERY_BOY
4, // FIGURE_WAREHOUSEMAN
15, // FIGURE_PROTESTER
15, // FIGURE_CRIMINAL
15, // FIGURE_RIOTER
25, // FIGURE_TRADE_CARAVAN
25, // FIGURE_TRADE_CARAVAN_DONKEY
8, // FIGURE_TRADE_SHIP
21, // FIGURE_INDIGENOUS_NATIVE
63, // FIGURE_NATIVE_TRADER
55, // FIGURE_WOLF
54, // FIGURE_SHEEP
56, // FIGURE_ZEBRA
33, // FIGURE_ENEMY_GLADIATOR
21, // FIGURE_ENEMY_BARBARIAN_SWORDSMAN
22, // FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN
30, // FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT
24, // FIGURE_ENEMY_BRITON_SWORDSMAN
23, // FIGURE_ENEMY_BRITON_CHARIOT
24, // FIGURE_ENEMY_CELT_SWORDSMAN
23, // FIGURE_ENEMY_CELT_CHARIOT
24, // FIGURE_ENEMY_PICT_SWORDSMAN
23, // FIGURE_ENEMY_PICT_CHARIOT
29, // FIGURE_ENEMY_EGYPTIAN_SWORDSMAN
28, // FIGURE_ENEMY_EGYPTIAN_CAMEL
31, // FIGURE_ENEMY_ETRUSCAN_SWORDSMAN
32, // FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER
31, // FIGURE_ENEMY_SAMNITE_SWORDSMAN
32, // FIGURE_ENEMY_SAMNITE_SPEAR_THROWER
40, // FIGURE_ENEMY_GAUL_SWORDSMAN
39, // FIGURE_ENEMY_GAUL_AXEMAN
40, // FIGURE_ENEMY_HELVETIUS_SWORDSMAN
39, // FIGURE_ENEMY_HELVETIUS_AXEMAN
35, // FIGURE_ENEMY_HUN_SWORDSMAN
34, // FIGURE_ENEMY_HUN_MOUNTED_ARCHER
35, // FIGURE_ENEMY_GOTH_SWORDSMAN
34, // FIGURE_ENEMY_GOTH_MOUNTED_ARCHER
35, // FIGURE_ENEMY_VISIGOTH_SWORDSMAN
34, // FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER
37, // FIGURE_ENEMY_GREEK_SWORDSMAN
36, // FIGURE_ENEMY_GREEK_SPEAR_THROWER
37, // FIGURE_ENEMY_MACEDONIAN_SWORDSMAN
36, // FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER
20, // FIGURE_ENEMY_NUMIDIAN_SWORDSMAN
20, // FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER
45, // FIGURE_ENEMY_PERGAMUM_SWORDSMAN
44, // FIGURE_ENEMY_PERGAMUM_ARCHER
47, // FIGURE_ENEMY_IBERIAN_SWORDSMAN
46, // FIGURE_ENEMY_IBERIAN_SPEAR_THROWER
47, // FIGURE_ENEMY_JUDEAN_SWORDSMAN
46, // FIGURE_ENEMY_JUDEAN_SPEAR_THROWER
47, // FIGURE_ENEMY_SELEUCID_SWORDSMAN
46, // FIGURE_ENEMY_SELEUCID_SPEAR_THROWER
43, // FIGURE_ENEMY_CAESAR_JAVELIN
27, // FIGURE_ENEMY_CAESAR_MOUNTED
48, // FIGURE_ENEMY_CAESAR_LEGIONARY
8, // FIGURE_ARROW
8, // FIGURE_MAP_FLAG
8, // FIGURE_EXPLOSION
};

static generic_button figure_buttons[] = {
    {26, 46, 50, 50, select_figure, button_none, 0, 0},
    {86, 46, 50, 50, select_figure, button_none, 1, 0},
    {146, 46, 50, 50, select_figure, button_none, 2, 0},
    {206, 46, 50, 50, select_figure, button_none, 3, 0},
    {266, 46, 50, 50, select_figure, button_none, 4, 0},
    {326, 46, 50, 50, select_figure, button_none, 5, 0},
    {386, 46, 50, 50, select_figure, button_none, 6, 0},
};

static struct {
    color_t figure_images[7][48 * 48];
    int focus_button_id;
    building_info_context *context_for_callback;
} data;

static int big_people_image(int type)
{
    return image_group(GROUP_BIG_PEOPLE) + FIGURE_TYPE_TO_BIG_FIGURE_IMAGE[type] - 1;
}

static int inventory_to_resource_id(int value)
{
    switch (value) {
        case INVENTORY_WHEAT: return RESOURCE_WHEAT;
        case INVENTORY_VEGETABLES: return RESOURCE_VEGETABLES;
        case INVENTORY_FRUIT: return RESOURCE_FRUIT;
        case INVENTORY_MEAT: return RESOURCE_MEAT;
        case INVENTORY_WINE: return RESOURCE_WINE;
        case INVENTORY_OIL: return RESOURCE_OIL;
        case INVENTORY_FURNITURE: return RESOURCE_FURNITURE;
        case INVENTORY_POTTERY: return RESOURCE_POTTERY;
        default: return RESOURCE_NONE;
    }
}

static void draw_trader(building_info_context *c, struct figure_t *f)
{
    while (f->type == FIGURE_TRADE_CARAVAN_DONKEY) {
        f = &figures[f->leading_figure_id];
    }
    lang_text_draw(65, f->name_id, c->x_offset + 40, c->y_offset + 110, FONT_NORMAL_BROWN);
    int width = text_draw(get_custom_string(TR_FIGURE_DESC_NOBODY + f->type), c->x_offset + 40, c->y_offset + 130, FONT_NORMAL_BROWN, COLOR_BLACK);
    lang_text_draw(21, empire_objects[f->empire_city_id].city_name_id, c->x_offset + 40 + width, c->y_offset + 130, FONT_NORMAL_BROWN);

    width = lang_text_draw(129, 1, c->x_offset + 40, c->y_offset + 150, FONT_NORMAL_BROWN);
    lang_text_draw_amount(8, 10, f->type == FIGURE_TRADE_SHIP ? 12 : 8, c->x_offset + 40 + width, c->y_offset + 150, FONT_NORMAL_BROWN);

    int trader_id = f->trader_id;
    int text_id;
    if (f->type == FIGURE_TRADE_SHIP) {
        switch (f->action_state) {
            case FIGURE_ACTION_TRADE_SHIP_ANCHORED: text_id = 6; break;
            case FIGURE_ACTION_TRADE_SHIP_MOORED: text_id = 7; break;
            case FIGURE_ACTION_TRADE_SHIP_LEAVING: text_id = 8; break;
            default: text_id = 9; break;
        }
    } else {
        switch (f->action_state) {
            case FIGURE_ACTION_TRADE_CARAVAN_ARRIVING:
                text_id = 12;
                break;
            case FIGURE_ACTION_TRADE_CARAVAN_TRADING:
                text_id = 10;
                break;
            case FIGURE_ACTION_TRADE_CARAVAN_LEAVING:
                if (trader_has_traded(trader_id)) {
                    text_id = 11;
                } else {
                    text_id = 13;
                }
                break;
            default:
                text_id = 11;
                break;
        }
    }
    lang_text_draw(129, text_id, c->x_offset + 40, c->y_offset + 170, FONT_NORMAL_BROWN);
    if (trader_has_traded(trader_id)) {
        // bought
        int y_base = c->y_offset + 192;
        width = lang_text_draw(129, 4, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (trader_bought_resources(trader_id, r)) {
                width += text_draw_number(trader_bought_resources(trader_id, r), ' ', "", c->x_offset + 40 + width, y_base, FONT_NORMAL_BROWN);
                int image_id = image_group(GROUP_RESOURCE_ICONS) + r + resource_image_offset(r, RESOURCE_IMAGE_ICON);
                image_draw(image_id, c->x_offset + 40 + width, y_base - 3);
                width += 25;
            }
        }
        // sold
        y_base = c->y_offset + 213;
        width = lang_text_draw(129, 5, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (trader_sold_resources(trader_id, r)) {
                width += text_draw_number(trader_sold_resources(trader_id, r), ' ', "", c->x_offset + 40 + width, y_base, FONT_NORMAL_BROWN);
                int image_id = image_group(GROUP_RESOURCE_ICONS) + r + resource_image_offset(r, RESOURCE_IMAGE_ICON);
                image_draw(image_id, c->x_offset + 40 + width, y_base - 3);
                width += 25;
            }
        }
    } else { // nothing sold/bought (yet)
        // buying
        int y_base = c->y_offset + 192;
        width = lang_text_draw(129, 2, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (empire_objects[f->empire_city_id].resource_buy_limit[r]) {
                int image_id = image_group(GROUP_RESOURCE_ICONS) + r + resource_image_offset(r, RESOURCE_IMAGE_ICON);
                image_draw(image_id, c->x_offset + 40 + width, y_base - 3);
                width += 25;
            }
        }
        // selling
        y_base = c->y_offset + 213;
        width = lang_text_draw(129, 3, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (empire_objects[f->empire_city_id].resource_sell_limit[r]) {
                int image_id = image_group(GROUP_RESOURCE_ICONS) + r + resource_image_offset(r, RESOURCE_IMAGE_ICON);
                image_draw(image_id, c->x_offset + 40 + width, y_base - 3);
                width += 25;
            }
        }
    }
}

static void draw_enemy(building_info_context *c, struct figure_t *f)
{
    image_draw(image_group(GROUP_BIG_PEOPLE) + FIGURE_TYPE_TO_BIG_FIGURE_IMAGE[f->type] - 1, c->x_offset + 28, c->y_offset + 112);

    lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);

    text_draw(get_custom_string(TR_ENEMY_TYPE_BARBARIAN + f->enemy_type), c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
}

static void draw_animal(building_info_context *c, struct figure_t *f)
{
    image_draw(big_people_image(f->type), c->x_offset + 28, c->y_offset + 112);
    text_draw(get_custom_string(TR_FIGURE_DESC_NOBODY + f->type), c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
}

static void draw_cartpusher(building_info_context *c, struct figure_t *f)
{
    image_draw(big_people_image(f->type), c->x_offset + 28, c->y_offset + 112);

    lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);
    int width = text_draw(get_custom_string(TR_FIGURE_DESC_NOBODY + f->type), c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);

    if (f->action_state != FIGURE_ACTION_DOCKER_IDLING && f->resource_id) {
        int resource = f->resource_id;
        image_draw(image_group(GROUP_RESOURCE_ICONS) +
            resource + resource_image_offset(resource, RESOURCE_IMAGE_ICON),
            c->x_offset + 92 + width, c->y_offset + 135);
    }

    int phrase_height = lang_text_draw_multiline(130, 21 * c->figure.sound_id + c->figure.phrase_id + 1,
        c->x_offset + 90, c->y_offset + 160, BLOCK_SIZE * (c->width_blocks - 8), FONT_NORMAL_BROWN);

    if (!f->building_id) {
        return;
    }
    struct building_t *source_building = &all_buildings[f->building_id];
    struct building_t *target_building = &all_buildings[f->destination_building_id];
    int is_returning = 0;
    switch (f->action_state) {
        case FIGURE_ACTION_CARTPUSHER_RETURNING:
        case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_EMPTY:
        case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_FOOD:
        case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_RESOURCE:
        case FIGURE_ACTION_DOCKER_EXPORT_QUEUE:
        case FIGURE_ACTION_DOCKER_EXPORT_RETURNING:
        case FIGURE_ACTION_DOCKER_IMPORT_RETURNING:
            is_returning = 1;
            break;
    }
    if (f->action_state != FIGURE_ACTION_DOCKER_IDLING) {
        int x_base = c->x_offset + 40;
        int y_base = c->y_offset + 216;
        if (phrase_height > 60) {
            y_base += 8;
        }
        if (is_returning) {
            width = lang_text_draw(129, 16, x_base, y_base, FONT_NORMAL_BROWN);
            width += lang_text_draw(41, source_building->type, x_base + width, y_base, FONT_NORMAL_BROWN);
            width += lang_text_draw(129, 14, x_base + width, y_base, FONT_NORMAL_BROWN);
            lang_text_draw(41, target_building->type, x_base + width, y_base, FONT_NORMAL_BROWN);
        } else {
            width = lang_text_draw(129, 15, x_base, y_base, FONT_NORMAL_BROWN);
            width += lang_text_draw(41, target_building->type, x_base + width, y_base, FONT_NORMAL_BROWN);
            width += lang_text_draw(129, 14, x_base + width, y_base, FONT_NORMAL_BROWN);
            lang_text_draw(41, source_building->type, x_base + width, y_base, FONT_NORMAL_BROWN);
        }
    }
}

static void draw_market_buyer(building_info_context *c, struct figure_t *f)
{
    image_draw(big_people_image(f->type), c->x_offset + 28, c->y_offset + 112);

    lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);
    int width = text_draw(get_custom_string(TR_FIGURE_DESC_NOBODY + f->type), c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);

    if (f->action_state == FIGURE_ACTION_MARKET_BUYER_GOING_TO_STORAGE) {
        width += lang_text_draw(129, 17, c->x_offset + 90 + width, c->y_offset + 139, FONT_NORMAL_BROWN);
        int resource = inventory_to_resource_id(f->collecting_item_id);
        image_draw(image_group(GROUP_RESOURCE_ICONS) + resource + resource_image_offset(resource, RESOURCE_IMAGE_ICON),
            c->x_offset + 90 + width, c->y_offset + 135);
    } else if (f->action_state == FIGURE_ACTION_MARKET_BUYER_RETURNING) {
        width += lang_text_draw(129, 18, c->x_offset + 90 + width, c->y_offset + 139, FONT_NORMAL_BROWN);
        int resource = inventory_to_resource_id(f->collecting_item_id);
        image_draw(image_group(GROUP_RESOURCE_ICONS) + resource + resource_image_offset(resource, RESOURCE_IMAGE_ICON),
            c->x_offset + 90 + width, c->y_offset + 135);
    }
    if (c->figure.phrase_id >= 0) {
        lang_text_draw_multiline(130, 21 * c->figure.sound_id + c->figure.phrase_id + 1,
            c->x_offset + 90, c->y_offset + 160, BLOCK_SIZE * (c->width_blocks - 8), FONT_NORMAL_BROWN);
    }
}

static void draw_normal_figure(building_info_context *c, struct figure_t *f)
{
    int image_id = big_people_image(f->type);
    if (f->action_state == FIGURE_ACTION_PREFECT_GOING_TO_FIRE ||
        f->action_state == FIGURE_ACTION_PREFECT_AT_FIRE) {
        image_id = image_group(GROUP_BIG_PEOPLE) + 18;
    }
    image_draw(image_id, c->x_offset + 28, c->y_offset + 112);

    lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);
    if (f->is_caesar_legion_unit) {
        text_draw(get_custom_string(TR_ENEMY_TYPE_LEGION), c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
    } else {
        text_draw(get_custom_string(TR_FIGURE_DESC_NOBODY + f->type), c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
    }
    if (c->figure.phrase_id >= 0) {
        lang_text_draw_multiline(130, 21 * c->figure.sound_id + c->figure.phrase_id + 1,
            c->x_offset + 90, c->y_offset + 160, BLOCK_SIZE * (c->width_blocks - 8), FONT_NORMAL_BROWN);
    }
}

static void draw_figure_info(building_info_context *c, int figure_id)
{
    button_border_draw(c->x_offset + 24, c->y_offset + 102, BLOCK_SIZE * (c->width_blocks - 3), 138, 0);

    struct figure_t *f = &figures[figure_id];
    if (f->is_empire_trader) {
        draw_trader(c, f);
    } else if (f->is_enemy_unit) {
        draw_enemy(c, f);
    } else if (f->type == FIGURE_SHIPWRECK || f->is_herd_animal) {
        draw_animal(c, f);
    } else if (f->type == FIGURE_CART_PUSHER || f->type == FIGURE_WAREHOUSEMAN || f->type == FIGURE_DOCKER) {
        draw_cartpusher(c, f);
    } else if (f->type == FIGURE_MARKET_BUYER) {
        draw_market_buyer(c, f);
    } else {
        draw_normal_figure(c, f);
    }
}

void window_building_draw_figure_list(building_info_context *c)
{
    inner_panel_draw(c->x_offset + 16, c->y_offset + 40, c->width_blocks - 2, 13);
    if (c->figure.count <= 0) {
        lang_text_draw_centered(70, 0, c->x_offset, c->y_offset + 120,
            BLOCK_SIZE * c->width_blocks, FONT_NORMAL_BROWN);
    } else {
        for (int i = 0; i < c->figure.count; i++) {
            button_border_draw(c->x_offset + 60 * i + 25, c->y_offset + 45, 52, 52, i == c->figure.selected_index);
            graphics_draw_from_buffer(c->x_offset + 27 + 60 * i, c->y_offset + 47, 48, 48, data.figure_images[i]);
        }
        draw_figure_info(c, c->figure.figure_ids[c->figure.selected_index]);
    }
    c->figure.drawn = 1;
}

static void draw_figure_in_city(int figure_id, pixel_coordinate *coord)
{
    int x_cam, y_cam;
    city_view_get_camera(&x_cam, &y_cam);

    int x, y;
    city_view_grid_offset_to_xy_view(figures[figure_id].grid_offset, &x, &y);

    city_view_set_camera(x - 2, y - 6);

    widget_city_draw_for_figure(figure_id, coord);

    city_view_set_camera(x_cam, y_cam);
}

void window_building_prepare_figure_list(building_info_context *c)
{
    if (c->figure.count > 0) {
        pixel_coordinate coord = { 0, 0 };
        for (int i = 0; i < c->figure.count; i++) {
            draw_figure_in_city(c->figure.figure_ids[i], &coord);
            graphics_save_to_buffer(coord.x, coord.y, 48, 48, data.figure_images[i]);
        }
        widget_city_draw();
    }
}

int window_building_handle_mouse_figure_list(const mouse *m, building_info_context *c)
{
    data.context_for_callback = c;
    int handled = generic_buttons_handle_mouse(m, c->x_offset, c->y_offset,
        figure_buttons, c->figure.count, &data.focus_button_id);
    data.context_for_callback = 0;
    return handled;
}

static void select_figure(int index, __attribute__((unused)) int param2)
{
    data.context_for_callback->figure.selected_index = index;
    window_building_play_figure_phrase(data.context_for_callback);
    window_invalidate();
}

void window_building_play_figure_phrase(building_info_context *c)
{
    int figure_id = c->figure.figure_ids[c->figure.selected_index];
    struct figure_t *f = &figures[figure_id];
    c->figure.sound_id = figure_phrase_play(f);
    c->figure.phrase_id = f->phrase_id;
}
