#include "build_menu.h"

#include "building/building.h"
#include "building/construction.h"
#include "building/menu.h"
#include "city/view.h"
#include "graphics/generic_button.h"
#include "game/custom_strings.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "widget/city.h"
#include "widget/sidebar/city.h"
#include "window/city.h"

#define MENU_X_OFFSET 258
#define MENU_Y_OFFSET 110
#define MENU_ITEM_HEIGHT 24
#define MENU_ITEM_WIDTH 176
#define MENU_CLICK_MARGIN 20

#define SUBMENU_NONE -1

static void button_menu_index(int param1, int param2);
static void button_menu_item(int item);

static generic_button build_menu_buttons[] = {
    {0, 0, 256, 20, button_menu_index, button_none, 1, 0},
    {0, 24, 256, 20, button_menu_index, button_none, 2, 0},
    {0, 48, 256, 20, button_menu_index, button_none, 3, 0},
    {0, 72, 256, 20, button_menu_index, button_none, 4, 0},
    {0, 96, 256, 20, button_menu_index, button_none, 5, 0},
    {0, 120, 256, 20, button_menu_index, button_none, 6, 0},
    {0, 144, 256, 20, button_menu_index, button_none, 7, 0},
    {0, 168, 256, 20, button_menu_index, button_none, 8, 0},
    {0, 192, 256, 20, button_menu_index, button_none, 9, 0},
    {0, 216, 256, 20, button_menu_index, button_none, 10, 0},
    {0, 240, 256, 20, button_menu_index, button_none, 11, 0},
    {0, 264, 256, 20, button_menu_index, button_none, 12, 0},
    {0, 288, 256, 20, button_menu_index, button_none, 13, 0},
    {0, 312, 256, 20, button_menu_index, button_none, 14, 0},
    {0, 336, 256, 20, button_menu_index, button_none, 15, 0},
    {0, 360, 256, 20, button_menu_index, button_none, 16, 0},
    {0, 384, 256, 20, button_menu_index, button_none, 17, 0},
    {0, 408, 256, 20, button_menu_index, button_none, 18, 0},
    {0, 432, 256, 20, button_menu_index, button_none, 19, 0},
    {0, 456, 256, 20, button_menu_index, button_none, 20, 0},
    {0, 480, 256, 20, button_menu_index, button_none, 21, 0},
    {0, 504, 256, 20, button_menu_index, button_none, 22, 0},
    {0, 528, 256, 20, button_menu_index, button_none, 23, 0},
    {0, 552, 256, 20, button_menu_index, button_none, 24, 0},
    {0, 576, 256, 20, button_menu_index, button_none, 25, 0},
    {0, 600, 256, 20, button_menu_index, button_none, 26, 0},
    {0, 624, 256, 20, button_menu_index, button_none, 27, 0},
    {0, 648, 256, 20, button_menu_index, button_none, 28, 0},
    {0, 672, 256, 20, button_menu_index, button_none, 29, 0},
    {0, 696, 256, 20, button_menu_index, button_none, 30, 0},
};

static const int Y_MENU_OFFSETS[] = {
    0, 322, 306, 274, 258, 226, 210, 178, 162, 130, 114,
    82, 66, 34, 18, -30, -46, -62, -78, -78, -94,
    -94, -110, -110,
    0, 0, 0, 0, 0, 0
};

static struct {
    build_menu_group selected_submenu;
    int num_items;
    int y_offset;

    int focus_button_id;
} data = { SUBMENU_NONE, 0, 0, 0 };

static int init(build_menu_group submenu)
{
    data.selected_submenu = submenu;
    data.num_items = building_menu_count_items(submenu);
    data.y_offset = Y_MENU_OFFSETS[data.num_items];
    if (submenu == BUILD_MENU_VACANT_HOUSE ||
        submenu == BUILD_MENU_CLEAR_LAND ||
        submenu == BUILD_MENU_ROAD) {
        button_menu_item(0);
        return 0;
    } else {
        return 1;
    }
}

int window_build_menu_image(void)
{
    building_type type = building_construction_type();
    int image_base = image_group(GROUP_PANEL_WINDOWS);
    if (type == BUILDING_NONE) {
        return image_base + 12;
    }
    switch (get_building_menu_for_type(type)) {
        default:
        case BUILD_MENU_VACANT_HOUSE:
            return image_base;
        case BUILD_MENU_CLEAR_LAND:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT);
            } else {
                return image_base + 11;
            }
        case BUILD_MENU_ROAD:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT) + 1;
            } else {
                return image_base + 10;
            }
        case BUILD_MENU_WATER:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT) + 2;
            } else {
                return image_base + 3;
            }
        case BUILD_MENU_HEALTH:
            return image_base + 5;
        case BUILD_MENU_TEMPLES:
        case BUILD_MENU_SMALL_TEMPLES:
        case BUILD_MENU_LARGE_TEMPLES:
            return image_base + 1;
        case BUILD_MENU_EDUCATION:
            return image_base + 6;
        case BUILD_MENU_ENTERTAINMENT:
            return image_base + 4;
        case BUILD_MENU_ADMINISTRATION:
            return image_base + 2;
        case BUILD_MENU_ENGINEERING:
            return image_base + 7;
        case BUILD_MENU_SECURITY:
        case BUILD_MENU_FORTS:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT) + 3;
            } else {
                return image_base + 8;
            }
        case BUILD_MENU_INDUSTRY:
        case BUILD_MENU_FARMS:
        case BUILD_MENU_RAW_MATERIALS:
        case BUILD_MENU_WORKSHOPS:
            return image_base + 9;
    }
}

static void draw_background(void)
{
    window_city_draw_background();
}

static int get_sidebar_x_offset(void)
{
    int view_x, view_y, view_width, view_height;
    city_view_get_viewport(&view_x, &view_y, &view_width, &view_height);
    return view_x + view_width;
}

static int is_all_button(building_type type)
{
    return (type == BUILDING_MENU_SMALL_TEMPLES && data.selected_submenu == BUILD_MENU_SMALL_TEMPLES) ||
        (type == BUILDING_MENU_LARGE_TEMPLES && data.selected_submenu == BUILD_MENU_LARGE_TEMPLES);
}

static void draw_menu_buttons(void)
{
    int x_offset = get_sidebar_x_offset();
    int item_index = -1;
    int item_x_align = x_offset - MENU_X_OFFSET - 8;
    for (int i = 0; i < data.num_items; i++) {
        item_index = building_menu_next_index(data.selected_submenu, item_index);
        int type = building_menu_type(data.selected_submenu, item_index);
        label_draw(item_x_align, data.y_offset + MENU_Y_OFFSET + MENU_ITEM_HEIGHT * i, 16,
            data.focus_button_id == i + 1 ? 1 : 2);
        if (is_all_button(type)) {
            text_draw_centered(get_custom_string(TR_BUILD_ALL_TEMPLES),
                item_x_align, data.y_offset + MENU_Y_OFFSET + 3 + MENU_ITEM_HEIGHT * i,
                MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, 0);
        } else {
            lang_text_draw_centered(28, type, item_x_align, data.y_offset + MENU_Y_OFFSET + 3 + MENU_ITEM_HEIGHT * i,
                MENU_ITEM_WIDTH, FONT_NORMAL_GREEN);
        }
        int cost = building_properties[type].cost;
        if (type == BUILDING_FORT) {
            cost = 0;
        } else if (type == BUILDING_MENU_SMALL_TEMPLES && data.selected_submenu == BUILD_MENU_SMALL_TEMPLES) {
            cost = building_properties[BUILDING_SMALL_TEMPLE_CERES].cost;
        } else if (type == BUILDING_MENU_LARGE_TEMPLES && data.selected_submenu == BUILD_MENU_LARGE_TEMPLES) {
            cost = building_properties[BUILDING_LARGE_TEMPLE_CERES].cost;
        }
        if (cost) {
            text_draw_money(cost, x_offset - 82, data.y_offset + MENU_Y_OFFSET + 4 + MENU_ITEM_HEIGHT * i,
                FONT_NORMAL_GREEN);
        }
    }
}

static void draw_foreground(void)
{
    widget_city_draw();
    draw_menu_buttons();
}

static int click_outside_menu(const mouse *m, int x_offset)
{
    return m->left.went_up &&
        (m->x < x_offset - MENU_X_OFFSET - MENU_CLICK_MARGIN ||
         m->x > x_offset + MENU_CLICK_MARGIN ||
         m->y < data.y_offset + MENU_Y_OFFSET - MENU_CLICK_MARGIN ||
         m->y > data.y_offset + MENU_Y_OFFSET + MENU_CLICK_MARGIN + MENU_ITEM_HEIGHT * data.num_items);
}

static int handle_build_submenu(const mouse *m)
{
    return generic_buttons_handle_mouse(
        m, get_sidebar_x_offset() - MENU_X_OFFSET, data.y_offset + MENU_Y_OFFSET,
               build_menu_buttons, data.num_items, &data.focus_button_id);
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (handle_build_submenu(m) ||
        widget_sidebar_city_handle_mouse_build_menu(m)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed || click_outside_menu(m, get_sidebar_x_offset())) {
        data.selected_submenu = SUBMENU_NONE;
        window_city_show();
        return;
    }
}

static int button_index_to_submenu_item(int index)
{
    int item = -1;
    for (int i = 0; i <= index; i++) {
        item = building_menu_next_index(data.selected_submenu, item);
    }
    return item;
}

static void button_menu_index(int param1, __attribute__((unused)) int param2)
{
    button_menu_item(button_index_to_submenu_item(param1 - 1));
}

static int set_submenu_for_type(building_type type)
{
    build_menu_group current_menu = data.selected_submenu;
    switch (type) {
        case BUILDING_MENU_FARMS:
            data.selected_submenu = BUILD_MENU_FARMS;
            break;
        case BUILDING_MENU_RAW_MATERIALS:
            data.selected_submenu = BUILD_MENU_RAW_MATERIALS;
            break;
        case BUILDING_MENU_WORKSHOPS:
            data.selected_submenu = BUILD_MENU_WORKSHOPS;
            break;
        case BUILDING_MENU_SMALL_TEMPLES:
            data.selected_submenu = BUILD_MENU_SMALL_TEMPLES;
            break;
        case BUILDING_MENU_LARGE_TEMPLES:
            data.selected_submenu = BUILD_MENU_LARGE_TEMPLES;
            break;
        case BUILDING_FORT:
            data.selected_submenu = BUILD_MENU_FORTS;
            break;
        default:
            return 0;
    }
    return current_menu != data.selected_submenu;
}

static void button_menu_item(int item)
{
    widget_city_clear_current_tile();

    building_type type = building_menu_type(data.selected_submenu, item);
    building_construction_set_type(type);

    if (set_submenu_for_type(type)) {
        data.num_items = building_menu_count_items(data.selected_submenu);
        data.y_offset = Y_MENU_OFFSETS[data.num_items];
        building_construction_clear_type();
    } else {
        data.selected_submenu = SUBMENU_NONE;
        window_city_show();
    }
}

void window_build_menu_show(int submenu)
{
    if (submenu == SUBMENU_NONE || submenu == data.selected_submenu) {
        window_build_menu_hide();
        return;
    }
    if (init(submenu)) {
        window_type window = {
            WINDOW_BUILD_MENU,
            draw_background,
            draw_foreground,
            handle_input,
            0
        };
        window_show(&window);
    }
}

void window_build_menu_hide(void)
{
    data.selected_submenu = SUBMENU_NONE;
    window_city_show();
}
