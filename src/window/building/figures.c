#include "figures.h"

#include "building/building.h"
#include "city/view.h"
#include "core/image.h"
#include "empire/object.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/phrase.h"
#include "figure/trader.h"
#include "graphics/graphics.h"
#include "scenario/scenario.h"
#include "widget/city.h"

static void select_figure(int index, int param2);

static struct generic_button_t figure_buttons[] = {
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
    struct building_info_context_t *context_for_callback;
} data;

static char *figure_desc_strings[] = {
    "Nobody", // 0
    "Immigrant", // 1
    "Emigrant", // 2
    "Homeless", // 3
    "Patrician", // 4
    "Cart pusher", // 5
    "Citizen", // 6
    "Barber", // 7
    "Baths worker", // 8
    "Doctor", // 9
    "Surgeon", // 10
    "Priest", // 11
    "School child", // 12
    "Teacher", // 13
    "Librarian", // 14
    "Missionary", // 15
    "Actor", // 16
    "Gladiator", // 17
    "Lion tamer", // 18
    "Charioteer", // 19
    "Hippodrome horse", // 20
    "Tax collector", // 21
    "Engineer", // 22
    "Fishing boat", // 23
    "Seagulls", // 24
    "Shipwreck", // 25
    "Docker", // 26
    "Flotsam", // 27
    "Ballista", // 28
    "Bolt", // 29
    "Sentry", // 30
    "Javelin", // 31
    "Prefect", // 32
    "Standard bearer", // 33
    "Javelin thrower", // 34
    "Mounted auxiliary", // 35
    "Legionary", // 36
    "Market buyer", // 37
    "Market trader", // 38
    "Delivery boy", // 39
    "Warehouseman", // 40
    "Protestor", // 41
    "Criminal", // 42
    "Rioter", // 43
    "Caravan of merchants from", // 44
    "Caravan of merchants from", // 45
    "Trade ship from", // 46
    "Indigenous native", // 47
    "Native trader", // 48
    "Wolf", // 49
    "Sheep", // 50
    "Zebra", // 51
    "Enemy", // 52
    "Arrow", // 53
    "Map flag", // 54
    "Explosion", // 55
};

static char *enemy_desc_strings[] = {
    "A barbarian warrior", // 0
    "A Carthaginian soldier", // 1
    "A Briton", // 2
    "Celtic warrior", // 3
    "Pictish warrior", // 4
    "An Egyptian soldier", // 5
    "An Etruscan soldier", // 6
    "A Samnite soldier", // 7
    "Gaulish warrior", // 8
    "A warrior of the Helvetii", // 9
    "A Hun warrior", // 10
    "A Goth warrior", // 11
    "A Visigoth warrior", // 12
    "A Greek soldier", // 13
    "A Macedonian soldier", // 14
    "A Numidian warrior", // 15
    "A soldier from Pergamum", // 16
    "An Iberian warrior", // 17
    "A Judaean warrior", // 18
    "A Seleucid soldier", // 19
    "Imperial soldier", // 20
};

static int big_people_image(int type)
{
    return image_group(GROUP_BIG_PEOPLE) + figure_properties[type].big_img_id - 1;
}

static void draw_trader(struct building_info_context_t *c, struct figure_t *f)
{
    while (f->type == FIGURE_TRADE_CARAVAN_DONKEY) {
        f = &figures[f->leading_figure_id];
    }
    lang_text_draw(65, f->name_id, c->x_offset + 40, c->y_offset + 110, FONT_NORMAL_BROWN);
    int width = text_draw(figure_desc_strings[f->type], c->x_offset + 40, c->y_offset + 130, FONT_NORMAL_BROWN, COLOR_BLACK);
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
        for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
            if (trader_bought_resources(trader_id, r)) {
                width += text_draw_number(trader_bought_resources(trader_id, r), ' ', "", c->x_offset + 40 + width, y_base, FONT_NORMAL_BROWN);
                image_draw(resource_images[r].icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON), c->x_offset + 40 + width, y_base - 3);
                width += 25;
            }
        }
        // sold
        y_base = c->y_offset + 213;
        width = lang_text_draw(129, 5, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
        for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
            if (trader_sold_resources(trader_id, r)) {
                width += text_draw_number(trader_sold_resources(trader_id, r), ' ', "", c->x_offset + 40 + width, y_base, FONT_NORMAL_BROWN);
                image_draw(resource_images[r].icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON), c->x_offset + 40 + width, y_base - 3);
                width += 25;
            }
        }
    } else { // nothing sold/bought (yet)
        // buying
        int y_base = c->y_offset + 192;
        width = lang_text_draw(129, 2, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
        for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
            if (empire_objects[f->empire_city_id].resource_buy_limit[r]) {
                image_draw(resource_images[r].icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON), c->x_offset + 40 + width, y_base - 3);
                width += 25;
            }
        }
        // selling
        y_base = c->y_offset + 213;
        width = lang_text_draw(129, 3, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
        for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
            if (empire_objects[f->empire_city_id].resource_sell_limit[r]) {
                image_draw(resource_images[r].icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON), c->x_offset + 40 + width, y_base - 3);
                width += 25;
            }
        }
    }
}

static void draw_enemy(struct building_info_context_t *c, struct figure_t *f)
{
    image_draw(image_group(GROUP_BIG_PEOPLE) + figure_properties[f->type].big_img_id - 1, c->x_offset + 28, c->y_offset + 112);

    lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);

    switch (f->type) {
        case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
            text_draw(enemy_desc_strings[0], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN:
        case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
            text_draw(enemy_desc_strings[1], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_BRITON_SWORDSMAN:
        case FIGURE_ENEMY_BRITON_CHARIOT:
            text_draw(enemy_desc_strings[2], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_CELT_SWORDSMAN:
        case FIGURE_ENEMY_CELT_CHARIOT:
            text_draw(enemy_desc_strings[3], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_PICT_SWORDSMAN:
        case FIGURE_ENEMY_PICT_CHARIOT:
            text_draw(enemy_desc_strings[4], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_EGYPTIAN_SWORDSMAN:
        case FIGURE_ENEMY_EGYPTIAN_CAMEL:
            text_draw(enemy_desc_strings[5], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_ETRUSCAN_SWORDSMAN:
        case FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER:
            text_draw(enemy_desc_strings[6], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_SAMNITE_SWORDSMAN:
        case FIGURE_ENEMY_SAMNITE_SPEAR_THROWER:
            text_draw(enemy_desc_strings[7], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_GAUL_SWORDSMAN:
        case FIGURE_ENEMY_GAUL_AXEMAN:
            text_draw(enemy_desc_strings[8], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_HELVETIUS_SWORDSMAN:
        case FIGURE_ENEMY_HELVETIUS_AXEMAN:
            text_draw(enemy_desc_strings[9], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_HUN_SWORDSMAN:
        case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
            text_draw(enemy_desc_strings[10], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_GOTH_SWORDSMAN:
        case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
            text_draw(enemy_desc_strings[11], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
        case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
            text_draw(enemy_desc_strings[12], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_GREEK_SWORDSMAN:
        case FIGURE_ENEMY_GREEK_SPEAR_THROWER:
            text_draw(enemy_desc_strings[13], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_MACEDONIAN_SWORDSMAN:
        case FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER:
            text_draw(enemy_desc_strings[14], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
        case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
            text_draw(enemy_desc_strings[15], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_PERGAMUM_SWORDSMAN:
        case FIGURE_ENEMY_PERGAMUM_ARCHER:
            text_draw(enemy_desc_strings[16], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_IBERIAN_SWORDSMAN:
        case FIGURE_ENEMY_IBERIAN_SPEAR_THROWER:
            text_draw(enemy_desc_strings[17], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_JUDEAN_SWORDSMAN:
        case FIGURE_ENEMY_JUDEAN_SPEAR_THROWER:
            text_draw(enemy_desc_strings[18], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
        case FIGURE_ENEMY_SELEUCID_SWORDSMAN:
        case FIGURE_ENEMY_SELEUCID_SPEAR_THROWER:
            text_draw(enemy_desc_strings[19], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
            break;
    }
}

static void draw_animal(struct building_info_context_t *c, struct figure_t *f)
{
    image_draw(big_people_image(f->type), c->x_offset + 28, c->y_offset + 112);
    text_draw(figure_desc_strings[f->type], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
}

static void draw_cartpusher(struct building_info_context_t *c, struct figure_t *f)
{
    image_draw(big_people_image(f->type), c->x_offset + 28, c->y_offset + 112);

    lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);
    int width = text_draw(figure_desc_strings[f->type], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);

    if (f->action_state != FIGURE_ACTION_DOCKER_IDLING && f->resource_id) {
        int resource = f->resource_id;
        image_draw(resource_images[resource].icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON),
            c->x_offset + 92 + width, c->y_offset + 135);
    }

    int phrase_height = lang_text_draw_multiline(130, 21 * c->figure.sound_id + c->figure.phrase_id + 1,
        c->x_offset + 90, c->y_offset + 160, BLOCK_SIZE * (c->width_blocks - 8), FONT_NORMAL_BROWN);

    if (!f->building_id) {
        return;
    }
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
        struct building_t *source_building = &all_buildings[f->building_id];
        struct building_t *target_building = &all_buildings[f->destination_building_id];
        if (is_returning) {
            width = lang_text_draw(129, 16, x_base, y_base, FONT_NORMAL_BROWN);
            width += text_draw(all_buildings_strings[source_building->type], x_base + width, y_base, FONT_NORMAL_BROWN, COLOR_BLACK);
            width += lang_text_draw(129, 14, x_base + width, y_base, FONT_NORMAL_BROWN);
            text_draw(all_buildings_strings[target_building->type], x_base + width, y_base, FONT_NORMAL_BROWN, COLOR_BLACK);
        } else {
            width = lang_text_draw(129, 15, x_base, y_base, FONT_NORMAL_BROWN);
            width += text_draw(all_buildings_strings[target_building->type], x_base + width, y_base, FONT_NORMAL_BROWN, COLOR_BLACK);
            width += lang_text_draw(129, 14, x_base + width, y_base, FONT_NORMAL_BROWN);
            text_draw(all_buildings_strings[source_building->type], x_base + width, y_base, FONT_NORMAL_BROWN, COLOR_BLACK);
        }
    }
}

static void draw_market_buyer(struct building_info_context_t *c, struct figure_t *f)
{
    image_draw(big_people_image(f->type), c->x_offset + 28, c->y_offset + 112);

    lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);
    int width = text_draw(figure_desc_strings[f->type], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);

    if (f->action_state == FIGURE_ACTION_MARKET_BUYER_GOING_TO_STORAGE) {
        width += lang_text_draw(129, 17, c->x_offset + 90 + width, c->y_offset + 139, FONT_NORMAL_BROWN);
        image_draw(resource_images[f->collecting_item_id + 1].icon_img_id + resource_image_offset(f->collecting_item_id + 1, RESOURCE_IMAGE_ICON),
            c->x_offset + 90 + width, c->y_offset + 135);
    } else if (f->action_state == FIGURE_ACTION_MARKET_BUYER_RETURNING) {
        width += lang_text_draw(129, 18, c->x_offset + 90 + width, c->y_offset + 139, FONT_NORMAL_BROWN);
        image_draw(resource_images[f->collecting_item_id + 1].icon_img_id + resource_image_offset(f->collecting_item_id + 1, RESOURCE_IMAGE_ICON),
            c->x_offset + 90 + width, c->y_offset + 135);
    }
    if (c->figure.phrase_id >= 0) {
        lang_text_draw_multiline(130, 21 * c->figure.sound_id + c->figure.phrase_id + 1,
            c->x_offset + 90, c->y_offset + 160, BLOCK_SIZE * (c->width_blocks - 8), FONT_NORMAL_BROWN);
    }
}

static void draw_normal_figure(struct building_info_context_t *c, struct figure_t *f)
{
    int image_id = big_people_image(f->type);
    if (f->action_state == FIGURE_ACTION_PREFECT_GOING_TO_FIRE ||
        f->action_state == FIGURE_ACTION_PREFECT_AT_FIRE) {
        image_id = image_group(GROUP_BIG_PEOPLE) + 18;
    }
    image_draw(image_id, c->x_offset + 28, c->y_offset + 112);

    lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);
    if (figure_properties[f->type].is_caesar_legion_unit) {
        text_draw(enemy_desc_strings[20], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
    } else {
        text_draw(figure_desc_strings[f->type], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
    }
    if (c->figure.phrase_id >= 0) {
        lang_text_draw_multiline(130, 21 * c->figure.sound_id + c->figure.phrase_id + 1,
            c->x_offset + 90, c->y_offset + 160, BLOCK_SIZE * (c->width_blocks - 8), FONT_NORMAL_BROWN);
    }
}

static void draw_figure_info(struct building_info_context_t *c, int figure_id)
{
    button_border_draw(c->x_offset + 24, c->y_offset + 102, BLOCK_SIZE * (c->width_blocks - 3), 138, 0);

    struct figure_t *f = &figures[figure_id];
    if (figure_properties[f->type].is_empire_trader) {
        draw_trader(c, f);
    } else if (figure_properties[f->type].is_enemy_unit) {
        draw_enemy(c, f);
    } else if (f->type == FIGURE_SHIPWRECK || figure_properties[f->type].is_herd_animal) {
        draw_animal(c, f);
    } else if (f->type == FIGURE_CART_PUSHER || f->type == FIGURE_WAREHOUSEMAN || f->type == FIGURE_DOCKER) {
        draw_cartpusher(c, f);
    } else if (f->type == FIGURE_MARKET_BUYER) {
        draw_market_buyer(c, f);
    } else {
        draw_normal_figure(c, f);
    }
}

void window_building_draw_figure_list(struct building_info_context_t *c)
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

static void draw_figure_in_city(int figure_id, struct pixel_coordinate_t *coord)
{
    int x_cam, y_cam;
    city_view_get_camera(&x_cam, &y_cam);

    int x, y;
    city_view_grid_offset_to_xy_view(figures[figure_id].grid_offset, &x, &y);

    city_view_set_camera(x - 2, y - 6);

    widget_city_draw_for_figure(figure_id, coord);

    city_view_set_camera(x_cam, y_cam);
}

void window_building_prepare_figure_list(struct building_info_context_t *c)
{
    if (c->figure.count > 0) {
        struct pixel_coordinate_t coord = { 0, 0 };
        for (int i = 0; i < c->figure.count; i++) {
            draw_figure_in_city(c->figure.figure_ids[i], &coord);
            graphics_save_to_buffer(coord.x, coord.y, 48, 48, data.figure_images[i]);
        }
        widget_city_draw();
    }
}

int window_building_handle_mouse_figure_list(const struct mouse_t *m, struct building_info_context_t *c)
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

void window_building_play_figure_phrase(struct building_info_context_t *c)
{
    int figure_id = c->figure.figure_ids[c->figure.selected_index];
    struct figure_t *f = &figures[figure_id];
    c->figure.sound_id = figure_phrase_play(f);
    c->figure.phrase_id = f->phrase_id;
}
