#include "build_menu.h"

#include "building/building.h"
#include "city/view.h"
#include "core/image.h"
#include "graphics/graphics.h"
#include "scenario/scenario.h"
#include "widget/city.h"
#include "widget/sidebar/city.h"
#include "window/city.h"

#include <string.h>

#define MENU_X_OFFSET 258
#define MENU_Y_OFFSET 110
#define MENU_ITEM_HEIGHT 24
#define MENU_ITEM_WIDTH 176
#define MENU_CLICK_MARGIN 20

#define MENU_NONE -1

static void button_submenu_or_building(int param1, int param2);

static struct generic_button_t build_menu_buttons[MAX_ITEMS_PER_BUILD_MENU] = {
    {0, 0, 256, 20, button_submenu_or_building, button_none, 0, 0},
    {0, 24, 256, 20, button_submenu_or_building, button_none, 1, 0},
    {0, 48, 256, 20, button_submenu_or_building, button_none, 2, 0},
    {0, 72, 256, 20, button_submenu_or_building, button_none, 3, 0},
    {0, 96, 256, 20, button_submenu_or_building, button_none, 4, 0},
    {0, 120, 256, 20, button_submenu_or_building, button_none, 5, 0},
    {0, 144, 256, 20, button_submenu_or_building, button_none, 6, 0},
    {0, 168, 256, 20, button_submenu_or_building, button_none, 7, 0},
    {0, 192, 256, 20, button_submenu_or_building, button_none, 8, 0},
    {0, 216, 256, 20, button_submenu_or_building, button_none, 9, 0},
    {0, 240, 256, 20, button_submenu_or_building, button_none, 10, 0},
};

static const int Y_MENU_OFFSETS[] = {
    0, 322, 306, 274, 258, 226, 210, 178, 162, 130, 114,
    82, 66, 34, 18, -30, -46, -62, -78, -78, -94,
    -94, -110, -110,
    0, 0, 0, 0, 0, 0
};

const int BUILDING_MENU_SUBMENU_ITEM_MAPPING[BUILD_MENU_BUTTONS_COUNT][MAX_ITEMS_PER_BUILD_MENU][MAX_ITEMS_PER_SUBMENU] = {
    { // MENU_VACANT_HOUSE
        {BUILDING_HOUSE_VACANT_LOT},
    },
    { // MENU_CLEAR_LAND
        {BUILDING_CLEAR_LAND},
    },
    { // MENU_ROAD
        {BUILDING_ROAD},
    },
    { // MENU_WATER
        {BUILDING_RESERVOIR},
        {BUILDING_AQUEDUCT},
        {BUILDING_FOUNTAIN},
        {BUILDING_WELL},
    },
    { // MENU_HEALTH
        {BUILDING_DOCTOR},
        {BUILDING_BATHHOUSE},
        {BUILDING_BARBER},
        {BUILDING_HOSPITAL},
    },
    { // MENU_TEMPLES
        {BUILDING_SMALL_TEMPLE_CERES, BUILDING_SMALL_TEMPLE_NEPTUNE, BUILDING_SMALL_TEMPLE_MERCURY, BUILDING_SMALL_TEMPLE_MARS, BUILDING_SMALL_TEMPLE_VENUS},
        {BUILDING_LARGE_TEMPLE_CERES, BUILDING_LARGE_TEMPLE_NEPTUNE, BUILDING_LARGE_TEMPLE_MERCURY, BUILDING_LARGE_TEMPLE_MARS, BUILDING_LARGE_TEMPLE_VENUS},
        {BUILDING_ORACLE},
    },
    { // MENU_EDUCATION
        {BUILDING_SCHOOL},
        {BUILDING_LIBRARY},
        {BUILDING_ACADEMY},
        {BUILDING_MISSION_POST},
    },
    { // MENU_ENTERTAINMENT
        {BUILDING_THEATER},
        {BUILDING_ACTOR_COLONY},
        {BUILDING_AMPHITHEATER},
        {BUILDING_GLADIATOR_SCHOOL},
        {BUILDING_LION_HOUSE},
        {BUILDING_COLOSSEUM},
        {BUILDING_CHARIOT_MAKER},
        {BUILDING_HIPPODROME},
    },
    { // MENU_ADMINISTRATION
        {BUILDING_GARDENS},
        {BUILDING_PLAZA},
        {BUILDING_SMALL_STATUE},
        {BUILDING_MEDIUM_STATUE},
        {BUILDING_LARGE_STATUE},
        {BUILDING_GOVERNORS_HOUSE},
        {BUILDING_GOVERNORS_VILLA},
        {BUILDING_GOVERNORS_PALACE},
        {BUILDING_FORUM},
        {BUILDING_SENATE},
        {BUILDING_TRIUMPHAL_ARCH},
    },
    { // MENU_ENGINEERING
        {BUILDING_ENGINEERS_POST},
        {BUILDING_LOW_BRIDGE},
        {BUILDING_SHIP_BRIDGE},
        {BUILDING_SHIPYARD},
        {BUILDING_WHARF},
        {BUILDING_DOCK},
    },
    { // MENU_SECURITY
        {BUILDING_PREFECTURE},
        {BUILDING_WALL},
        {BUILDING_TOWER},
        {BUILDING_GATEHOUSE},
        {BUILDING_FORT_LEGIONARIES, BUILDING_FORT_JAVELIN, BUILDING_FORT_MOUNTED},
        {BUILDING_BARRACKS},
        {BUILDING_MILITARY_ACADEMY},
    },
    { // MENU_INDUSTRY
        {BUILDING_WHEAT_FARM, BUILDING_VEGETABLE_FARM, BUILDING_FRUIT_FARM, BUILDING_PIG_FARM, BUILDING_OLIVE_FARM, BUILDING_VINES_FARM},
        {BUILDING_CLAY_PIT, BUILDING_TIMBER_YARD, BUILDING_MARBLE_QUARRY, BUILDING_IRON_MINE},
        {BUILDING_OIL_WORKSHOP, BUILDING_WINE_WORKSHOP, BUILDING_POTTERY_WORKSHOP, BUILDING_FURNITURE_WORKSHOP, BUILDING_WEAPONS_WORKSHOP},
        {BUILDING_MARKET},
        {BUILDING_GRANARY},
        {BUILDING_WAREHOUSE},
    }
};

static char *submenu_strings[] = {
"Menu: Small Temples",  // 0
"Menu: Large Temples",  // 1
"Menu: Forts", // 2
"Menu: Farms",  // 3
"Menu: Raw Materials",  // 4
"Menu: Workshops"  // 5
};

struct build_menu_t build_menus[BUILD_MENU_BUTTONS_COUNT];

static struct {
    int selected_menu;
    int selected_submenu;
    int num_items_to_draw;
    int y_offset;
    int focus_button_id;
} data = { MENU_NONE, MENU_NONE, 0, 0, 0 };

static void map_submenu_items(int menu_index, int submenu_index, int submenu_string_index)
{
    build_menus[menu_index].menu_items[submenu_index].submenu_string = submenu_strings[submenu_string_index];
    for (int k = 0; k < MAX_ITEMS_PER_SUBMENU; k++) {
        if (scenario.allowed_buildings[BUILDING_MENU_SUBMENU_ITEM_MAPPING[menu_index][submenu_index][k]]) {
            build_menus[menu_index].is_enabled = 1;
            build_menus[menu_index].menu_items[submenu_index].building_id = -1; // submenu is enabled (negative number to avoid conflict with actual building types)
            build_menus[menu_index].menu_items[submenu_index].submenu_items[k] = BUILDING_MENU_SUBMENU_ITEM_MAPPING[menu_index][submenu_index][k];
        }
    }
}

void map_building_menu_items(void)
{
    // reset values so as to not carry over between maps
    for (int i = 0; i < BUILD_MENU_BUTTONS_COUNT; i++) {
        memset(&build_menus[i], 0, sizeof(struct build_menu_t));
    }

    for (int i = 0; i < BUILD_MENU_BUTTONS_COUNT; i++) {
        for (int j = 0; j < MAX_ITEMS_PER_BUILD_MENU; j++) {
            if (i == 5 && j == 0) { // small temples
                map_submenu_items(i, j, 0);
            } else if (i == 5 && j == 1) { // large temples
                map_submenu_items(i, j, 1);
            } else if (i == 10 && j == 4) { // forts
                map_submenu_items(i, j, 2);
            } else if (i == 11 && j == 0) { // farms
                map_submenu_items(i, j, 3);
            } else if (i == 11 && j == 1) { // raw materials
                map_submenu_items(i, j, 4);
            } else if (i == 11 && j == 2) { // workshops
                map_submenu_items(i, j, 5);
            } else {
                if (scenario.allowed_buildings[BUILDING_MENU_SUBMENU_ITEM_MAPPING[i][j][0]] && BUILDING_MENU_SUBMENU_ITEM_MAPPING[i][j][0] != BUILDING_TRIUMPHAL_ARCH) {
                    build_menus[i].is_enabled = 1;
                    build_menus[i].menu_items[j].building_id = BUILDING_MENU_SUBMENU_ITEM_MAPPING[i][j][0];
                }
            }
        }
    }
}

static void init(int menu)
{
    data.selected_menu = menu;
    data.selected_submenu = MENU_NONE;
    data.num_items_to_draw = 0;
    for (int j = 0; j < MAX_ITEMS_PER_BUILD_MENU; j++) {
        if (build_menus[menu].menu_items[j].building_id) {
            data.num_items_to_draw++;
        }
    }
    data.y_offset = Y_MENU_OFFSETS[data.num_items_to_draw];
}

static int get_sidebar_x_offset(void)
{
    int view_x, view_y, view_width, view_height;
    city_view_get_viewport(&view_x, &view_y, &view_width, &view_height);
    return view_x + view_width;
}

static void draw_menu_buttons(void)
{
    int x_offset = get_sidebar_x_offset();
    int item_x_align = x_offset - MENU_X_OFFSET - 8;
    int n_skipped_items = 0;
    for (int j = 0; j < data.num_items_to_draw; j++) {
        label_draw(item_x_align, data.y_offset + MENU_Y_OFFSET + MENU_ITEM_HEIGHT * j, 16, data.focus_button_id == j + 1 ? 1 : 2);
        if (data.selected_submenu > MENU_NONE) { // drawing items from a submenu
            for (int k = j + n_skipped_items; k < MAX_ITEMS_PER_SUBMENU; k++) { // draw next enabled item
                if (build_menus[data.selected_menu].menu_items[data.selected_submenu].submenu_items[k]) {
                    text_draw_centered(all_buildings_strings[build_menus[data.selected_menu].menu_items[data.selected_submenu].submenu_items[k]], item_x_align, data.y_offset + MENU_Y_OFFSET + 4 + MENU_ITEM_HEIGHT * j, MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, COLOR_BLACK);
                    text_draw_money(building_properties[build_menus[data.selected_menu].menu_items[data.selected_submenu].submenu_items[k]].cost, x_offset - 82, data.y_offset + MENU_Y_OFFSET + 4 + MENU_ITEM_HEIGHT * j, FONT_NORMAL_GREEN);
                    break;
                } else {
                    n_skipped_items++;
                }
            }
        } else { // drawing items from a menu
            for (int k = j + n_skipped_items; k < MAX_ITEMS_PER_BUILD_MENU; k++) {
                if (build_menus[data.selected_menu].menu_items[k].building_id) { // building or submenu is enabled
                    if (build_menus[data.selected_menu].menu_items[k].building_id == -1) { // submenu item
                        text_draw_centered(build_menus[data.selected_menu].menu_items[k].submenu_string, item_x_align, data.y_offset + MENU_Y_OFFSET + 4 + MENU_ITEM_HEIGHT * j, MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, COLOR_BLACK);
                    } else { // building item
                        text_draw_centered(all_buildings_strings[build_menus[data.selected_menu].menu_items[k].building_id], item_x_align, data.y_offset + MENU_Y_OFFSET + 4 + MENU_ITEM_HEIGHT * j, MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, COLOR_BLACK);
                        text_draw_money(building_properties[build_menus[data.selected_menu].menu_items[k].building_id].cost, x_offset - 82, data.y_offset + MENU_Y_OFFSET + 4 + MENU_ITEM_HEIGHT * j, FONT_NORMAL_GREEN);
                    }
                    break;
                } else {
                    n_skipped_items++;
                }
            }
        }
    }
}

static void draw_foreground(void)
{
    widget_city_draw();
    draw_menu_buttons();
}

static int click_outside_menu(const struct mouse_t *m, int x_offset)
{
    return m->left.went_up &&
        (m->x < x_offset - MENU_X_OFFSET - MENU_CLICK_MARGIN ||
         m->x > x_offset + MENU_CLICK_MARGIN ||
         m->y < data.y_offset + MENU_Y_OFFSET - MENU_CLICK_MARGIN ||
         m->y > data.y_offset + MENU_Y_OFFSET + MENU_CLICK_MARGIN + MENU_ITEM_HEIGHT * data.num_items_to_draw);
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(m, get_sidebar_x_offset() - MENU_X_OFFSET, data.y_offset + MENU_Y_OFFSET, build_menu_buttons, data.num_items_to_draw, &data.focus_button_id)
        || widget_sidebar_city_handle_mouse_build_menu(m)
        ) {
        return;
    }
    if (m->right.went_up || h->escape_pressed || click_outside_menu(m, get_sidebar_x_offset())) {
        data.selected_menu = MENU_NONE;
        data.selected_submenu = MENU_NONE;
        window_city_show();
        return;
    }
}

static void select_building_type(int building_type)
{
    building_construction_set_type(building_type);
    data.selected_menu = MENU_NONE;
    data.selected_submenu = MENU_NONE;
    window_city_show();
}

static void button_submenu_or_building(int param1, __attribute__((unused)) int param2)
{
    int n_skipped_items = 0;
    if (data.selected_submenu > MENU_NONE) { // selecting items from a submenu
        for (int k = 0; k < MAX_ITEMS_PER_SUBMENU; k++) { // select next enabled item
            if (build_menus[data.selected_menu].menu_items[data.selected_submenu].submenu_items[k]) {
                if (k - n_skipped_items >= param1) {
                    select_building_type(build_menus[data.selected_menu].menu_items[data.selected_submenu].submenu_items[k]);
                    break;
                }
            } else {
                n_skipped_items++;
            }
        }
    } else { // selecting items from a menu
        for (int j = 0; j < MAX_ITEMS_PER_BUILD_MENU; j++) { // select next enabled item
            if (build_menus[data.selected_menu].menu_items[j].building_id) { // building or submenu is enabled
                if (j - n_skipped_items >= param1) { // align button index with item
                    if (build_menus[data.selected_menu].menu_items[j].building_id == -1) { // submenu item
                        data.selected_submenu = j;
                        data.num_items_to_draw = 0;
                        for (int k = 0; k < MAX_ITEMS_PER_SUBMENU; k++) {
                            if (build_menus[data.selected_menu].menu_items[j].submenu_items[k]) {
                                data.num_items_to_draw++;
                            }
                        }
                    } else { // building item
                        select_building_type(build_menus[data.selected_menu].menu_items[j].building_id);
                    }
                    break;
                }
            } else {
                n_skipped_items++;
            }
        }
    }
}

int window_build_menu_image(void)
{
    int image_base = image_group(GROUP_PANEL_WINDOWS);
    switch (data.selected_menu) {
        case MENU_VACANT_HOUSE:
            return image_base;
        case MENU_CLEAR_LAND:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT);
            } else {
                return image_base + 11;
            }
        case MENU_ROAD:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT) + 1;
            } else {
                return image_base + 10;
            }
        case MENU_WATER:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT) + 2;
            } else {
                return image_base + 3;
            }
        case MENU_HEALTH:
            return image_base + 5;
        case MENU_TEMPLES:
            return image_base + 1;
        case MENU_EDUCATION:
            return image_base + 6;
        case MENU_ENTERTAINMENT:
            return image_base + 4;
        case MENU_ADMINISTRATION:
            return image_base + 2;
        case MENU_ENGINEERING:
            return image_base + 7;
        case MENU_SECURITY:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT) + 3;
            } else {
                return image_base + 8;
            }
        case MENU_INDUSTRY:
            return image_base + 9;
        default:
            return image_base + 12;
    }
}

void window_build_menu_show(int menu)
{
    if (menu == MENU_NONE || menu == data.selected_menu) {
        window_build_menu_hide();
        return;
    }
    init(menu);
    struct window_type_t window = {
        WINDOW_BUILD_MENU,
        window_city_draw_background,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}

void window_build_menu_hide(void)
{
    data.selected_menu = MENU_NONE;
    window_city_show();
}