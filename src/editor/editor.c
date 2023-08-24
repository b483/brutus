#include "editor/editor.h"

#include "building/building.h"
#include "city/view.h"
#include "city/warning.h"
#include "core/image.h"
#include "core/lang.h"
#include "core/random.h"
#include "core/string.h"
#include "core/time.h"
#include "empire/empire.h"
#include "figuretype/water.h"
#include "game/game.h"
#include "graphics/graphics.h"
#include "editor/editor.h"
#include "empire/object.h"
#include "input/input.h"
#include "map/map.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "widget/city_figure.h"
#include "widget/input_box.h"
#include "widget/minimap.h"
#include "widget/sidebar/common.h"
#include "window/display_options.h"
#include "window/file_dialog.h"
#include "window/main_menu.h"
#include "window/message_dialog.h"
#include "window/numeric_input.h"
#include "window/popup_dialog.h"
#include "window/select_list.h"
#include "window/sound_options.h"
#include "window/speed_options.h"

#include <stdlib.h>

#define MENU_X_OFFSET 170
#define MENU_Y_OFFSET 110
#define MENU_ITEM_HEIGHT 24
#define MENU_ITEM_WIDTH 160
#define MENU_CLICK_MARGIN 20
#define MAX_ITEMS_PER_MENU 16

#define MAX_TILES_OFFSETS 4
#define TERRAIN_NOT_DISPLACEABLE TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP

#define MAX_EMPIRE_WIDTH 2032
#define MAX_EMPIRE_HEIGHT 1136

#define MINIMAP_Y_OFFSET 30

#define MAX_ALLOWED_BUILDINGS 78

#define INVASION_TYPE_MAX_COUNT 5

#define MAX_DEMAND_ROUTES 20
#define DEMAND_ROUTE_MAX_NAME_LENGTH 50

enum {
    MENU_NONE = -1,
    MENU_SHRUB = 0,
    MENU_ELEVATION = 1,
    MENU_ROCK = 2,
    MENU_BRUSH_SIZE = 3,
    MENU_EARTHQUAKE_POINTS = 4,
    MENU_INVASION_POINTS = 5,
    MENU_PEOPLE_POINTS = 6,
    MENU_RIVER_POINTS = 7,
    MENU_NATIVE_BUILDINGS = 8,
    MENU_ANIMAL_POINTS = 9,
};

enum {
    TOOL_GRASS = 0,
    TOOL_SMALL_SHRUB = 1,
    TOOL_MEDIUM_SHRUB = 2,
    TOOL_LARGE_SHRUB = 3,
    TOOL_LARGEST_SHRUB = 4,
    TOOL_WATER = 5,
    TOOL_RAISE_LAND = 6,
    TOOL_LOWER_LAND = 7,
    TOOL_ACCESS_RAMP = 8,
    TOOL_TREES = 9,
    TOOL_SMALL_ROCK = 10,
    TOOL_MEDIUM_ROCK = 11,
    TOOL_LARGE_ROCK = 12,
    TOOL_MEADOW = 13,
    TOOL_ROAD = 14,
    TOOL_EARTHQUAKE_POINT = 15,
    TOOL_INVASION_POINT = 16,
    TOOL_ENTRY_POINT = 17,
    TOOL_EXIT_POINT = 18,
    TOOL_RIVER_ENTRY_POINT = 19,
    TOOL_RIVER_EXIT_POINT = 20,
    TOOL_NATIVE_CENTER = 21,
    TOOL_NATIVE_HUT = 22,
    TOOL_NATIVE_FIELD = 23,
    TOOL_HOUSE_VACANT_LOT = 24,
    TOOL_FISHING_POINT = 25,
    TOOL_HERD_POINT = 26
};

enum {
    RATING_CULTURE,
    RATING_PROSPERITY,
    RATING_PEACE,
    RATING_FAVOR
};

enum {
    CUSTOM_MESSAGE_ATTRIBUTES = 0,
    CUSTOM_MESSAGE_TITLE = 1,
    CUSTOM_MESSAGE_TEXT = 2,
};

static void show_editor_attributes(void);
static void set_trade_route_cost(__attribute__((unused)) int param1, __attribute__((unused)) int param2);
static void show_editor_starting_conditions(void);
static void show_editor_requests(void);
static void show_editor_custom_messages(void);
static void show_editor_earthquakes(void);
static void show_editor_invasions(void);
static void show_editor_price_changes(void);
static void show_editor_demand_changes(void);

static void show_editor_top_menu_window(void);
static void window_editor_map_draw_all(void);

static int focus_button_id_build_menu;
static int selected_submenu_build_menu = MENU_NONE;
static int num_items_build_menu;

static int focus_button_id_attributes;

static const int TILE_X_VIEW_OFFSETS[MAX_TILES_OFFSETS] = { 0, -30, 30, 0 };
static const int TILE_Y_VIEW_OFFSETS[MAX_TILES_OFFSETS] = { 0, 15, 15, 30 };
static const int TILE_GRID_OFFSETS[] = { 0, GRID_SIZE, 1, GRID_SIZE + 1 };
static const int ACCESS_RAMP_TILE_OFFSETS_BY_ORIENTATION[4][6] = {
    {OFFSET(0,1), OFFSET(1,1), OFFSET(0,2), OFFSET(1,2), OFFSET(0,0), OFFSET(1,0)},
    {OFFSET(0,0), OFFSET(0,1), OFFSET(-1,0), OFFSET(-1,1), OFFSET(1,0), OFFSET(1,1)},
    {OFFSET(0,0), OFFSET(1,0), OFFSET(0,-1), OFFSET(1,-1), OFFSET(0,1), OFFSET(1,1)},
    {OFFSET(1,0), OFFSET(1,1), OFFSET(2,0), OFFSET(2,1), OFFSET(0,0), OFFSET(0,1)},
};

static struct {
    int active;
    int type;
    int id;
    int brush_size;
    int build_in_progress;
    int start_elevation;
    struct map_tile_t start_tile;
} tool_data = { 0, TOOL_GRASS, 0, 2, 0, 0, {0} };

static struct {
    uint32_t last_water_animation_time;
    int advance_water_animation;
    int image_id_water_first;
    int image_id_water_last;
} draw_context;

static struct empire_object_t *selected_empire_object_editor;
static int empire_editor_x_min;
static int empire_editor_x_max;
static int empire_editor_y_min;
static int empire_editor_y_max;
static int empire_editor_x_draw_offset;
static int empire_editor_y_draw_offset;
static int empire_editor_focus_trade_route_cost_button_id;
static int empire_editor_focus_expansion_year_button_id;
static int empire_editor_show_battle_objects;

static char brief_description[MAX_BRIEF_DESCRIPTION];
static struct input_box_t scenario_description_input = {
    90, 16, 20, 2, FONT_NORMAL_WHITE, 1,
    brief_description, MAX_BRIEF_DESCRIPTION
};

static int focus_button_id_briefing;
static char briefing[MAX_BRIEFING];
static struct input_box_t scenario_briefing_input = {
    -260, -150, 55, 2, FONT_NORMAL_WHITE, 1,
    briefing, MAX_BRIEFING
};

static int focus_button_id_starting_conditions;
static int focus_button_id_start_year;

static int focus_button_id_win_criteria;

static int focus_button_id_allowed_buildings;

static int focus_button_id_special_events;

static int focus_button_id_requests;
static int id_edit_request;
static int focus_button_id_edit_request;

static int focus_button_id_attr_custom_messages;
static int focus_button_id_title_custom_messages;
static int focus_button_id_text_custom_messages;
static int custom_message_id;
static int custom_message_category;
static int focus_button_id_edit_custom_msg;
static char editor_custom_message_video_file[MAX_CUSTOM_MESSAGE_VIDEO_TEXT];
static char editor_custom_message_title[MAX_CUSTOM_MESSAGE_TEXT];
static char editor_custom_message_text[MAX_CUSTOM_MESSAGE_TEXT];
static struct input_box_t editor_custom_message_input_video_file = { 36, 154, 10, 2, FONT_NORMAL_WHITE, 1, editor_custom_message_video_file, MAX_CUSTOM_MESSAGE_TITLE };
static struct input_box_t editor_custom_message_input_title = { -68, 64, 17, 2, FONT_NORMAL_WHITE, 1, editor_custom_message_title, MAX_CUSTOM_MESSAGE_TITLE };
static struct input_box_t editor_custom_message_input_text = { -68, 64, 46, 2, FONT_NORMAL_WHITE, 1, editor_custom_message_text, MAX_CUSTOM_MESSAGE_TEXT };

static int focus_button_id_earthquakes;
static int id_edit_earthquake;
static char *earthquake_point_names[MAX_EARTHQUAKE_POINTS];
static int focus_button_id_edit_earthquake;

static int focus_button_id_invasions;
static int id_edit_invasion;
static char *invasion_type_names[INVASION_TYPE_MAX_COUNT];
static char *enemy_type_names[ENEMY_TYPE_MAX_COUNT];
static int focus_button_id_edit_invasion;

static int focus_button_id_price_changes;
static int id_edit_price_change;
static int focus_button_id_edit_price_change;

static int focus_button_id_demand_changes;
static char route_display_names[MAX_DEMAND_ROUTES][DEMAND_ROUTE_MAX_NAME_LENGTH] = { {"None available"} };
static int id_demand_route;
static int focus_button_id_demand_route;
static int demand_routes_route_ids[MAX_DEMAND_ROUTES];
static char *demand_routes_route_names[MAX_DEMAND_ROUTES];
static int demand_routes_num_routes;

static int open_sub_menu_top_menu_editor;
static int focus_menu_id_top_menu_editor;
static int focus_sub_menu_id_top_menu_editor;

static struct map_tile_t current_tile;

static char *editor_sidebar_menu_label_strings[] = {
    "Grass",
    "Small shrub",
    "Medium shrub",
    "Large shrub",
    "Largest shrub",
    "Water",
    "Raise land",
    "Lower land",
    "Access ramp",
    "Trees",
    "Small rock",
    "Medium rock",
    "Large rock",
    "Meadow",
    "Road",
    "Earthquake",
    "Invasion point",
    "Entry point",
    "Exit point",
    "River IN",
    "River OUT",
    "Native hut",
    "Native center",
    "Native field",
    "Vacant lot",
    "Fishing waters",
    "Herd point",
};

static char editor_menu_types_strings[][MAX_ITEMS_PER_MENU][19] = {
    {"Small shrub", "Medium shrub", "Large shrub", "Largest shrub"},
    {"Raise land", "Lower land", "Access ramp"},
    {"Small rock", "Medium rock", "Large rock"},
    {"Tiny brush", "Small brush", "Medium brush", "Big brush", "Biggest brush"},
    {"Earthquake point 1", "Earthquake point 2", "Earthquake point 3", "Earthquake point 4", "Earthquake point 5", "Earthquake point 6", "Earthquake point 7", "Earthquake point 8"},
    {"Invasion point 1", "Invasion point 2", "Invasion point 3", "Invasion point 4", "Invasion point 5", "Invasion point 6", "Invasion point 7", "Invasion point 8"},
    {"Entry point", "Exit point"},
    {"River entry", "River exit"},
    {"Native hut", "Native center", "Native field", "Vacant lot"},
    {"Fishing point 1", "Fishing point 2", "Fishing point 3", "Fishing point 4", "Fishing point 5", "Fishing point 6", "Fishing point 7", "Fishing point 8",
    "Herd point 1", "Herd point 2", "Herd point 3", "Herd point 4", "Herd point 5", "Herd point 6", "Herd point 7", "Herd point 8"},
};

static char *attribute_window_strings[] = {
    "Scenario briefing", // 0
    "Requests scheduled", // 1
    "No requests", // 2
    "Messages scheduled", // 3
    "No messages", // 4
    "Earthquakes scheduled", // 5
    "No earthquakes", // 6
    "Invasions scheduled", // 7
    "No invasions", // 8
    "Price changes sch.", // 9
    "No price changes", // 10
    "Demand changes sch.", // 11
    "No demand changes", // 12
};

char *climate_types_strings[] = {
    "Northern provinces", // 0
    "Central provinces", // 1
    "Desert provinces", // 2
};

static char *common_editor_strings[] = {
    "Year offset:", // 0
    "Month:", // 1
    "Jan year 0 invalid", // 2
    "Amount:", // 3
    "Resource:", // 4
    "Hint: 'atL' for new line, 'atP' for new paragraph (replace 'at' with the symbol - will be hidden)", // 5
};

static char *custom_messages_strings[] = {
    "Messages to player", // 0
    "Attributes", // 1
    "Title", // 2
    "Text", // 3
};

static char *earthquakes_strings[] = {
    "Earthquakes", // 0
    "Free earthquake slot", // 1
    "Earthquake point 1", // 2
    "Earthquake point 2", // 3
    "Earthquake point 3", // 4
    "Earthquake point 4", // 5
    "Earthquake point 5", // 6
    "Earthquake point 6", // 7
    "Earthquake point 7", // 8
    "Earthquake point 8", // 9
};

static char *invasions_enemy_type_strings[] = {
    "Barbarians", // 0
    "Carthaginians", // 1
    "Britons", // 2
    "Celts", // 3
    "Picts", // 4
    "Egyptians", // 5
    "Etruscans", // 6
    "Samnites", // 7
    "Gauls", // 8
    "Helvetii", // 9
    "Huns", // 10
    "Goths", // 11
    "Visigoths", // 12
    "Graeci", // 13
    "Macedonians", // 14
    "Numidians", // 15
    "Pergamum", // 16
    "Iberians", // 17
    "Judaeans", // 18
    "Seleucids", // 19
};

static char *invasions_enemy_army_type_strings[] = {
    "No invaders", // 0
    "Local raiders", // 1
    "Enemy army", // 2
    "Caesar's legions", // 3
    "Distant battle", // 4
};

static void editor_tool_set_with_id(int type, int id)
{
    tool_data.active = 1;
    tool_data.type = type;
    tool_data.id = id;
}

static void button_menu_item(int index, __attribute__((unused)) int param2)
{
    current_tile.grid_offset = 0;

    switch (selected_submenu_build_menu) {
        case MENU_SHRUB:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_SMALL_SHRUB, 0); break;
                case 1: editor_tool_set_with_id(TOOL_MEDIUM_SHRUB, 0); break;
                case 2: editor_tool_set_with_id(TOOL_LARGE_SHRUB, 0); break;
                case 3: editor_tool_set_with_id(TOOL_LARGEST_SHRUB, 0); break;
            }
            break;
        case MENU_ELEVATION:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_RAISE_LAND, 0); break;
                case 1: editor_tool_set_with_id(TOOL_LOWER_LAND, 0); break;
                case 2: editor_tool_set_with_id(TOOL_ACCESS_RAMP, 0); break;
            }
            break;
        case MENU_ROCK:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_SMALL_ROCK, 0); break;
                case 1: editor_tool_set_with_id(TOOL_MEDIUM_ROCK, 0); break;
                case 2: editor_tool_set_with_id(TOOL_LARGE_ROCK, 0); break;
            }
            break;
        case MENU_BRUSH_SIZE:
            tool_data.brush_size = index;
            break;
        case MENU_EARTHQUAKE_POINTS:
            editor_tool_set_with_id(TOOL_EARTHQUAKE_POINT, index);
            break;
        case MENU_INVASION_POINTS:
            editor_tool_set_with_id(TOOL_INVASION_POINT, index);
            break;
        case MENU_PEOPLE_POINTS:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_ENTRY_POINT, 0); break;
                case 1: editor_tool_set_with_id(TOOL_EXIT_POINT, 0); break;
            }
            break;
        case MENU_RIVER_POINTS:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_RIVER_ENTRY_POINT, 0); break;
                case 1: editor_tool_set_with_id(TOOL_RIVER_EXIT_POINT, 0); break;
            }
            break;
        case MENU_NATIVE_BUILDINGS:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_NATIVE_HUT, 0); break;
                case 1: editor_tool_set_with_id(TOOL_NATIVE_CENTER, 0); break;
                case 2: editor_tool_set_with_id(TOOL_NATIVE_FIELD, 0); break;
                case 3: editor_tool_set_with_id(TOOL_HOUSE_VACANT_LOT, 0); break;
            }
            break;
        case MENU_ANIMAL_POINTS:
            if (index < MAX_FISH_POINTS) {
                editor_tool_set_with_id(TOOL_FISHING_POINT, index);
            } else {
                editor_tool_set_with_id(TOOL_HERD_POINT, index - MAX_FISH_POINTS);
            }
            break;
    }
    selected_submenu_build_menu = MENU_NONE;
    show_editor_map();
}

static struct generic_button_t build_menu_buttons[] = {
    {0, 0, 160, 20, button_menu_item, button_none, 0, 0},
    {0, 24, 160, 20, button_menu_item, button_none, 1, 0},
    {0, 48, 160, 20, button_menu_item, button_none, 2, 0},
    {0, 72, 160, 20, button_menu_item, button_none, 3, 0},
    {0, 96, 160, 20, button_menu_item, button_none, 4, 0},
    {0, 120, 160, 20, button_menu_item, button_none, 5, 0},
    {0, 144, 160, 20, button_menu_item, button_none, 6, 0},
    {0, 168, 160, 20, button_menu_item, button_none, 7, 0},
    {0, 192, 160, 20, button_menu_item, button_none, 8, 0},
    {0, 216, 160, 20, button_menu_item, button_none, 9, 0},
    {0, 240, 160, 20, button_menu_item, button_none, 10, 0},
    {0, 264, 160, 20, button_menu_item, button_none, 11, 0},
    {0, 288, 160, 20, button_menu_item, button_none, 12, 0},
    {0, 312, 160, 20, button_menu_item, button_none, 13, 0},
    {0, 336, 160, 20, button_menu_item, button_none, 14, 0},
    {0, 360, 160, 20, button_menu_item, button_none, 15, 0}
};

static int get_sidebar_x_offset(void)
{
    int view_x, view_y, view_width, view_height;
    city_view_get_viewport(&view_x, &view_y, &view_width, &view_height);
    return view_x + view_width;
}

static void draw_footprint(int x, int y, int grid_offset)
{
    if (grid_offset < 0) {
        // Outside map: draw black tile
        image_draw_isometric_footprint_from_draw_tile(image_group(GROUP_TERRAIN_BLACK), x, y, 0);
    } else if (map_property_is_draw_tile(grid_offset)) {
        // Valid grid_offset and leftmost tile -> draw
        color_t color_mask = 0;
        int image_id = map_image_at(grid_offset);
        if (draw_context.advance_water_animation &&
            image_id >= draw_context.image_id_water_first &&
            image_id <= draw_context.image_id_water_last) {
            image_id++;
            if (image_id > draw_context.image_id_water_last) {
                image_id = draw_context.image_id_water_first;
            }
            map_image_set(grid_offset, image_id);
        }
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, color_mask);
    }
}

static void draw_flags(int x, int y, int grid_offset)
{
    int figure_id = map_figure_at(grid_offset);
    while (figure_id) {
        struct figure_t *f = &figures[figure_id];
        if (!f->is_invisible) {
            city_draw_figure(f, x, y, 0);
        }
        figure_id = f->next_figure_id_on_same_tile;
    }
}

static void draw_top(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    int image_id = map_image_at(grid_offset);
    color_t color_mask = 0;
    image_draw_isometric_top_from_draw_tile(image_id, x, y, color_mask);
}

static void draw_flat_tile(int x, int y, color_t color_mask)
{
    if (color_mask == COLOR_MASK_GREEN && scenario.climate != CLIMATE_DESERT) {
        image_draw_blend_alpha(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, ALPHA_MASK_SEMI_TRANSPARENT | color_mask);
    } else {
        image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, color_mask);
    }
}

static void draw_partially_blocked(int x, int y, int num_tiles, int *blocked_tiles)
{
    for (int i = 0; i < num_tiles; i++) {
        int x_offset = x + TILE_X_VIEW_OFFSETS[i];
        int y_offset = y + TILE_Y_VIEW_OFFSETS[i];
        if (blocked_tiles[i]) {
            draw_flat_tile(x_offset, y_offset, COLOR_MASK_RED);
        } else {
            draw_flat_tile(x_offset, y_offset, COLOR_MASK_GREEN);
        }
    }
}

static void draw_building_image(int image_id, int x, int y)
{
    image_draw_isometric_footprint(image_id, x, y, COLOR_MASK_GREEN);
    image_draw_isometric_top(image_id, x, y, COLOR_MASK_GREEN);
}

static int can_place_building_editor(const struct map_tile_t *tile, int num_tiles, int *blocked_tiles)
{
    int blocked = 0;
    for (int i = 0; i < num_tiles; i++) {
        int tile_offset = tile->grid_offset + TILE_GRID_OFFSETS[i];
        if ((terrain_grid.items[tile_offset] & TERRAIN_NOT_CLEAR) || map_has_figure_at(tile_offset)) {
            blocked = 1;
            if (blocked_tiles) blocked_tiles[i] = 1;
        } else {
            if (blocked_tiles) blocked_tiles[i] = 0;
        }
    }
    return !blocked;
}

static void draw_building(const struct map_tile_t *tile, int x_view, int y_view, int type)
{
    int num_tiles = building_properties[type].size * building_properties[type].size;
    int blocked_tiles[MAX_TILES_OFFSETS];
    int blocked = !can_place_building_editor(tile, num_tiles, blocked_tiles);

    if (blocked) {
        draw_partially_blocked(x_view, y_view, num_tiles, blocked_tiles);
    } else if (tool_data.build_in_progress) {
        int image_id = image_group(GROUP_TERRAIN_OVERLAY);
        for (int i = 0; i < num_tiles; i++) {
            int x_offset = x_view + TILE_X_VIEW_OFFSETS[i];
            int y_offset = y_view + TILE_Y_VIEW_OFFSETS[i];
            image_draw_isometric_footprint(image_id, x_offset, y_offset, 0);
        }
    } else {
        int image_id;
        if (type == BUILDING_NATIVE_CROPS) {
            image_id = image_group(GROUP_EDITOR_BUILDING_CROPS);
        } else if (type == BUILDING_HOUSE_VACANT_LOT) {
            image_id = image_group(GROUP_EDITOR_BUILDING_NATIVE) - 4;
        } else {
            image_id = image_group(building_properties[type].image_group) + building_properties[type].image_offset;
        }
        draw_building_image(image_id, x_view, y_view);
    }
}

static int can_place_access_ramp_editor(const struct map_tile_t *tile, int *orientation_index)
{
    if (!map_grid_is_inside(tile->x, tile->y, 2)) {
        return 0;
    }
    for (int orientation = 0; orientation < 4; orientation++) {
        int right_tiles = 0;
        int wrong_tiles = 0;
        int top_elevation = 0;
        for (int index = 0; index < 6; index++) {
            int tile_offset = tile->grid_offset + ACCESS_RAMP_TILE_OFFSETS_BY_ORIENTATION[orientation][index];
            int elevation = terrain_elevation.items[tile_offset];
            if (index < 2) {
                if (map_terrain_is(tile_offset, TERRAIN_ELEVATION)) {
                    right_tiles++;
                } else {
                    wrong_tiles++;
                }
                top_elevation = elevation;
            } else if (index < 4) {
                if (map_terrain_is(tile_offset, TERRAIN_ELEVATION)) {
                    if (elevation == top_elevation) {
                        wrong_tiles++;
                    } else {
                        right_tiles++;
                    }
                } else if (elevation >= top_elevation) {
                    right_tiles++;
                } else {
                    wrong_tiles++;
                }
            } else {
                if (map_terrain_is(tile_offset, TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP)) {
                    wrong_tiles++;
                } else if (elevation >= top_elevation) {
                    wrong_tiles++;
                } else {
                    right_tiles++;
                }
            }
        }
        if (right_tiles == 6) {
            if (orientation_index) {
                *orientation_index = orientation;
            }
            return 1;
        }
    }
    return 0;
}

static int is_clear_terrain(const struct map_tile_t *tile, int *warning)
{
    int result = !map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR ^ TERRAIN_ROAD);
    if (!result && warning) {
        *warning = WARNING_EDITOR_CANNOT_PLACE;
    }
    return result;
}

static int is_edge(const struct map_tile_t *tile, int *warning)
{
    int result = tile->x == 0 || tile->y == 0 || tile->x == map_data.width - 1 || tile->y == map_data.height - 1;
    if (!result && warning) {
        *warning = WARNING_EDITOR_NEED_MAP_EDGE;
    }
    return result;
}

static int can_place_flag_editor(int type, const struct map_tile_t *tile, int *warning)
{
    switch (type) {
        case TOOL_ENTRY_POINT:
        case TOOL_EXIT_POINT:
        case TOOL_INVASION_POINT:
            return is_clear_terrain(tile, warning) && is_edge(tile, warning);
        case TOOL_EARTHQUAKE_POINT:
        case TOOL_HERD_POINT:
            return is_clear_terrain(tile, warning);
        case TOOL_FISHING_POINT:
            int is_water = map_terrain_is(tile->grid_offset, TERRAIN_WATER);
            if (!is_water && warning) {
                *warning = WARNING_EDITOR_NEED_OPEN_WATER;
            }
            return is_water;
        case TOOL_RIVER_ENTRY_POINT:
        case TOOL_RIVER_EXIT_POINT:
            int is_deep_water = map_terrain_is(tile->grid_offset, TERRAIN_WATER) && map_terrain_count_directly_adjacent_with_type(tile->grid_offset, TERRAIN_WATER) == 4;
            if (!is_deep_water && warning) {
                *warning = WARNING_EDITOR_NEED_OPEN_WATER;
            }
            return is_edge(tile, warning) && is_deep_water;
        default:
            return 0;
    }
}

static void draw_editor_map(void)
{
    int x, y, width, height;
    city_view_get_viewport(&x, &y, &width, &height);
    graphics_set_clip_rectangle(x, y, width, height);

    draw_context.advance_water_animation = 0;
    uint32_t now = time_get_millis();
    if (now - draw_context.last_water_animation_time > 60) {
        draw_context.last_water_animation_time = now;
        draw_context.advance_water_animation = 1;
    }
    draw_context.image_id_water_first = image_group(GROUP_TERRAIN_WATER);
    draw_context.image_id_water_last = 5 + draw_context.image_id_water_first;
    city_view_foreach_map_tile(draw_footprint);
    city_view_foreach_valid_map_tile(draw_flags, draw_top, 0);

    // draw editor tool
    if (!current_tile.grid_offset || scroll_in_progress() || !tool_data.active) {
        graphics_reset_clip_rectangle();
        return;
    }
    city_view_get_selected_tile_pixels(&x, &y);
    switch (tool_data.type) {
        case TOOL_NATIVE_CENTER:
            draw_building(&current_tile, x, y, BUILDING_NATIVE_MEETING);
            break;
        case TOOL_NATIVE_HUT:
            draw_building(&current_tile, x, y, BUILDING_NATIVE_HUT);
            break;
        case TOOL_NATIVE_FIELD:
            draw_building(&current_tile, x, y, BUILDING_NATIVE_CROPS);
            break;
        case TOOL_HOUSE_VACANT_LOT:
            draw_building(&current_tile, x, y, BUILDING_HOUSE_VACANT_LOT);
            break;
        case TOOL_EARTHQUAKE_POINT:
        case TOOL_ENTRY_POINT:
        case TOOL_EXIT_POINT:
        case TOOL_RIVER_ENTRY_POINT:
        case TOOL_RIVER_EXIT_POINT:
        case TOOL_INVASION_POINT:
        case TOOL_FISHING_POINT:
        case TOOL_HERD_POINT:
            draw_flat_tile(x, y, can_place_flag_editor(tool_data.type, &current_tile, 0) ? COLOR_MASK_GREEN : COLOR_MASK_RED);
            break;
        case TOOL_ACCESS_RAMP:
            int orientation;
            if (can_place_access_ramp_editor(&current_tile, &orientation)) {
                int image_id = image_group(GROUP_TERRAIN_ACCESS_RAMP) + orientation;
                draw_building_image(image_id, x, y);
            } else {
                int blocked[4] = { 1, 1, 1, 1 };
                draw_partially_blocked(x, y, 4, blocked);
            }
            break;
        case TOOL_GRASS:
        case TOOL_SMALL_SHRUB:
        case TOOL_MEDIUM_SHRUB:
        case TOOL_LARGE_SHRUB:
        case TOOL_LARGEST_SHRUB:
        case TOOL_TREES:
        case TOOL_WATER:
        case TOOL_RAISE_LAND:
        case TOOL_LOWER_LAND:
        case TOOL_SMALL_ROCK:
        case TOOL_MEDIUM_ROCK:
        case TOOL_LARGE_ROCK:
        case TOOL_MEADOW:
            // draw brush
            draw_flat_tile(x, y, COLOR_MASK_GREEN);
            int tiles_remaining = tool_data.brush_size;
            while (tiles_remaining) {
                for (int i = 1; i <= tiles_remaining; i++) {
                    draw_flat_tile(x + HALF_TILE_WIDTH_PIXELS * (i + (tool_data.brush_size - tiles_remaining)), y - HALF_TILE_HEIGHT_PIXELS * (i - (tool_data.brush_size - tiles_remaining)), COLOR_MASK_GREEN);
                    draw_flat_tile(x + HALF_TILE_WIDTH_PIXELS * (i - (tool_data.brush_size - tiles_remaining)), y + HALF_TILE_HEIGHT_PIXELS * (i + (tool_data.brush_size - tiles_remaining)), COLOR_MASK_GREEN);
                    draw_flat_tile(x - HALF_TILE_WIDTH_PIXELS * (i + (tool_data.brush_size - tiles_remaining)), y + HALF_TILE_HEIGHT_PIXELS * (i - (tool_data.brush_size - tiles_remaining)), COLOR_MASK_GREEN);
                    draw_flat_tile(x - HALF_TILE_WIDTH_PIXELS * (i - (tool_data.brush_size - tiles_remaining)), y - HALF_TILE_HEIGHT_PIXELS * (i + (tool_data.brush_size - tiles_remaining)), COLOR_MASK_GREEN);
                }
                tiles_remaining--;
            }
            break;
        case TOOL_ROAD:
            int blocked = 0;
            int image_id = 0;
            if (map_terrain_is(current_tile.grid_offset, TERRAIN_NOT_CLEAR)) {
                blocked = 1;
            } else {
                image_id = image_group(GROUP_TERRAIN_ROAD);
                if (!map_terrain_has_adjacent_x_with_type(current_tile.grid_offset, TERRAIN_ROAD) &&
                    map_terrain_has_adjacent_y_with_type(current_tile.grid_offset, TERRAIN_ROAD)) {
                    image_id++;
                }
            }
            if (blocked) {
                draw_flat_tile(x, y, COLOR_MASK_RED);
            } else {
                draw_building_image(image_id, x, y);
            }
            break;
    }

    graphics_reset_clip_rectangle();
}

static void draw_foreground_build_menu(void)
{
    draw_editor_map();
    // draw menu buttons
    int x_offset = get_sidebar_x_offset();
    for (int i = 0; i < num_items_build_menu; i++) {
        label_draw(x_offset - MENU_X_OFFSET, 180 + MENU_Y_OFFSET + MENU_ITEM_HEIGHT * i, 10, focus_button_id_build_menu == i + 1 ? 1 : 2);
        text_draw_centered(editor_menu_types_strings[selected_submenu_build_menu][i], x_offset - MENU_X_OFFSET, 180 + MENU_Y_OFFSET + 3 + MENU_ITEM_HEIGHT * i,
            MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, COLOR_BLACK);
    }
}

static void handle_input_build_menu(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(m, get_sidebar_x_offset() - MENU_X_OFFSET, 180 + MENU_Y_OFFSET, build_menu_buttons, num_items_build_menu, &focus_button_id_build_menu)) {
        return;
    }
    if (m->right.went_up
    || h->escape_pressed
    || (m->left.went_up
        && (m->x < get_sidebar_x_offset() - MENU_X_OFFSET - MENU_CLICK_MARGIN || m->x > get_sidebar_x_offset() + MENU_CLICK_MARGIN
            || m->y < 180 + MENU_Y_OFFSET - MENU_CLICK_MARGIN || m->y > 180 + MENU_Y_OFFSET + MENU_CLICK_MARGIN + MENU_ITEM_HEIGHT * num_items_build_menu)) // click outside menu
    ) {
        selected_submenu_build_menu = MENU_NONE;
        show_editor_map();
    }
}

static void stop_brief_description_box_input(void)
{
    input_box_stop(&scenario_description_input);
    if (!string_equals(scenario.brief_description, brief_description)) {
        string_copy(brief_description, scenario.brief_description, MAX_BRIEF_DESCRIPTION);
        scenario.is_saved = 0;
    }
}

static void stop_briefing_box_input(void)
{
    input_box_stop(&scenario_briefing_input);
    if (!string_equals(scenario.briefing, briefing)) {
        string_copy(briefing, scenario.briefing, MAX_BRIEFING);
        scenario.is_saved = 0;
    }
}

static void start_briefing_box_input(void)
{
    string_copy(scenario.briefing, briefing, MAX_BRIEFING);
    input_box_start(&scenario_briefing_input);
}

static void button_reset_briefing_text(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    briefing[0] = '\0';
    stop_briefing_box_input();
    rich_text_reset(0);
    start_briefing_box_input();
}

static struct generic_button_t button_reset[] = {
    {455, 540, 190, 35, button_reset_briefing_text, button_none, 0, 0}
};

static void draw_foreground_briefing(void)
{
    graphics_in_dialog();

    outer_panel_draw(-300, -165, 60, 4);
    input_box_draw(&scenario_briefing_input);

    outer_panel_draw(-300, -100, 60, 43);
    // Formatted typed in text
    rich_text_set_fonts(FONT_NORMAL_BLACK, FONT_NORMAL_RED, 6);
    rich_text_init(briefing, -350, -75, 60, 38, 0);
    graphics_set_clip_rectangle(-300, -100, 970, 635);
    rich_text_draw(briefing, -260, -70, 912, 35, 0);
    rich_text_reset_lines_only();
    rich_text_draw_scrollbar();
    graphics_reset_clip_rectangle();

    // @L, @P hint
    text_draw(common_editor_strings[5], -285, 550, FONT_NORMAL_PLAIN, COLOR_TOOLTIP);

    // Reset briefing
    button_border_draw(455, 540, 190, 35, focus_button_id_briefing);
    text_draw_centered("Reset briefing", 455, 545, 190, FONT_LARGE_PLAIN, COLOR_RED);

    graphics_reset_dialog();
}

static void handle_input_briefing(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (m->right.went_up || h->escape_pressed) {
        stop_briefing_box_input();
        rich_text_reset(0);
        show_editor_attributes();
    }
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, button_reset, 1, &focus_button_id_briefing)) {
        return;
    }
    rich_text_handle_mouse(m_dialog);
}

static void clear_state(void)
{
    open_sub_menu_top_menu_editor = 0;
    focus_menu_id_top_menu_editor = 0;
    focus_sub_menu_id_top_menu_editor = 0;
}

static void map_size_selected(int size)
{
    clear_state();
    if (size >= 0 && size <= 5) {
        game_file_editor_create_scenario(size);
        show_editor_map();
    } else {
        window_go_back();
    }
}

static void menu_file_new_map(__attribute__((unused)) int param)
{
    window_select_list_show(50, 50, 33, 7, map_size_selected);
}

static void menu_file_load_map(__attribute__((unused)) int param)
{
    clear_state();
    show_editor_map();
    window_file_dialog_show(FILE_TYPE_SCENARIO, FILE_DIALOG_LOAD);
}

static void menu_file_save_map(__attribute__((unused)) int param)
{
    clear_state();
    show_editor_map();
    window_file_dialog_show(FILE_TYPE_SCENARIO, FILE_DIALOG_SAVE);
}

static void exit_editor_to_main_menu(void)
{
    if (!reload_language(0, 0)) {
        return;
    }
    editor_set_active(0);
    window_main_menu_show(1);
}

static void request_exit_editor(void)
{
    if (scenario.is_saved) {
        exit_editor_to_main_menu();
    } else {
        window_popup_dialog_show(POPUP_DIALOG_EDITOR_QUIT_WITHOUT_SAVING, exit_editor_to_main_menu, 1);
    }
}

static void menu_file_exit_editor(__attribute__((unused)) int param)
{
    clear_state();
    request_exit_editor();
}

static struct menu_item_t menu_file[] = {
    {7, 1, menu_file_new_map, 0, 0},
    {7, 2, menu_file_load_map, 0, 0},
    {7, 3, menu_file_save_map, 0, 0},
    {7, 4, menu_file_exit_editor, 0, 0},
};

static void menu_options_display_editor(__attribute__((unused)) int param)
{
    clear_state();
    show_editor_map();
    window_display_options_show(show_editor_map);
}

static void menu_options_sound_editor(__attribute__((unused)) int param)
{
    clear_state();
    show_editor_map();
    window_sound_options_show(1);
}

static void menu_options_speed_editor(__attribute__((unused)) int param)
{
    clear_state();
    show_editor_map();
    window_speed_options_show(1);
}

static struct menu_item_t menu_options[] = {
    {2, 1, menu_options_display_editor, 0, 0},
    {2, 2, menu_options_sound_editor, 0, 0},
    {2, 3, menu_options_speed_editor, 0, 0},
};

static void menu_editor_help(__attribute__((unused)) int param)
{
    clear_state();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_EDITOR_HELP, window_editor_map_draw_all);
}

static void menu_help_editor_about(__attribute__((unused)) int param)
{
    clear_state();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_EDITOR_ABOUT, window_editor_map_draw_all);
}

static struct menu_item_t menu_help[] = {
    {3, 1, menu_editor_help, 0, 0},
    {3, 7, menu_help_editor_about, 0, 0},
};

static void menu_resets_herds(__attribute__((unused)) int param)
{
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        scenario.herd_points[i].x = -1;
        scenario.herd_points[i].y = -1;
    }
    scenario.is_saved = 0;
    clear_state();
    window_go_back();
}

static void menu_resets_fish(__attribute__((unused)) int param)
{
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        scenario.fishing_points[i].x = -1;
        scenario.fishing_points[i].y = -1;
    }
    scenario.is_saved = 0;
    clear_state();
    window_go_back();
}

static void menu_resets_invasions(__attribute__((unused)) int param)
{
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        scenario.invasion_points[i].x = -1;
        scenario.invasion_points[i].y = -1;
    }
    scenario.is_saved = 0;
    clear_state();
    window_go_back();
}

static void menu_resets_earthquakes(__attribute__((unused)) int param)
{
    for (int i = 0; i < MAX_EARTHQUAKE_POINTS; i++) {
        scenario.earthquake_points[i].x = -1;
        scenario.earthquake_points[i].y = -1;
    }
    scenario.is_saved = 0;
    clear_state();
    window_go_back();
}

static struct menu_item_t menu_resets[] = {
    {10, 1, menu_resets_herds, 0, 0},
    {10, 2, menu_resets_fish, 0, 0},
    {10, 3, menu_resets_invasions, 0, 0},
    {10, 4, menu_resets_earthquakes, 0, 0},
};

static struct menu_bar_item_t top_menu_editor[] = {
    {7, menu_file, 4, 0, 0, 0, 0},
    {2, menu_options, 3, 0, 0, 0, 0},
    {3, menu_help, 2, 0, 0, 0, 0},
    {10, menu_resets, 4, 0, 0, 0, 0},
};

static void hide_editor_build_menu(void)
{
    selected_submenu_build_menu = MENU_NONE;
    show_editor_map();
}

static void button_attributes(int show, __attribute__((unused)) int param2)
{
    hide_editor_build_menu();
    if (show) {
        if (!window_is(WINDOW_EDITOR_ATTRIBUTES)) {
            show_editor_attributes();
        }
    } else {
        if (!window_is(WINDOW_EDITOR_MAP)) {
            show_editor_map();
        }
    }
}

static int map_viewport_width(void)
{
    return empire_editor_x_max - empire_editor_x_min - 32;
}

static int map_viewport_height(void)
{
    return empire_editor_y_max - empire_editor_y_min - 136;
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

static struct arrow_button_t arrow_buttons_empire[] = {
    {28, -52, 17, 24, button_change_empire, -1, 0, 0, 0},
    {52, -52, 15, 24, button_change_empire, 1, 0, 0, 0}
};

static void set_city_type(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (selected_empire_object_editor->city_type == EMPIRE_CITY_DISTANT_ROMAN) {
        selected_empire_object_editor->city_type = EMPIRE_CITY_TRADE;
    } else if (selected_empire_object_editor->city_type == EMPIRE_CITY_TRADE) {
        selected_empire_object_editor->city_type = EMPIRE_CITY_FUTURE_TRADE;
    } else {
        selected_empire_object_editor->city_type = EMPIRE_CITY_DISTANT_ROMAN;
    }
    // fix graphics for city sprite and flag color when changing city types
    if (selected_empire_object_editor->city_type == EMPIRE_CITY_TRADE || selected_empire_object_editor->city_type == EMPIRE_CITY_FUTURE_TRADE) {
        selected_empire_object_editor->image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
        selected_empire_object_editor->expanded.image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
    } else if (selected_empire_object_editor->city_type == EMPIRE_CITY_DISTANT_ROMAN) {
        selected_empire_object_editor->image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
        selected_empire_object_editor->expanded.image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
    }
    window_request_refresh();
}

static struct arrow_button_t arrow_buttons_set_city_type[] = {
    {0, 0, 21, 24, set_city_type, 0, 0, 0, 0}
};

static void set_resource_sell_limit(int resource, __attribute__((unused)) int param2)
{
    switch (selected_empire_object_editor->resource_sell_limit[resource]) {
        case 0:
            selected_empire_object_editor->resource_sell_limit[resource] = 15;
            break;
        case 15:
            selected_empire_object_editor->resource_sell_limit[resource] = 25;
            break;
        case 25:
            selected_empire_object_editor->resource_sell_limit[resource] = 40;
            break;
        default:
            selected_empire_object_editor->resource_sell_limit[resource] = 0;
    }
    // if resource to buy already enabled, disable
    selected_empire_object_editor->resource_buy_limit[resource] = 0;
}

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

static void set_resource_buy_limit(int resource, __attribute__((unused)) int param2)
{
    switch (selected_empire_object_editor->resource_buy_limit[resource]) {
        case 0:
            selected_empire_object_editor->resource_buy_limit[resource] = 15;
            break;
        case 15:
            selected_empire_object_editor->resource_buy_limit[resource] = 25;
            break;
        case 25:
            selected_empire_object_editor->resource_buy_limit[resource] = 40;
            break;
        default:
            selected_empire_object_editor->resource_buy_limit[resource] = 0;
    }
    // if resource to sell already enabled, disable
    selected_empire_object_editor->resource_sell_limit[resource] = 0;
}

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

static void set_trade_route_cost_callback(int value)
{
    selected_empire_object_editor->trade_route_cost = value;
}

static struct generic_button_t button_set_trade_route_cost[] = {
    {0, 0, 65, 26, set_trade_route_cost, button_none, 0, 0},
};

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
    window_numeric_input_show(empire_editor_x_min + 500, empire_editor_y_max - 250, 3, 500, set_expansion_year_offset_callback);
}

static struct generic_button_t button_set_expansion_year[] = {
    {0, 0, 38, 26, set_expansion_year_offset, button_none, 0, 0},
};

static void draw_background_editor_empire(void)
{
    int s_width = screen_width();
    int s_height = screen_height();
    empire_editor_x_min = s_width <= MAX_EMPIRE_WIDTH ? 0 : (s_width - MAX_EMPIRE_WIDTH) / 2;
    empire_editor_x_max = s_width <= MAX_EMPIRE_WIDTH ? s_width : empire_editor_x_min + MAX_EMPIRE_WIDTH;
    empire_editor_y_min = s_height <= MAX_EMPIRE_HEIGHT ? 0 : (s_height - MAX_EMPIRE_HEIGHT) / 2;
    empire_editor_y_max = s_height <= MAX_EMPIRE_HEIGHT ? s_height : empire_editor_y_min + MAX_EMPIRE_HEIGHT;

    if (empire_editor_x_min || empire_editor_y_min) {
        graphics_clear_screen();
    }

    int image_base = image_group(GROUP_EDITOR_EMPIRE_PANELS);
    // bottom panel background
    graphics_set_clip_rectangle(empire_editor_x_min, empire_editor_y_min, empire_editor_x_max - empire_editor_x_min, empire_editor_y_max - empire_editor_y_min);
    for (int x = empire_editor_x_min; x < empire_editor_x_max; x += 70) {
        image_draw(image_base + 3, x, empire_editor_y_max - 120);
        image_draw(image_base + 3, x, empire_editor_y_max - 80);
        image_draw(image_base + 3, x, empire_editor_y_max - 40);
    }
    // horizontal bar borders
    for (int x = empire_editor_x_min; x < empire_editor_x_max; x += 86) {
        image_draw(image_base + 1, x, empire_editor_y_min);
        image_draw(image_base + 1, x, empire_editor_y_max - 120);
        image_draw(image_base + 1, x, empire_editor_y_max - 16);
    }
    // vertical bar borders
    for (int y = empire_editor_y_min + 16; y < empire_editor_y_max; y += 86) {
        image_draw(image_base, empire_editor_x_min, y);
        image_draw(image_base, empire_editor_x_max - 16, y);
    }
    // crossbars
    image_draw(image_base + 2, empire_editor_x_min, empire_editor_y_min);
    image_draw(image_base + 2, empire_editor_x_min, empire_editor_y_max - 120);
    image_draw(image_base + 2, empire_editor_x_min, empire_editor_y_max - 16);
    image_draw(image_base + 2, empire_editor_x_max - 16, empire_editor_y_min);
    image_draw(image_base + 2, empire_editor_x_max - 16, empire_editor_y_max - 120);
    image_draw(image_base + 2, empire_editor_x_max - 16, empire_editor_y_max - 16);
    graphics_reset_clip_rectangle();
}

static void draw_shadowed_number(int value, int x, int y, color_t color)
{
    text_draw_number_colored(value, '@', " ", x + 1, y - 1, FONT_SMALL_PLAIN, COLOR_BLACK);
    text_draw_number_colored(value, '@', " ", x, y, FONT_SMALL_PLAIN, color);
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


static void draw_trade_city_info_editor_empire(int x_offset, int y_offset, int width)
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
        if (selected_empire_object_editor->resource_sell_limit[r]) {
            draw_resource_trade_city(r, selected_empire_object_editor->resource_sell_limit[r], resource_x_offset + 1, y_offset - 8);
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
        if (selected_empire_object_editor->resource_buy_limit[r]) {
            draw_resource_trade_city(r, selected_empire_object_editor->resource_buy_limit[r], resource_x_offset + 1, y_offset - 8);
        } else {
            image_draw_blend(871, resource_x_offset + 1, y_offset - 8, COLOR_MOUSE_DARK_GRAY);
        }
        resource_x_offset += 32;
    }

    // draw the trade route cost
    button_set_trade_route_cost->x = resource_x_offset + 255;
    button_set_trade_route_cost->y = y_offset - 8;
    text_draw("Cost to open trade route: ", resource_x_offset + 50, y_offset, FONT_NORMAL_GREEN, 0);
    button_border_draw(button_set_trade_route_cost->x, button_set_trade_route_cost->y, button_set_trade_route_cost->width, 24, empire_editor_focus_trade_route_cost_button_id == 1);
    text_draw_number_centered(selected_empire_object_editor->trade_route_cost, button_set_trade_route_cost->x, y_offset, button_set_trade_route_cost->width, FONT_NORMAL_GREEN);
}

static void draw_foreground_editor_empire(void)
{
    int viewport_width = map_viewport_width();
    int viewport_height = map_viewport_height();
    graphics_set_clip_rectangle(empire_editor_x_min + 16, empire_editor_y_min + 16, viewport_width, viewport_height);
    empire_set_viewport(viewport_width, viewport_height);
    empire_editor_x_draw_offset = empire_editor_x_min + 16;
    empire_editor_y_draw_offset = empire_editor_y_min + 16;
    empire_adjust_scroll(&empire_editor_x_draw_offset, &empire_editor_y_draw_offset);
    image_draw(image_group(GROUP_EDITOR_EMPIRE_MAP), empire_editor_x_draw_offset, empire_editor_y_draw_offset);
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use) {
            // don't draw trade route if trade city switched to non-trade city
            if (empire_objects[i].type == EMPIRE_OBJECT_LAND_TRADE_ROUTE || empire_objects[i].type == EMPIRE_OBJECT_SEA_TRADE_ROUTE) {
                struct empire_object_t *trade_city = get_trade_city_by_trade_route(empire_objects[i].trade_route_id);
                if (!trade_city) {
                    continue;
                }
            }

            if (!empire_editor_show_battle_objects && (
                empire_objects[i].type == EMPIRE_OBJECT_BATTLE_ICON ||
                empire_objects[i].type == EMPIRE_OBJECT_ROMAN_ARMY ||
                empire_objects[i].type == EMPIRE_OBJECT_ENEMY_ARMY)) {
                continue;
            }

            if (empire_objects[i].type == EMPIRE_OBJECT_BATTLE_ICON) {
                draw_shadowed_number(empire_objects[i].invasion_path_id, empire_editor_x_draw_offset + empire_objects[i].x - 9, empire_editor_y_draw_offset + empire_objects[i].y - 9, COLOR_WHITE);
                draw_shadowed_number(empire_objects[i].invasion_years, empire_editor_x_draw_offset + empire_objects[i].x + 15, empire_editor_y_draw_offset + empire_objects[i].y - 9, COLOR_FONT_RED);
            } else if (empire_objects[i].type == EMPIRE_OBJECT_ROMAN_ARMY || empire_objects[i].type == EMPIRE_OBJECT_ENEMY_ARMY) {
                draw_shadowed_number(empire_objects[i].distant_battle_travel_months, empire_editor_x_draw_offset + empire_objects[i].x + 7, empire_editor_y_draw_offset + empire_objects[i].y - 9,
                    empire_objects[i].type == EMPIRE_OBJECT_ROMAN_ARMY ? COLOR_WHITE : COLOR_FONT_RED);
            }
            image_draw(empire_objects[i].image_id, empire_editor_x_draw_offset + empire_objects[i].x, empire_editor_y_draw_offset + empire_objects[i].y);
            const struct image_t *img = image_get(empire_objects[i].image_id);
            if (img->animation_speed_id) {
                image_draw(empire_objects[i].image_id + empire_object_update_animation(&empire_objects[i], empire_objects[i].image_id),
                    empire_editor_x_draw_offset + empire_objects[i].x + img->sprite_offset_x,
                    empire_editor_y_draw_offset + empire_objects[i].y + img->sprite_offset_y);
            }
        }
    }
    graphics_reset_clip_rectangle();

    arrow_buttons_draw(empire_editor_x_min, empire_editor_y_max, arrow_buttons_empire, 2);
    if (selected_empire_object_editor && selected_empire_object_editor->type == EMPIRE_OBJECT_CITY) {
        // draw city info
        int x_offset = empire_editor_x_min + 28;
        int y_offset = empire_editor_y_max - 85;

        int width = lang_text_draw(21, selected_empire_object_editor->city_name_id, x_offset, y_offset, FONT_NORMAL_WHITE);
        arrow_buttons_set_city_type[0].y_offset = y_offset - 8;

        switch (selected_empire_object_editor->city_type) {
            case EMPIRE_CITY_DISTANT_ROMAN:
            case EMPIRE_CITY_VULNERABLE_ROMAN:
                width += lang_text_draw(47, 12, x_offset + 20 + width, y_offset, FONT_NORMAL_GREEN);
                if (selected_empire_object_editor->trade_route_id) {
                    arrow_buttons_set_city_type[0].x_offset = x_offset + 20 + width;
                    arrow_buttons_draw(0, 0, arrow_buttons_set_city_type, 1);
                }
                break;
            case EMPIRE_CITY_FUTURE_TRADE:
                width += text_draw("A future trade city", x_offset + 20 + width, y_offset, FONT_NORMAL_GREEN, 0);
                draw_trade_city_info_editor_empire(x_offset, y_offset, width);
                // draw empire expansion year (offset from scenario start year)
                text_draw("Year offset for empire expansion: ", x_offset + 350, y_offset + 40, FONT_NORMAL_GREEN, 0);
                button_border_draw(x_offset + 620, y_offset + 32, button_set_expansion_year->width, button_set_expansion_year->height, empire_editor_focus_expansion_year_button_id == 1);
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
                    if (selected_empire_object_editor->resource_sell_limit[r]) {
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
                draw_trade_city_info_editor_empire(x_offset, y_offset, width);
                break;
            }
        }
    } else {
        lang_text_draw_centered(150, scenario.empire.id, empire_editor_x_min, empire_editor_y_max - 85, empire_editor_x_max - empire_editor_x_min, FONT_NORMAL_GREEN);
    }
}

static void handle_input_editor_empire(const struct mouse_t *m, const struct hotkeys_t *h)
{
    struct pixel_view_coordinates_t position;
    if (scroll_get_delta(m, &position, SCROLL_TYPE_EMPIRE)) {
        empire_scroll_map(position.x, position.y);
    }
    if (h->toggle_editor_battle_info) {
        empire_editor_show_battle_objects = !empire_editor_show_battle_objects;
    }
    if (h->show_empire_map) {
        show_editor_map();
    }
    if (arrow_buttons_handle_mouse(m, empire_editor_x_min, empire_editor_y_max, arrow_buttons_empire, 2, 0)) {
        return;
    }
    if (m->left.went_up
    && (m->x >= empire_editor_x_min + 16 && m->x <= empire_editor_x_max - 16 && m->y >= empire_editor_y_min + 16 && m->y <= empire_editor_y_max - 120) // mouse inside map
    ) {
        selected_empire_object_editor = empire_select_object(m->x - empire_editor_x_min - 16, m->y - empire_editor_y_min - 16);
        // place border flag on empty map spot
        if (!selected_empire_object_editor
        || !selected_empire_object_editor->in_use
        || (selected_empire_object_editor->type == EMPIRE_OBJECT_ORNAMENT && selected_empire_object_editor->image_id != 3323)) {
            for (int i = 0; i < MAX_OBJECTS; i++) {
                if (!empire_objects[i].in_use) {
                    empire_objects[i].in_use = 1;
                    empire_objects[i].image_id = 3323;
                    empire_objects[i].x = m->x - empire_editor_x_min - 16;
                    empire_objects[i].y = m->y - empire_editor_y_min - 16;
                    return;
                }
            }
        }
        // if border flag selected, remove it
        if (selected_empire_object_editor && selected_empire_object_editor->image_id == 3323) {
            selected_empire_object_editor->in_use = 0;
        }
        window_invalidate();
    }
    if (selected_empire_object_editor && selected_empire_object_editor->type == EMPIRE_OBJECT_CITY) {
        if (selected_empire_object_editor->trade_route_id) {
            arrow_buttons_handle_mouse(m, 0, 0, arrow_buttons_set_city_type, 1, 0);
            if (generic_buttons_handle_mouse(m, 0, 0, button_toggle_sell_resource_limit, sizeof(button_toggle_sell_resource_limit) / sizeof(struct generic_button_t), 0)) {
                return;
            }
            if (generic_buttons_handle_mouse(m, 0, 0, button_toggle_buy_resource_limit, sizeof(button_toggle_buy_resource_limit) / sizeof(struct generic_button_t), 0)) {
                return;
            }
            if (generic_buttons_handle_mouse(m, 0, 0, button_set_trade_route_cost, 1, &empire_editor_focus_trade_route_cost_button_id)) {
                return;
            }
            if (selected_empire_object_editor->city_type == EMPIRE_CITY_FUTURE_TRADE) {
                if (generic_buttons_handle_mouse(m, empire_editor_x_min + 648, empire_editor_y_max - 53, button_set_expansion_year, 1, &empire_editor_focus_expansion_year_button_id)) {
                    return;
                }
            }
        }
        if (m->right.went_up || h->escape_pressed) {
            selected_empire_object_editor = 0;
            window_invalidate();
        }
    } else if (m->right.went_up || h->escape_pressed) {
        show_editor_map();
    }
}

static void show_editor_empire(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EMPIRE,
        draw_background_editor_empire,
        draw_foreground_editor_empire,
        handle_input_editor_empire,
    };
    selected_empire_object_editor = 0;
    empire_editor_focus_trade_route_cost_button_id = 0;
    empire_editor_focus_expansion_year_button_id = 0;
    window_show(&window);
}

static void button_build_tool(int tool, __attribute__((unused)) int param2)
{
    hide_editor_build_menu();
    current_tile.grid_offset = 0;
    editor_tool_set_with_id(tool, 0);
    if (window_is(WINDOW_EDITOR_BUILD_MENU)) {
        show_editor_map();
    } else {
        window_request_refresh();
    }
}

static void show_editor_build_menu(int submenu, __attribute__((unused)) int param2)
{
    if (submenu == MENU_NONE || submenu == selected_submenu_build_menu) {
        hide_editor_build_menu();
        return;
    }
    selected_submenu_build_menu = submenu;
    num_items_build_menu = 0;
    for (int i = 0; i < MAX_ITEMS_PER_MENU; i++) {
        if (editor_menu_types_strings[selected_submenu_build_menu][i][0]) {
            num_items_build_menu++;
        }
    }
    struct window_type_t window = {
        WINDOW_EDITOR_BUILD_MENU,
        0,
        draw_foreground_build_menu,
        handle_input_build_menu,
    };
    window_show(&window);
}

static struct image_button_t buttons_build[] = {
    {7, 123, 71, 23, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 48, button_attributes, button_none, 1, 0, 1, 0, 0, 0},
    {84, 123, 71, 23, IB_NORMAL, GROUP_SIDEBAR_ADVISORS_EMPIRE, 3, show_editor_empire, button_none, 0, 0, 1, 0, 0, 0},
    {13, 267, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 0, button_build_tool, button_none, TOOL_GRASS, 0, 1, 0, 0, 0},
    {63, 267, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 3, show_editor_build_menu, button_none, MENU_SHRUB, 0, 1, 0, 0, 0},
    {113, 267, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 6, button_build_tool, button_none, TOOL_WATER, 0, 1, 0, 0, 0},
    {13, 303, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 21, show_editor_build_menu, button_none, MENU_ELEVATION, 0, 1, 0, 0, 0},
    {63, 303, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 12, button_build_tool, button_none, TOOL_TREES, 0, 1, 0, 0, 0},
    {113, 303, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 15, show_editor_build_menu, button_none, MENU_ROCK, 0, 1, 0, 0, 0},
    {13, 339, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 18, button_build_tool, button_none, TOOL_MEADOW, 0, 1, 0, 0, 0},
    {63, 339, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 30, button_build_tool, button_none, TOOL_ROAD, 0, 1, 0, 0, 0},
    {113, 339, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 24, show_editor_build_menu, button_none, MENU_BRUSH_SIZE, 0, 1, 0, 0, 0},
    {13, 375, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 9, show_editor_build_menu, button_none, MENU_EARTHQUAKE_POINTS, 0, 1, 0, 0, 0},
    {63, 375, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 39, show_editor_build_menu, button_none, MENU_INVASION_POINTS, 0, 1, 0, 0, 0},
    {113, 375, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 42, show_editor_build_menu, button_none, MENU_PEOPLE_POINTS, 0, 1, 0, 0, 0},
    {13, 411, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 33, show_editor_build_menu, button_none, MENU_RIVER_POINTS, 0, 1, 0, 0, 0},
    {63, 411, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 27, show_editor_build_menu, button_none, MENU_NATIVE_BUILDINGS, 0, 1, 0, 0, 0},
    {113, 411, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 51, show_editor_build_menu, button_none, MENU_ANIMAL_POINTS, 0, 1, 0, 0, 0},
};

static void draw_background_editor_map(void)
{
    graphics_clear_screen();
    int image_base = image_group(GROUP_EDITOR_SIDE_PANEL);
    int x_offset = sidebar_common_get_x_offset_expanded();
    image_draw(image_base, x_offset, TOP_MENU_HEIGHT);
    image_buttons_draw(sidebar_common_get_x_offset_expanded(), TOP_MENU_HEIGHT, buttons_build, sizeof(buttons_build) / sizeof(struct image_button_t));
    widget_minimap_draw(x_offset + 8, MINIMAP_Y_OFFSET, MINIMAP_WIDTH, MINIMAP_HEIGHT, 1);

    // draw status
    inner_panel_draw(x_offset + 1, 175, 10, 7);
    int text_offset = x_offset + 6;
    text_draw(editor_sidebar_menu_label_strings[tool_data.type], text_offset, 178, FONT_NORMAL_WHITE, 0);
    switch (tool_data.type) {
        case TOOL_GRASS:
        case TOOL_SMALL_SHRUB:
        case TOOL_MEDIUM_SHRUB:
        case TOOL_LARGE_SHRUB:
        case TOOL_LARGEST_SHRUB:
        case TOOL_WATER:
        case TOOL_TREES:
        case TOOL_SMALL_ROCK:
        case TOOL_MEDIUM_ROCK:
        case TOOL_LARGE_ROCK:
        case TOOL_MEADOW:
        case TOOL_RAISE_LAND:
        case TOOL_LOWER_LAND:
            lang_text_draw(48, tool_data.brush_size, text_offset, 194, FONT_NORMAL_GREEN);
            break;
        default:
            break;
    }
    int people_text;
    int people_font = FONT_NORMAL_RED;
    if (scenario.entry_point.x == -1) {
        if (scenario.exit_point.x == -1) {
            people_text = 60;
        } else {
            people_text = 59;
        }
    } else if (scenario.exit_point.x == -1) {
        people_text = 61;
    } else {
        people_text = 62;
        people_font = FONT_NORMAL_GREEN;
    }
    lang_text_draw(44, people_text, text_offset, 224, people_font);
    if (scenario.entry_point.x != -1 || scenario.exit_point.x != -1) {
        if (scenario.entry_point.x == -1) {
            lang_text_draw(44, 137, text_offset, 239, FONT_NORMAL_RED);
        } else if (scenario.exit_point.x == -1) {
            lang_text_draw(44, 138, text_offset, 239, FONT_NORMAL_RED);
        } else {
            lang_text_draw(44, 67, text_offset, 239, FONT_NORMAL_GREEN);
        }
    }
    int invasion_points = 0;
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        if (scenario.invasion_points[i].x != -1) {
            invasion_points++;
        }
    }
    if (invasion_points == 1) {
        lang_text_draw(44, 64, text_offset, 254, FONT_NORMAL_GREEN);
    } else if (invasion_points > 1) {
        int width = text_draw_number(invasion_points, '@', " ", text_offset - 2, 254, FONT_NORMAL_GREEN);
        lang_text_draw(44, 65, text_offset + width - 8, 254, FONT_NORMAL_GREEN);
    } else {
        if (scenario.invasions[0].type) {
            lang_text_draw(44, 63, text_offset, 254, FONT_NORMAL_RED);
        }
    }
    sidebar_common_draw_relief(x_offset, SIDEBAR_FILLER_Y_OFFSET, GROUP_EDITOR_SIDE_PANEL, 0);
    // draw top menu
    int block_width = 24;
    image_base = image_group(GROUP_TOP_MENU);
    int s_width = screen_width();
    for (int i = 0; i * block_width < s_width; i++) {
        image_draw(image_base + i % 8, i * block_width, 0);
    }
    menu_bar_draw(top_menu_editor, sizeof(top_menu_editor) / sizeof(struct menu_bar_item_t), s_width);
}

static void draw_foreground_editor_map(void)
{
    image_buttons_draw(sidebar_common_get_x_offset_expanded(), TOP_MENU_HEIGHT, buttons_build, sizeof(buttons_build) / sizeof(struct image_button_t));
    widget_minimap_draw(sidebar_common_get_x_offset_expanded() + 8, MINIMAP_Y_OFFSET, MINIMAP_WIDTH, MINIMAP_HEIGHT, 0);
    draw_editor_map();
}

static void window_editor_map_draw_all(void)
{
    draw_background_editor_map();
    draw_foreground_editor_map();
}

static void show_editor_briefing(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_BRIEFING,
        window_editor_map_draw_all,
        draw_foreground_briefing,
        handle_input_briefing,
    };
    start_briefing_box_input();
    window_show(&window);
}

static void button_briefing(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_briefing();
}

static void change_climate(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.climate++;
    if (scenario.climate > 2) {
        scenario.climate = 0;
    }
    scenario.is_saved = 0;
    image_load_climate(scenario.climate, 1, 0);
    widget_minimap_invalidate();
    window_request_refresh();
}

static void set_player_rank(int rank)
{
    scenario.player_rank = rank;
    scenario.is_saved = 0;
}

static void button_rank(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show(screen_dialog_offset_x() + 462, screen_dialog_offset_y() - 55, 32, 11, set_player_rank);
}

static void set_start_year(int year)
{
    scenario.start_year = year;
    scenario.is_saved = 0;
}

static void button_era_start_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    set_start_year(-scenario.start_year);
}

static void set_year_start_year(int value)
{
    if (scenario.start_year < 0) {
        value = -value;
    }
    set_start_year(value);
}

static void button_year_start_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 80, 4, 9999, set_year_start_year);
}

static struct generic_button_t buttons_start_year[] = {
    {158, 100, 100, 30, button_era_start_year, button_none, 0, 0},
    {278, 100, 120, 30, button_year_start_year, button_none, 0, 0},
};

static void draw_foreground_start_year(void)
{
    graphics_in_dialog();

    outer_panel_draw(128, 44, 19, 7);

    lang_text_draw_centered(44, 13, 128, 60, 304, FONT_LARGE_BLACK);

    int start_year = scenario.start_year;
    button_border_draw(158, 100, 100, 30, focus_button_id_start_year == 1);
    lang_text_draw_centered(20, start_year >= 0 ? 1 : 0, 158, 110, 100, FONT_NORMAL_BLACK);

    button_border_draw(278, 100, 120, 30, focus_button_id_start_year == 2);
    text_draw_number_centered(start_year >= 0 ? start_year : -start_year, 278, 110, 120, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_start_year(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_start_year, 2, &focus_button_id_start_year)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_starting_conditions();
    }
}

static void show_editor_start_year(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_START_YEAR,
        window_editor_map_draw_all,
        draw_foreground_start_year,
        handle_input_start_year,
    };
    window_show(&window);
}

static void button_start_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    show_editor_start_year();
}

static void set_initial_favor(int amount)
{
    scenario.initial_favor = amount;
    scenario.is_saved = 0;
}

static void button_initial_favor(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 300, screen_dialog_offset_y() + 50, 3, 100, set_initial_favor);
}

static void set_initial_funds(int amount)
{
    scenario.initial_funds = amount;
    scenario.is_saved = 0;
}

static void button_initial_funds(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 300, screen_dialog_offset_y() + 90, 5, 99999, set_initial_funds);
}

static void set_rescue_loan(int amount)
{
    scenario.rescue_loan = amount;
    scenario.is_saved = 0;
}

static void button_rescue_loan(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 300, screen_dialog_offset_y() + 130, 5, 99999, set_rescue_loan);
}

static void set_initial_personal_savings(int amount)
{
    scenario.initial_personal_savings = amount;
    scenario.is_saved = 0;
}

static void button_initial_personal_savings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 300, screen_dialog_offset_y() + 170, 5, 99999, set_initial_personal_savings);
}

static void button_wheat(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.rome_supplies_wheat = !scenario.rome_supplies_wheat;
    scenario.is_saved = 0;
}

static void button_flotsam(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.flotsam_enabled = !scenario.flotsam_enabled;
    scenario.is_saved = 0;
}

static struct generic_button_t buttons_starting_conditions[] = {
    {262, 48, 200, 30, button_rank, button_none, 0, 0},
    {262, 88, 200, 30, button_start_year, button_none, 0, 0},
    {262, 128, 200, 30, button_initial_favor, button_none, 0, 0},
    {262, 168, 200, 30, button_initial_funds,button_none, 0, 0},
    {262, 208, 200, 30, button_rescue_loan,button_none, 0, 0},
    {262, 248, 200, 30, button_initial_personal_savings, button_none, 0, 0},
    {262, 288, 200, 30, button_wheat,button_none, 0, 0},
    {262, 328, 200, 30, button_flotsam, button_none, 0, 0},
};

static void draw_foreground_starting_conditions(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 30, 24);

    lang_text_draw_centered(44, 88, 0, 16, 480, FONT_LARGE_BLACK);

    // Initial rank
    lang_text_draw(44, 108, 32, 57, FONT_NORMAL_BLACK);
    button_border_draw(262, 48, 200, 30, focus_button_id_starting_conditions == 1);
    lang_text_draw_centered(32, scenario.player_rank, 262, 57, 200, FONT_NORMAL_BLACK);

    // Adjust the start date
    lang_text_draw(44, 89, 32, 97, FONT_NORMAL_BLACK);
    button_border_draw(262, 88, 200, 30, focus_button_id_starting_conditions == 2);
    lang_text_draw_year(scenario.start_year, 330, 97, FONT_NORMAL_BLACK);

    // Initial favor
    text_draw("Initial favor", 32, 137, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(262, 128, 200, 30, focus_button_id_starting_conditions == 3);
    text_draw_number_centered(scenario.initial_favor, 262, 137, 200, FONT_NORMAL_BLACK);

    // Initial funds
    lang_text_draw(44, 39, 32, 177, FONT_NORMAL_BLACK);
    button_border_draw(262, 168, 200, 30, focus_button_id_starting_conditions == 4);
    text_draw_number_centered(scenario.initial_funds, 262, 177, 200, FONT_NORMAL_BLACK);

    // Rescue loan
    lang_text_draw(44, 68, 32, 217, FONT_NORMAL_BLACK);
    button_border_draw(262, 208, 200, 30, focus_button_id_starting_conditions == 5);
    text_draw_number_centered(scenario.rescue_loan, 262, 217, 200, FONT_NORMAL_BLACK);

    // Initial personal savings
    text_draw("Initial personal savings", 32, 257, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(262, 248, 200, 30, focus_button_id_starting_conditions == 6);
    text_draw_number_centered(scenario.initial_personal_savings, 262, 257, 200, FONT_NORMAL_BLACK);

    // Rome supplies wheat?
    lang_text_draw(44, 43, 32, 297, FONT_NORMAL_BLACK);
    button_border_draw(262, 288, 200, 30, focus_button_id_starting_conditions == 7);
    lang_text_draw_centered(18, scenario.rome_supplies_wheat, 262, 297, 200, FONT_NORMAL_BLACK);

    // Flotsam on?
    lang_text_draw(44, 80, 32, 337, FONT_NORMAL_BLACK);
    button_border_draw(262, 328, 200, 30, focus_button_id_starting_conditions == 8);
    lang_text_draw_centered(18, scenario.flotsam_enabled, 262, 337, 200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_starting_conditions(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_starting_conditions, 11, &focus_button_id_starting_conditions)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_attributes();
    }
}

static void show_editor_starting_conditions(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_STARTING_CONDITIONS,
        window_editor_map_draw_all,
        draw_foreground_starting_conditions,
        handle_input_starting_conditions,
    };
    window_show(&window);
}

static void button_starting_conditions(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_starting_conditions();
}

static void button_population_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.population_win_criteria.enabled = !scenario.population_win_criteria.enabled;
    scenario.is_saved = 0;
}

static void set_population(int goal)
{
    scenario.population_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void button_population_value(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 24, 5, 99999, set_population);
}

static void button_rating_toggle(int rating, __attribute__((unused)) int param2)
{
    switch (rating) {
        case RATING_CULTURE:
            scenario.culture_win_criteria.enabled = !scenario.culture_win_criteria.enabled;
            scenario.is_saved = 0;
            break;
        case RATING_PROSPERITY:
            scenario.prosperity_win_criteria.enabled = !scenario.prosperity_win_criteria.enabled;
            scenario.is_saved = 0;
            break;
        case RATING_PEACE:
            scenario.peace_win_criteria.enabled = !scenario.peace_win_criteria.enabled;
            scenario.is_saved = 0;
            break;
        case RATING_FAVOR:
            scenario.favor_win_criteria.enabled = !scenario.favor_win_criteria.enabled;
            scenario.is_saved = 0;
            break;
    }
}

static void set_culture(int goal)
{
    scenario.culture_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void set_prosperity(int goal)
{
    scenario.prosperity_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void set_peace(int goal)
{
    scenario.peace_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void set_favor(int goal)
{
    scenario.favor_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void button_rating_value(int rating, __attribute__((unused)) int param2)
{
    switch (rating) {
        case RATING_CULTURE:
            window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 104, 3, 100, set_culture);
            break;
        case RATING_PROSPERITY:
            window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 104, 3, 100, set_prosperity);
            break;
        case RATING_PEACE:
            window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 104, 3, 100, set_peace);
            break;
        case RATING_FAVOR:
            window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 104, 3, 100, set_favor);
            break;
        default:
            return;
    }
}

static void button_time_limit_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.time_limit_win_criteria.enabled = !scenario.time_limit_win_criteria.enabled;
    if (scenario.time_limit_win_criteria.enabled) {
        scenario.survival_time_win_criteria.enabled = 0;
    }
    scenario.is_saved = 0;
}

static void set_time_limit_years(int years)
{
    scenario.time_limit_win_criteria.years = years;
    scenario.is_saved = 0;
}

static void button_time_limit_years(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 224, 3, 999, set_time_limit_years);
}

static void button_survival_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.survival_time_win_criteria.enabled = !scenario.survival_time_win_criteria.enabled;
    if (scenario.survival_time_win_criteria.enabled) {
        scenario.time_limit_win_criteria.enabled = 0;
    }
    scenario.is_saved = 0;
}

static void set_survival_time_years(int years)
{
    scenario.survival_time_win_criteria.years = years;
    scenario.is_saved = 0;
}

static void button_survival_years(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 264, 3, 999, set_survival_time_years);
}

static struct generic_button_t buttons_win_criteria[] = {
    {260, 62, 80, 30, button_population_toggle, button_none, 0, 0},
    {360, 62, 150, 30, button_population_value, button_none, 0, 0},
    {260, 102, 80, 30, button_rating_toggle, button_none, RATING_CULTURE, 0},
    {360, 102, 150, 30, button_rating_value, button_none, RATING_CULTURE, 0},
    {260, 142, 80, 30, button_rating_toggle, button_none, RATING_PROSPERITY, 0},
    {360, 142, 150, 30, button_rating_value, button_none, RATING_PROSPERITY, 0},
    {260, 182, 80, 30, button_rating_toggle, button_none, RATING_PEACE, 0},
    {360, 182, 150, 30, button_rating_value, button_none, RATING_PEACE, 0},
    {260, 222, 80, 30, button_rating_toggle, button_none, RATING_FAVOR, 0},
    {360, 222, 150, 30, button_rating_value, button_none, RATING_FAVOR, 0},
    {260, 262, 80, 30, button_time_limit_toggle, button_none, 0, 0},
    {360, 262, 150, 30, button_time_limit_years, button_none, 0, 0},
    {260, 302, 80, 30, button_survival_toggle, button_none, 0, 0},
    {360, 302, 150, 30, button_survival_years, button_none, 0, 0},
};

static void draw_foreground_win_criteria(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 34, 24);

    lang_text_draw_centered(44, 48, 0, 16, 544, FONT_LARGE_BLACK);

    // Winning population
    lang_text_draw(44, 56, 30, 70, FONT_NORMAL_BLACK);
    button_border_draw(260, 62, 80, 30, focus_button_id_win_criteria == 1);
    lang_text_draw_centered(18, scenario.population_win_criteria.enabled, 260, 70, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 62, 150, 30, focus_button_id_win_criteria == 2);
    text_draw_number_centered(scenario.population_win_criteria.goal, 360, 70, 150, FONT_NORMAL_BLACK);

    // Culture needed
    lang_text_draw(44, 50, 30, 110, FONT_NORMAL_BLACK);
    button_border_draw(260, 102, 80, 30, focus_button_id_win_criteria == 3);
    lang_text_draw_centered(18, scenario.culture_win_criteria.enabled, 260, 110, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 102, 150, 30, focus_button_id_win_criteria == 4);
    text_draw_number_centered(scenario.culture_win_criteria.goal, 360, 110, 150, FONT_NORMAL_BLACK);

    // Prosperity needed
    lang_text_draw(44, 51, 30, 150, FONT_NORMAL_BLACK);
    button_border_draw(260, 142, 80, 30, focus_button_id_win_criteria == 5);
    lang_text_draw_centered(18, scenario.prosperity_win_criteria.enabled, 260, 150, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 142, 150, 30, focus_button_id_win_criteria == 6);
    text_draw_number_centered(scenario.prosperity_win_criteria.goal, 360, 150, 150, FONT_NORMAL_BLACK);

    // Peace needed
    lang_text_draw(44, 52, 30, 190, FONT_NORMAL_BLACK);
    button_border_draw(260, 182, 80, 30, focus_button_id_win_criteria == 7);
    lang_text_draw_centered(18, scenario.peace_win_criteria.enabled, 260, 190, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 182, 150, 30, focus_button_id_win_criteria == 8);
    text_draw_number_centered(scenario.peace_win_criteria.goal, 360, 190, 150, FONT_NORMAL_BLACK);

    // Favor needed
    lang_text_draw(44, 53, 30, 230, FONT_NORMAL_BLACK);
    button_border_draw(260, 222, 80, 30, focus_button_id_win_criteria == 9);
    lang_text_draw_centered(18, scenario.favor_win_criteria.enabled, 260, 230, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 222, 150, 30, focus_button_id_win_criteria == 10);
    text_draw_number_centered(scenario.favor_win_criteria.goal, 360, 230, 150, FONT_NORMAL_BLACK);

    // Time limit (losing time)
    lang_text_draw(44, 54, 30, 270, FONT_NORMAL_BLACK);
    button_border_draw(260, 262, 80, 30, focus_button_id_win_criteria == 11);
    lang_text_draw_centered(18, scenario.time_limit_win_criteria.enabled, 260, 270, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 262, 150, 30, focus_button_id_win_criteria == 12);
    int width = text_draw_number(scenario.time_limit_win_criteria.years, '+', 0, 380, 270, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.time_limit_win_criteria.years, width + 404, 270, FONT_NORMAL_BLACK);

    // Survival (winning time)
    lang_text_draw(44, 55, 30, 310, FONT_NORMAL_BLACK);
    button_border_draw(260, 302, 80, 30, focus_button_id_win_criteria == 13);
    lang_text_draw_centered(18, scenario.survival_time_win_criteria.enabled, 260, 310, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 302, 150, 30, focus_button_id_win_criteria == 14);
    width = text_draw_number(scenario.survival_time_win_criteria.years, '+', 0, 380, 310, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.survival_time_win_criteria.years, width + 404, 310, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_win_criteria(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_win_criteria, sizeof(buttons_win_criteria) / sizeof(struct generic_button_t), &focus_button_id_win_criteria)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_attributes();
    }
}

static void show_editor_win_criteria(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_WIN_CRITERIA,
        window_editor_map_draw_all,
        draw_foreground_win_criteria,
        handle_input_win_criteria,
    };
    window_show(&window);
}

static void button_win_criteria(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_win_criteria();
}

static void toggle_allowed_building(int id, __attribute__((unused)) int param2)
{
    // sync with building types index
    int building_index = align_bulding_type_index_to_strings(id);

    scenario.allowed_buildings[building_index] = !scenario.allowed_buildings[building_index];
    scenario.is_saved = 0;
}

static struct generic_button_t buttons_allowed_buildings[] = {
    {-138, 50, 190, 20, toggle_allowed_building, button_none, 1, 0},
    {-138, 70, 190, 20, toggle_allowed_building, button_none, 2, 0},
    {-138, 90, 190, 20, toggle_allowed_building, button_none, 3, 0},
    {-138, 110, 190, 20, toggle_allowed_building, button_none, 4, 0},
    {-138, 130, 190, 20, toggle_allowed_building, button_none, 5, 0},
    {-138, 150, 190, 20, toggle_allowed_building, button_none, 6, 0},
    {-138, 170, 190, 20, toggle_allowed_building, button_none, 7, 0},
    {-138, 190, 190, 20, toggle_allowed_building, button_none, 8, 0},
    {-138, 210, 190, 20, toggle_allowed_building, button_none, 9, 0},
    {-138, 230, 190, 20, toggle_allowed_building, button_none, 10, 0},
    {-138, 250, 190, 20, toggle_allowed_building, button_none, 11, 0},
    {-138, 270, 190, 20, toggle_allowed_building, button_none, 12, 0},
    {-138, 290, 190, 20, toggle_allowed_building, button_none, 13, 0},
    {-138, 310, 190, 20, toggle_allowed_building, button_none, 14, 0},
    {-138, 330, 190, 20, toggle_allowed_building, button_none, 15, 0},
    {-138, 350, 190, 20, toggle_allowed_building, button_none, 16, 0},
    {-138, 370, 190, 20, toggle_allowed_building, button_none, 17, 0},
    {-138, 390, 190, 20, toggle_allowed_building, button_none, 18, 0},
    {-138, 410, 190, 20, toggle_allowed_building, button_none, 19, 0},
    {-138, 430, 190, 20, toggle_allowed_building, button_none, 20, 0},
    {61, 50, 190, 20, toggle_allowed_building, button_none, 21, 0},
    {61, 70, 190, 20, toggle_allowed_building, button_none, 22, 0},
    {61, 90, 190, 20, toggle_allowed_building, button_none, 23, 0},
    {61, 110, 190, 20, toggle_allowed_building, button_none, 24, 0},
    {61, 130, 190, 20, toggle_allowed_building, button_none, 25, 0},
    {61, 150, 190, 20, toggle_allowed_building, button_none, 26, 0},
    {61, 170, 190, 20, toggle_allowed_building, button_none, 27, 0},
    {61, 190, 190, 20, toggle_allowed_building, button_none, 28, 0},
    {61, 210, 190, 20, toggle_allowed_building, button_none, 29, 0},
    {61, 230, 190, 20, toggle_allowed_building, button_none, 30, 0},
    {61, 250, 190, 20, toggle_allowed_building, button_none, 31, 0},
    {61, 270, 190, 20, toggle_allowed_building, button_none, 32, 0},
    {61, 290, 190, 20, toggle_allowed_building, button_none, 33, 0},
    {61, 310, 190, 20, toggle_allowed_building, button_none, 34, 0},
    {61, 330, 190, 20, toggle_allowed_building, button_none, 35, 0},
    {61, 350, 190, 20, toggle_allowed_building, button_none, 36, 0},
    {61, 370, 190, 20, toggle_allowed_building, button_none, 37, 0},
    {61, 390, 190, 20, toggle_allowed_building, button_none, 38, 0},
    {61, 410, 190, 20, toggle_allowed_building, button_none, 39, 0},
    {61, 430, 190, 20, toggle_allowed_building, button_none, 40, 0},
    {260, 50, 190, 20, toggle_allowed_building, button_none, 41, 0},
    {260, 70, 190, 20, toggle_allowed_building, button_none, 42, 0},
    {260, 90, 190, 20, toggle_allowed_building, button_none, 43, 0},
    {260, 110, 190, 20, toggle_allowed_building, button_none, 44, 0},
    {260, 130, 190, 20, toggle_allowed_building, button_none, 45, 0},
    {260, 150, 190, 20, toggle_allowed_building, button_none, 46, 0},
    {260, 170, 190, 20, toggle_allowed_building, button_none, 47, 0},
    {260, 190, 190, 20, toggle_allowed_building, button_none, 48, 0},
    {260, 210, 190, 20, toggle_allowed_building, button_none, 49, 0},
    {260, 230, 190, 20, toggle_allowed_building, button_none, 50, 0},
    {260, 250, 190, 20, toggle_allowed_building, button_none, 51, 0},
    {260, 270, 190, 20, toggle_allowed_building, button_none, 52, 0},
    {260, 290, 190, 20, toggle_allowed_building, button_none, 53, 0},
    {260, 310, 190, 20, toggle_allowed_building, button_none, 54, 0},
    {260, 330, 190, 20, toggle_allowed_building, button_none, 55, 0},
    {260, 350, 190, 20, toggle_allowed_building, button_none, 56, 0},
    {260, 370, 190, 20, toggle_allowed_building, button_none, 57, 0},
    {260, 390, 190, 20, toggle_allowed_building, button_none, 58, 0},
    {260, 410, 190, 20, toggle_allowed_building, button_none, 59, 0},
    {260, 430, 190, 20, toggle_allowed_building, button_none, 60, 0},
    {459, 50, 190, 20, toggle_allowed_building, button_none, 61, 0},
    {459, 70, 190, 20, toggle_allowed_building, button_none, 62, 0},
    {459, 90, 190, 20, toggle_allowed_building, button_none, 63, 0},
    {459, 110, 190, 20, toggle_allowed_building, button_none, 64, 0},
    {459, 130, 190, 20, toggle_allowed_building, button_none, 65, 0},
    {459, 150, 190, 20, toggle_allowed_building, button_none, 66, 0},
    {459, 170, 190, 20, toggle_allowed_building, button_none, 67, 0},
    {459, 190, 190, 20, toggle_allowed_building, button_none, 68, 0},
    {459, 210, 190, 20, toggle_allowed_building, button_none, 69, 0},
    {459, 230, 190, 20, toggle_allowed_building, button_none, 70, 0},
    {459, 250, 190, 20, toggle_allowed_building, button_none, 71, 0},
    {459, 270, 190, 20, toggle_allowed_building, button_none, 72, 0},
    {459, 290, 190, 20, toggle_allowed_building, button_none, 73, 0},
    {459, 310, 190, 20, toggle_allowed_building, button_none, 74, 0},
    {459, 330, 190, 20, toggle_allowed_building, button_none, 75, 0},
    {459, 350, 190, 20, toggle_allowed_building, button_none, 76, 0},
    {459, 370, 190, 20, toggle_allowed_building, button_none, 77, 0},
    {459, 390, 190, 20, toggle_allowed_building, button_none, 78, 0},
};

static void draw_foreground_allowed_buildings(void)
{
    graphics_in_dialog();

    outer_panel_draw(-160, 0, 52, 30);

    lang_text_draw_centered(44, 47, -160, 16, 832, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_ALLOWED_BUILDINGS; i++) {
        int x, y;
        if (i < 20) {
            x = -138;
            y = 50 + 20 * (i);
        } else if (i < 40) {
            x = 61;
            y = 50 + 20 * (i - 20);
        } else if (i < 60) {
            x = 260;
            y = 50 + 20 * (i - 40);
        } else {
            x = 459;
            y = 50 + 20 * (i - 60);
        }
        button_border_draw(x, y, 190, 20, focus_button_id_allowed_buildings == i + 1);
        int building_index = align_bulding_type_index_to_strings(i + 1);
        if (scenario.allowed_buildings[building_index]) {
            text_draw_centered(all_buildings_strings[building_index], x, y + 5, 190, FONT_NORMAL_BLACK, 0);
        } else {
            text_draw_centered(all_buildings_strings[building_index], x, y + 5, 190, FONT_NORMAL_PLAIN, COLOR_FONT_RED);
        }

    }

    graphics_reset_dialog();
}

static void handle_input_allowed_buildings(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_allowed_buildings, MAX_ALLOWED_BUILDINGS, &focus_button_id_allowed_buildings)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        empire_object_our_city_set_resources_sell();
        show_editor_attributes();
    }
}

static void show_editor_allowed_buildings(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_ALLOWED_BUILDINGS,
        window_editor_map_draw_all,
        draw_foreground_allowed_buildings,
        handle_input_allowed_buildings,
    };
    window_show(&window);
}

static void button_allowed_buildings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_allowed_buildings();
}

static void button_gladiator_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.gladiator_revolt.state = !scenario.gladiator_revolt.state;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void set_gladiator_revolt_month(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.gladiator_revolt.month = value - 1;
}

static void button_gladiator_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 237, screen_dialog_offset_y() + 17, 2, 12, set_gladiator_revolt_month);
}

static void set_gladiator_revolt_year(int year)
{
    scenario.gladiator_revolt.year = year;
    scenario.is_saved = 0;
}

static void button_gladiator_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 335, screen_dialog_offset_y() + 20, 3, 999, set_gladiator_revolt_year);
}

static void button_sea_trade_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.random_events.sea_trade_problem = !scenario.random_events.sea_trade_problem;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void button_land_trade_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.random_events.land_trade_problem = !scenario.random_events.land_trade_problem;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void button_raise_wages_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.random_events.raise_wages = !scenario.random_events.raise_wages;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void button_lower_wages_toggle(__attribute__((unused)) int param1, __attribute__((unused))  int param2)
{
    scenario.random_events.lower_wages = !scenario.random_events.lower_wages;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void button_contamination_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.random_events.contaminated_water = !scenario.random_events.contaminated_water;
    scenario.is_saved = 0;
    window_request_refresh();
}

static struct generic_button_t buttons_special_events[] = {
    {200, 74, 75, 24, button_gladiator_toggle, button_none, 0, 0},
    {280, 74, 45, 24, button_gladiator_month,button_none, 0, 0},
    {330, 74, 150, 24, button_gladiator_year, button_none, 0, 0},
    {200, 104, 75, 24, button_sea_trade_toggle,button_none, 0, 0},
    {200, 134, 75, 24, button_land_trade_toggle,button_none, 0, 0},
    {200, 164, 75, 24, button_raise_wages_toggle,button_none, 0, 0},
    {200, 194, 75, 24, button_lower_wages_toggle, button_none, 0, 0},
    {200, 224, 75, 24, button_contamination_toggle, button_none, 0, 0},
};

static void draw_foreground_special_events(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 32, 17);

    lang_text_draw_centered(38, 0, 0, 16, 480, FONT_LARGE_BLACK);

    // table header
    lang_text_draw(38, 11, 220, 60, FONT_SMALL_PLAIN);
    lang_text_draw(38, 12, 330, 60, FONT_SMALL_PLAIN);

    // Gladiator revolt
    lang_text_draw(38, 2, 20, 80, FONT_NORMAL_BLACK);
    button_border_draw(200, 74, 75, 24, focus_button_id_special_events == 1);
    lang_text_draw_centered(18, scenario.gladiator_revolt.state, 200, 80, 75, FONT_NORMAL_BLACK);

    button_border_draw(280, 74, 45, 24, focus_button_id_special_events == 2);
    lang_text_draw(25, scenario.gladiator_revolt.month, 288, 80, FONT_NORMAL_BLACK);

    button_border_draw(330, 74, 150, 24, focus_button_id_special_events == 3);
    int width = text_draw_number(scenario.gladiator_revolt.year, '+', 0, 346, 80, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.gladiator_revolt.year, width + 346, 80, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.gladiator_revolt.year == 0 && scenario.gladiator_revolt.month == 0) {
        text_draw(common_editor_strings[2], 346, 24, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // random events
    // Sea trade problem
    lang_text_draw(38, 4, 20, 110, FONT_NORMAL_BLACK);
    button_border_draw(200, 104, 75, 24, focus_button_id_special_events == 4);
    lang_text_draw_centered(18, scenario.random_events.sea_trade_problem, 200, 110, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 110, FONT_SMALL_PLAIN);

    // Land trade problem
    lang_text_draw(38, 5, 20, 140, FONT_NORMAL_BLACK);
    button_border_draw(200, 134, 75, 24, focus_button_id_special_events == 5);
    lang_text_draw_centered(18, scenario.random_events.land_trade_problem, 200, 140, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 140, FONT_SMALL_PLAIN);

    // Rome raises wages
    lang_text_draw(38, 6, 20, 170, FONT_NORMAL_BLACK);
    button_border_draw(200, 164, 75, 24, focus_button_id_special_events == 6);
    lang_text_draw_centered(18, scenario.random_events.raise_wages, 200, 170, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 170, FONT_SMALL_PLAIN);

    // Rome lowers wages
    lang_text_draw(38, 7, 20, 200, FONT_NORMAL_BLACK);
    button_border_draw(200, 194, 75, 24, focus_button_id_special_events == 7);
    lang_text_draw_centered(18, scenario.random_events.lower_wages, 200, 200, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 200, FONT_SMALL_PLAIN);

    // Contaminated water
    lang_text_draw(38, 8, 20, 230, FONT_NORMAL_BLACK);
    button_border_draw(200, 224, 75, 24, focus_button_id_special_events == 8);
    lang_text_draw_centered(18, scenario.random_events.contaminated_water, 200, 230, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 230, FONT_SMALL_PLAIN);

    graphics_reset_dialog();
}

static void handle_input_special_events(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_special_events, sizeof(buttons_special_events) / sizeof(struct generic_button_t), &focus_button_id_special_events)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_attributes();
    }
}

static void show_editor_special_events(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_SPECIAL_EVENTS,
        window_editor_map_draw_all,
        draw_foreground_special_events,
        handle_input_special_events,
    };
    window_show(&window);
}

static void button_special_events(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_special_events();
}

static void set_year_request(int value)
{
    scenario.requests[id_edit_request].year = value;
}

static void button_year_request(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 65, 3, 999, set_year_request);
}

static void set_month_request(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.requests[id_edit_request].month = value - 1;
}

static void button_month_request(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 95, 2, 12, set_month_request);
}

static void set_amount_request(int value)
{
    // don't allow 0
    scenario.requests[id_edit_request].amount = value ? value : 1;
}

static void button_amount_request(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    int max_digits = 3;
    int max_amount = 999;
    if (scenario.requests[id_edit_request].resource == RESOURCE_DENARII) {
        max_digits = 5;
        max_amount = 99999;
    }
    window_numeric_input_show(
        screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 125,
        max_digits, max_amount, set_amount_request
    );
}

static void set_resource_request(int value)
{
    scenario.requests[id_edit_request].resource = value;
    if (scenario.requests[id_edit_request].amount > 999 && scenario.requests[id_edit_request].resource != RESOURCE_DENARII) {
        scenario.requests[id_edit_request].amount = 999;
    }
}

static void button_resource_request(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show_text(screen_dialog_offset_x() + 255, screen_dialog_offset_y() + 77, resource_strings, RESOURCE_TYPES_MAX + 1, set_resource_request);
}

static void set_deadline_years_request(int value)
{
    // don't allow 0
    scenario.requests[id_edit_request].years_deadline = value ? value : 1;
    scenario.requests[id_edit_request].months_to_comply = 12 * scenario.requests[id_edit_request].years_deadline;
}

static void button_deadline_years_request(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 185, 3, 999, set_deadline_years_request);
}

static void set_favor_request(int value)
{
    scenario.requests[id_edit_request].favor = value;
}

static void button_favor_request(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 215, 3, 100, set_favor_request);
}

static void sort_editor_requests(void)
{
    for (int i = 0; i < MAX_REQUESTS; i++) {
        for (int j = MAX_REQUESTS - 1; j > 0; j--) {
            if (scenario.requests[j].resource) {
                // if no previous request scheduled, move current back until first; if previous request is later than current, swap
                if (!scenario.requests[j - 1].resource || scenario.requests[j - 1].year > scenario.requests[j].year
                || (scenario.requests[j - 1].year == scenario.requests[j].year && scenario.requests[j - 1].month > scenario.requests[j].month)) {
                    struct request_t tmp = scenario.requests[j];
                    scenario.requests[j] = scenario.requests[j - 1];
                    scenario.requests[j - 1] = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

static void button_delete_edit_request(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.requests[id_edit_request].year = 1;
    scenario.requests[id_edit_request].month = 0;
    scenario.requests[id_edit_request].amount = 1;
    scenario.requests[id_edit_request].resource = 0;
    scenario.requests[id_edit_request].years_deadline = 5;
    scenario.requests[id_edit_request].months_to_comply = 60;
    scenario.requests[id_edit_request].favor = 8;
    sort_editor_requests();
    show_editor_requests();
}

static struct generic_button_t buttons_edit_request[] = {
    {155, 152, 100, 25, button_year_request, button_none, 0, 0},
    {155, 182, 100, 25, button_month_request, button_none, 0, 0},
    {155, 212, 100, 25, button_amount_request, button_none, 0, 0},
    {155, 242, 100, 25, button_resource_request, button_none, 0, 0},
    {155, 272, 100, 25, button_deadline_years_request, button_none, 0, 0},
    {155, 302, 100, 25, button_favor_request, button_none, 0, 0},
    {105, 342, 200, 25, button_delete_edit_request, button_none, 0, 0},
};

static void draw_foreground_edit_request(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 26, 18);
    // Request from the Emperor
    text_draw_centered("Request from the Emperor", 0, 116, 416, FONT_LARGE_BLACK, COLOR_BLACK);

    // Year offset
    text_draw(common_editor_strings[0], 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 152, 100, 25, focus_button_id_edit_request == 1);
    text_draw_number_centered_prefix(scenario.requests[id_edit_request].year, '+', 157, 158, 100, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.requests[id_edit_request].year, 275, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(common_editor_strings[1], 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 182, 100, 25, focus_button_id_edit_request == 2);
    text_draw_number_centered(scenario.requests[id_edit_request].month + 1, 155, 188, 100, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.requests[id_edit_request].year == 0 && scenario.requests[id_edit_request].month == 0) {
        text_draw(common_editor_strings[2], 260, 188, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // Amount
    text_draw(common_editor_strings[3], 30, 218, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 212, 100, 25, focus_button_id_edit_request == 3);
    text_draw_number_centered(scenario.requests[id_edit_request].amount, 155, 218, 100, FONT_NORMAL_BLACK);

    // Resource
    text_draw(common_editor_strings[4], 30, 248, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 242, 100, 25, focus_button_id_edit_request == 4);
    text_draw_centered(resource_strings[scenario.requests[id_edit_request].resource], 155, 248, 100, FONT_NORMAL_BLACK, COLOR_BLACK);

    // Years deadline
    text_draw("Years deadline:", 30, 278, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 272, 100, 25, focus_button_id_edit_request == 5);
    lang_text_draw_amount(8, 8, scenario.requests[id_edit_request].years_deadline, 160, 278, FONT_NORMAL_BLACK);

    // Favor granted
    text_draw("Favor granted:", 30, 308, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 302, 100, 25, focus_button_id_edit_request == 6);
    text_draw_number_centered_prefix(scenario.requests[id_edit_request].favor, '+', 157, 308, 100, FONT_NORMAL_BLACK);

    // Unschedule request
    button_border_draw(105, 342, 200, 25, focus_button_id_edit_request == 7);
    lang_text_draw_centered(44, 25, 105, 348, 200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_edit_request(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_request, sizeof(buttons_edit_request) / sizeof(struct generic_button_t), &focus_button_id_edit_request)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        sort_editor_requests();
        show_editor_requests();
    }
}

static void show_editor_edit_request(int id, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EDIT_REQUEST,
        window_editor_map_draw_all,
        draw_foreground_edit_request,
        handle_input_edit_request,
    };
    id_edit_request = id;
    window_show(&window);
}

static struct generic_button_t buttons_requests[] = {
    {-300, 48, 290, 25, show_editor_edit_request, button_none, 0, 0},
    {-300, 78, 290, 25, show_editor_edit_request, button_none, 1, 0},
    {-300, 108, 290, 25, show_editor_edit_request, button_none, 2, 0},
    {-300, 138, 290, 25, show_editor_edit_request, button_none, 3, 0},
    {-300, 168, 290, 25, show_editor_edit_request, button_none, 4, 0},
    {-300, 198, 290, 25, show_editor_edit_request, button_none, 5, 0},
    {-300, 228, 290, 25, show_editor_edit_request, button_none, 6, 0},
    {-300, 258, 290, 25, show_editor_edit_request, button_none, 7, 0},
    {-300, 288, 290, 25, show_editor_edit_request, button_none, 8, 0},
    {-300, 318, 290, 25, show_editor_edit_request, button_none, 9, 0},
    {0, 48, 290, 25, show_editor_edit_request, button_none, 10, 0},
    {0, 78, 290, 25, show_editor_edit_request, button_none, 11, 0},
    {0, 108, 290, 25, show_editor_edit_request, button_none, 12, 0},
    {0, 138, 290, 25, show_editor_edit_request, button_none, 13, 0},
    {0, 168, 290, 25, show_editor_edit_request, button_none, 14, 0},
    {0, 198, 290, 25, show_editor_edit_request, button_none, 15, 0},
    {0, 228, 290, 25, show_editor_edit_request, button_none, 16, 0},
    {0, 258, 290, 25, show_editor_edit_request, button_none, 17, 0},
    {0, 288, 290, 25, show_editor_edit_request, button_none, 18, 0},
    {0, 318, 290, 25, show_editor_edit_request, button_none, 19, 0},
    {300, 48, 290, 25, show_editor_edit_request, button_none, 20, 0},
    {300, 78, 290, 25, show_editor_edit_request, button_none, 21, 0},
    {300, 108, 290, 25, show_editor_edit_request, button_none, 22, 0},
    {300, 138, 290, 25, show_editor_edit_request, button_none, 23, 0},
    {300, 168, 290, 25, show_editor_edit_request, button_none, 24, 0},
    {300, 198, 290, 25, show_editor_edit_request, button_none, 25, 0},
    {300, 228, 290, 25, show_editor_edit_request, button_none, 26, 0},
    {300, 258, 290, 25, show_editor_edit_request, button_none, 27, 0},
    {300, 288, 290, 25, show_editor_edit_request, button_none, 28, 0},
    {300, 318, 290, 25, show_editor_edit_request, button_none, 29, 0},
    {600, 48, 290, 25, show_editor_edit_request, button_none, 30, 0},
    {600, 78, 290, 25, show_editor_edit_request, button_none, 31, 0},
    {600, 108, 290, 25, show_editor_edit_request, button_none, 32, 0},
    {600, 138, 290, 25, show_editor_edit_request, button_none, 33, 0},
    {600, 168, 290, 25, show_editor_edit_request, button_none, 34, 0},
    {600, 198, 290, 25, show_editor_edit_request, button_none, 35, 0},
    {600, 228, 290, 25, show_editor_edit_request, button_none, 36, 0},
    {600, 258, 290, 25, show_editor_edit_request, button_none, 37, 0},
    {600, 288, 290, 25, show_editor_edit_request, button_none, 38, 0},
    {600, 318, 290, 25, show_editor_edit_request, button_none, 39, 0},
};

static void draw_foreground_requests(void)
{
    graphics_in_dialog();

    outer_panel_draw(-320, 0, 77, 26);
    lang_text_draw_centered(44, 14, -320, 16, 1232, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_REQUESTS; i++) {
        int x, y;
        if (i < 10) {
            x = -300;
            y = 48 + 30 * i;
        } else if (i < 20) {
            x = 0;
            y = 48 + 30 * (i - 10);
        } else if (i < 30) {
            x = 300;
            y = 48 + 30 * (i - 20);
        } else {
            x = 600;
            y = 48 + 30 * (i - 30);
        }
        button_border_draw(x, y, 290, 25, focus_button_id_requests == i + 1);
        if (scenario.requests[i].resource) {
            int width = lang_text_draw(25, scenario.requests[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.requests[i].year, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            width += text_draw_number(scenario.requests[i].amount, 0, "", x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            image_draw(resource_images[scenario.requests[i].resource].editor_icon_img_id + resource_image_offset(scenario.requests[i].resource, RESOURCE_IMAGE_ICON), x + 12 + width, y + 3);
            width += text_draw_number(scenario.requests[i].years_deadline, 0, "Y", x + 40 + width, y + 6, FONT_NORMAL_BLACK);
            text_draw_number(scenario.requests[i].favor, 0, "F", x + 40 + width, y + 6, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_centered(44, 23, x, y + 6, 290, FONT_NORMAL_BLACK);
        }
    }

    // requests hint
    lang_text_draw_multiline(152, 1, -288, 360, 1200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_requests(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_requests, sizeof(buttons_requests) / sizeof(struct generic_button_t), &focus_button_id_requests)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_attributes();
    }
}

static void show_editor_requests(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_REQUESTS,
        window_editor_map_draw_all,
        draw_foreground_requests,
        handle_input_requests,
    };
    window_show(&window);
}

static void show_editor_edit_requests(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_requests();
}

static void set_year_custom_message(int value)
{
    scenario.editor_custom_messages[custom_message_id].year = value;
}

static void button_year_custom_message(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 15, screen_dialog_offset_y() - 28, 3, 999, set_year_custom_message);
}

static void set_month_custom_message(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.editor_custom_messages[custom_message_id].month = value - 1;
}

static void button_month_custom_message(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 15, screen_dialog_offset_y() + 2, 2, 12, set_month_custom_message);
}

static void button_urgent_custom_msg(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[custom_message_id].urgent = !scenario.editor_custom_messages[custom_message_id].urgent;
}

static void button_enabled_custom_msg(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[custom_message_id].enabled = !scenario.editor_custom_messages[custom_message_id].enabled;
}

static void scenario_editor_sort_custom_messages(void)
{
    for (int i = 0; i < MAX_EDITOR_CUSTOM_MESSAGES; i++) {
        for (int j = MAX_EDITOR_CUSTOM_MESSAGES - 1; j > 0; j--) {
            if (scenario.editor_custom_messages[j].enabled) {
                // if no previous custom message scheduled, move current back until first; if previous custom message is later than current, swap
                if (!scenario.editor_custom_messages[j - 1].enabled || scenario.editor_custom_messages[j - 1].year > scenario.editor_custom_messages[j].year
                || (scenario.editor_custom_messages[j - 1].year == scenario.editor_custom_messages[j].year && scenario.editor_custom_messages[j - 1].month > scenario.editor_custom_messages[j].month)) {
                    struct editor_custom_messages_t tmp = scenario.editor_custom_messages[j];
                    scenario.editor_custom_messages[j] = scenario.editor_custom_messages[j - 1];
                    scenario.editor_custom_messages[j - 1] = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

static void button_reset_message(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[custom_message_id].year = 1;
    scenario.editor_custom_messages[custom_message_id].month = 0;
    scenario.editor_custom_messages[custom_message_id].urgent = 0;
    scenario.editor_custom_messages[custom_message_id].enabled = 0;
    scenario.editor_custom_messages[custom_message_id].title[0] = '\0';
    scenario.editor_custom_messages[custom_message_id].text[0] = '\0';
    scenario.editor_custom_messages[custom_message_id].video_file[0] = '\0';
    scenario_editor_sort_custom_messages();
    show_editor_custom_messages();
}

static void button_reset_title_custom_msg(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[custom_message_id].title[0] = '\0';
    input_box_stop(&editor_custom_message_input_title);
    string_copy(scenario.editor_custom_messages[custom_message_id].title, editor_custom_message_title, MAX_CUSTOM_MESSAGE_TITLE);
    input_box_start(&editor_custom_message_input_title);
}

static void button_reset_text_custom_msg(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[custom_message_id].text[0] = '\0';
    input_box_stop(&editor_custom_message_input_text);
    string_copy(scenario.editor_custom_messages[custom_message_id].text, editor_custom_message_text, MAX_CUSTOM_MESSAGE_TITLE);
    rich_text_reset(0);
    input_box_start(&editor_custom_message_input_text);
}

static struct generic_button_t buttons_edit_custom_message[] = {
    {36, 58, 75, 25, button_year_custom_message, button_none, 0, 0},
    {36, 88, 75, 25, button_month_custom_message, button_none, 0, 0},
    {36, 118, 75, 25, button_urgent_custom_msg, button_none, 0, 0},
    {36, 194, 75, 25, button_enabled_custom_msg, button_none, 0, 0},
    {135, 194, 125, 25, button_reset_message, button_none, 0, 0},
};

static struct generic_button_t reset_title_button[] = {
    {5, 110, 125, 25, button_reset_title_custom_msg, button_none, 0, 0},
};

static struct generic_button_t reset_text_button[] = {
    {515, 16, 150, 35, button_reset_text_custom_msg, button_none, 0, 0},
};

static void draw_foreground_edit_custom_msg(void)
{
    graphics_in_dialog();

    if (custom_message_category == CUSTOM_MESSAGE_ATTRIBUTES) {
        outer_panel_draw(-100, 0, 24, 15);

        // Attributes
        text_draw_centered(custom_messages_strings[1], -100, 16, 384, FONT_LARGE_BLACK, COLOR_BLACK);

        // Year offset
        text_draw(common_editor_strings[0], -68, 64, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(36, 58, 75, 25, focus_button_id_edit_custom_msg == 1);
        text_draw_number_centered_prefix(scenario.editor_custom_messages[custom_message_id].year, '+', 38, 64, 75, FONT_NORMAL_BLACK);
        lang_text_draw_year(scenario.start_year + scenario.editor_custom_messages[custom_message_id].year, 118, 64, FONT_NORMAL_BLACK);

        // Month
        text_draw(common_editor_strings[1], -68, 94, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(36, 88, 75, 25, focus_button_id_edit_custom_msg == 2);
        text_draw_number_centered(scenario.editor_custom_messages[custom_message_id].month + 1, 36, 94, 75, FONT_NORMAL_BLACK);

        // Invalid year/month combination
        if (scenario.editor_custom_messages[custom_message_id].year == 0 && scenario.editor_custom_messages[custom_message_id].month == 0) {
            text_draw(common_editor_strings[2], 122, 94, FONT_NORMAL_PLAIN, COLOR_RED);
        }

        // Urgent
        text_draw("Urgent?", -68, 124, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(36, 118, 75, 25, focus_button_id_edit_custom_msg == 3);
        lang_text_draw_centered(18, scenario.editor_custom_messages[custom_message_id].urgent, 36, 124, 75, FONT_NORMAL_BLACK);

        // Video file
        text_draw("Video file:", -68, 160, FONT_NORMAL_BLACK, COLOR_BLACK);
        input_box_draw(&editor_custom_message_input_video_file);

        // Video file hint
        text_draw("smk/file", 200, 160, FONT_NORMAL_PLAIN, COLOR_TOOLTIP);

        // Enabled
        text_draw("Enabled?", -68, 200, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(36, 194, 75, 25, focus_button_id_edit_custom_msg == 4);
        lang_text_draw_centered(18, scenario.editor_custom_messages[custom_message_id].enabled, 36, 200, 75, FONT_NORMAL_BLACK);

        // Reset message
        button_border_draw(135, 194, 125, 25, focus_button_id_edit_custom_msg == 5);
        text_draw_centered("Reset message", 135, 200, 125, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    else if (custom_message_category == CUSTOM_MESSAGE_TITLE) {
        outer_panel_draw(-100, 0, 21, 10);
        // Title
        text_draw_centered(custom_messages_strings[2], -100, 16, 336, FONT_LARGE_BLACK, COLOR_BLACK);
        input_box_draw(&editor_custom_message_input_title);

        // Reset title
        button_border_draw(5, 110, 125, 25, focus_button_id_edit_custom_msg == 1);
        text_draw_centered("Reset title", 5, 116, 125, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    else if (custom_message_category == CUSTOM_MESSAGE_TEXT) {
        outer_panel_draw(-100, 0, 50, 32);
        // Text
        text_draw_centered(custom_messages_strings[3], -100, 16, 800, FONT_LARGE_BLACK, COLOR_BLACK);
        input_box_draw(&editor_custom_message_input_text);

        // Formatted typed in text
        rich_text_set_fonts(FONT_NORMAL_BLACK, FONT_NORMAL_RED, 6);
        rich_text_init(editor_custom_message_text, -76, 112, 44, 22, 0);
        graphics_set_clip_rectangle(-76, 112, 750, 350);
        rich_text_draw(editor_custom_message_text, -68, 112, 736, 20, 0);
        rich_text_reset_lines_only();
        rich_text_draw_scrollbar();
        graphics_reset_clip_rectangle();

        // @L, @P hint
        text_draw(common_editor_strings[5], -60, 475, FONT_NORMAL_PLAIN, COLOR_TOOLTIP);

        // Reset text
        button_border_draw(515, 16, 150, 35, focus_button_id_edit_custom_msg == 1);
        text_draw_centered("Reset text", 515, 21, 150, FONT_LARGE_PLAIN, COLOR_RED);
    }

    graphics_reset_dialog();
}

static void handle_input_edit_custom_msg(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        if (custom_message_category == CUSTOM_MESSAGE_ATTRIBUTES) {
            input_box_stop(&editor_custom_message_input_video_file);
            if (!string_equals(scenario.editor_custom_messages[custom_message_id].video_file, editor_custom_message_video_file)) {
                string_copy(editor_custom_message_video_file, scenario.editor_custom_messages[custom_message_id].video_file, MAX_CUSTOM_MESSAGE_VIDEO_TEXT);
            }
        } else if (custom_message_category == CUSTOM_MESSAGE_TITLE) {
            input_box_stop(&editor_custom_message_input_title);
            if (!string_equals(scenario.editor_custom_messages[custom_message_id].title, editor_custom_message_title)) {
                string_copy(editor_custom_message_title, scenario.editor_custom_messages[custom_message_id].title, MAX_CUSTOM_MESSAGE_TITLE);
            }
        } else if (custom_message_category == CUSTOM_MESSAGE_TEXT) {
            input_box_stop(&editor_custom_message_input_text);
            if (!string_equals(scenario.editor_custom_messages[custom_message_id].text, editor_custom_message_text)) {
                string_copy(editor_custom_message_text, scenario.editor_custom_messages[custom_message_id].text, MAX_CUSTOM_MESSAGE_TEXT);
            }
            rich_text_reset(0);
        }
        scenario_editor_sort_custom_messages();
        show_editor_custom_messages();
    }

    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (custom_message_category == CUSTOM_MESSAGE_ATTRIBUTES) {
        if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_edit_custom_message, sizeof(buttons_edit_custom_message) / sizeof(struct generic_button_t), &focus_button_id_edit_custom_msg)) {
            return;
        }
    } else if (custom_message_category == CUSTOM_MESSAGE_TITLE) {
        if (generic_buttons_handle_mouse(m_dialog, 0, 0, reset_title_button, 1, &focus_button_id_edit_custom_msg)) {
            return;
        }
    } else if (custom_message_category == CUSTOM_MESSAGE_TEXT) {
        rich_text_handle_mouse(m_dialog);
        if (generic_buttons_handle_mouse(m_dialog, 0, 0, reset_text_button, 1, &focus_button_id_edit_custom_msg)) {
            return;
        }
    }
}

static void show_editor_edit_custom_message(int id, int category)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EDIT_CUSTOM_MESSAGE,
        window_editor_map_draw_all,
        draw_foreground_edit_custom_msg,
        handle_input_edit_custom_msg,
    };
    custom_message_id = id;
    custom_message_category = category;
    if (custom_message_category == CUSTOM_MESSAGE_ATTRIBUTES) {
        string_copy(scenario.editor_custom_messages[id].video_file, editor_custom_message_video_file, MAX_CUSTOM_MESSAGE_TITLE);
        input_box_start(&editor_custom_message_input_video_file);
    } else if (custom_message_category == CUSTOM_MESSAGE_TITLE) {
        string_copy(scenario.editor_custom_messages[id].title, editor_custom_message_title, MAX_CUSTOM_MESSAGE_TITLE);
        input_box_start(&editor_custom_message_input_title);
    } else if (custom_message_category == CUSTOM_MESSAGE_TEXT) {
        string_copy(scenario.editor_custom_messages[id].text, editor_custom_message_text, MAX_CUSTOM_MESSAGE_TEXT);
        input_box_start(&editor_custom_message_input_text);
    }
    window_show(&window);
}

static struct generic_button_t buttons_attributes[] = {
    {-300, 48, 125, 25, show_editor_edit_custom_message, button_none, 0, 0},
    {-300, 78, 125, 25, show_editor_edit_custom_message, button_none, 1, 0},
    {-300, 108, 125, 25, show_editor_edit_custom_message, button_none, 2, 0},
    {-300, 138, 125, 25, show_editor_edit_custom_message, button_none, 3, 0},
    {-300, 168, 125, 25, show_editor_edit_custom_message, button_none, 4, 0},
    {-300, 198, 125, 25, show_editor_edit_custom_message, button_none, 5, 0},
    {-300, 228, 125, 25, show_editor_edit_custom_message, button_none, 6, 0},
    {-300, 258, 125, 25, show_editor_edit_custom_message, button_none, 7, 0},
    {-300, 288, 125, 25, show_editor_edit_custom_message, button_none, 8, 0},
    {-300, 318, 125, 25, show_editor_edit_custom_message, button_none, 9, 0},
    {-300, 348, 125, 25, show_editor_edit_custom_message, button_none, 10, 0},
    {-300, 378, 125, 25, show_editor_edit_custom_message, button_none, 11, 0},
    {-300, 408, 125, 25, show_editor_edit_custom_message, button_none, 12, 0},
    {-300, 438, 125, 25, show_editor_edit_custom_message, button_none, 13, 0},
    {-300, 468, 125, 25, show_editor_edit_custom_message, button_none, 14, 0},
    {100, 48, 125, 25, show_editor_edit_custom_message, button_none, 15, 0},
    {100, 78, 125, 25, show_editor_edit_custom_message, button_none, 16, 0},
    {100, 108, 125, 25, show_editor_edit_custom_message, button_none, 17, 0},
    {100, 138, 125, 25, show_editor_edit_custom_message, button_none, 18, 0},
    {100, 168, 125, 25, show_editor_edit_custom_message, button_none, 19, 0},
    {100, 198, 125, 25, show_editor_edit_custom_message, button_none, 20, 0},
    {100, 228, 125, 25, show_editor_edit_custom_message, button_none, 21, 0},
    {100, 258, 125, 25, show_editor_edit_custom_message, button_none, 22, 0},
    {100, 288, 125, 25, show_editor_edit_custom_message, button_none, 23, 0},
    {100, 318, 125, 25, show_editor_edit_custom_message, button_none, 24, 0},
    {100, 348, 125, 25, show_editor_edit_custom_message, button_none, 25, 0},
    {100, 378, 125, 25, show_editor_edit_custom_message, button_none, 26, 0},
    {100, 408, 125, 25, show_editor_edit_custom_message, button_none, 27, 0},
    {100, 438, 125, 25, show_editor_edit_custom_message, button_none, 28, 0},
    {100, 468, 125, 25, show_editor_edit_custom_message, button_none, 29, 0},
    {500, 48, 125, 25, show_editor_edit_custom_message, button_none, 30, 0},
    {500, 78, 125, 25, show_editor_edit_custom_message, button_none, 31, 0},
    {500, 108, 125, 25, show_editor_edit_custom_message, button_none, 32, 0},
    {500, 138, 125, 25, show_editor_edit_custom_message, button_none, 33, 0},
    {500, 168, 125, 25, show_editor_edit_custom_message, button_none, 34, 0},
    {500, 198, 125, 25, show_editor_edit_custom_message, button_none, 35, 0},
    {500, 228, 125, 25, show_editor_edit_custom_message, button_none, 36, 0},
    {500, 258, 125, 25, show_editor_edit_custom_message, button_none, 37, 0},
    {500, 288, 125, 25, show_editor_edit_custom_message, button_none, 38, 0},
    {500, 318, 125, 25, show_editor_edit_custom_message, button_none, 39, 0},
    {500, 348, 125, 25, show_editor_edit_custom_message, button_none, 39, 0},
    {500, 378, 125, 25, show_editor_edit_custom_message, button_none, 39, 0},
    {500, 408, 125, 25, show_editor_edit_custom_message, button_none, 39, 0},
    {500, 438, 125, 25, show_editor_edit_custom_message, button_none, 39, 0},
    {500, 468, 125, 25, show_editor_edit_custom_message, button_none, 39, 0},
};

static struct generic_button_t buttons_custom_message_title[] = {
    {-170, 48, 125, 25, show_editor_edit_custom_message, button_none, 0, 1},
    {-170, 78, 125, 25, show_editor_edit_custom_message, button_none, 1, 1},
    {-170, 108, 125, 25, show_editor_edit_custom_message, button_none, 2, 1},
    {-170, 138, 125, 25, show_editor_edit_custom_message, button_none, 3, 1},
    {-170, 168, 125, 25, show_editor_edit_custom_message, button_none, 4, 1},
    {-170, 198, 125, 25, show_editor_edit_custom_message, button_none, 5, 1},
    {-170, 228, 125, 25, show_editor_edit_custom_message, button_none, 6, 1},
    {-170, 258, 125, 25, show_editor_edit_custom_message, button_none, 7, 1},
    {-170, 288, 125, 25, show_editor_edit_custom_message, button_none, 8, 1},
    {-170, 318, 125, 25, show_editor_edit_custom_message, button_none, 9, 1},
    {-170, 348, 125, 25, show_editor_edit_custom_message, button_none, 10, 1},
    {-170, 378, 125, 25, show_editor_edit_custom_message, button_none, 11, 1},
    {-170, 408, 125, 25, show_editor_edit_custom_message, button_none, 12, 1},
    {-170, 438, 125, 25, show_editor_edit_custom_message, button_none, 13, 1},
    {-170, 468, 125, 25, show_editor_edit_custom_message, button_none, 14, 1},
    {230, 48, 125, 25, show_editor_edit_custom_message, button_none, 15, 1},
    {230, 78, 125, 25, show_editor_edit_custom_message, button_none, 16, 1},
    {230, 108, 125, 25, show_editor_edit_custom_message, button_none, 17, 1},
    {230, 138, 125, 25, show_editor_edit_custom_message, button_none, 18, 1},
    {230, 168, 125, 25, show_editor_edit_custom_message, button_none, 19, 1},
    {230, 198, 125, 25, show_editor_edit_custom_message, button_none, 20, 1},
    {230, 228, 125, 25, show_editor_edit_custom_message, button_none, 21, 1},
    {230, 258, 125, 25, show_editor_edit_custom_message, button_none, 22, 1},
    {230, 288, 125, 25, show_editor_edit_custom_message, button_none, 23, 1},
    {230, 318, 125, 25, show_editor_edit_custom_message, button_none, 24, 1},
    {230, 348, 125, 25, show_editor_edit_custom_message, button_none, 25, 1},
    {230, 378, 125, 25, show_editor_edit_custom_message, button_none, 26, 1},
    {230, 408, 125, 25, show_editor_edit_custom_message, button_none, 27, 1},
    {230, 438, 125, 25, show_editor_edit_custom_message, button_none, 28, 1},
    {230, 468, 125, 25, show_editor_edit_custom_message, button_none, 29, 1},
    {630, 48, 125, 25, show_editor_edit_custom_message, button_none, 30, 1},
    {630, 78, 125, 25, show_editor_edit_custom_message, button_none, 31, 1},
    {630, 108, 125, 25, show_editor_edit_custom_message, button_none, 32, 1},
    {630, 138, 125, 25, show_editor_edit_custom_message, button_none, 33, 1},
    {630, 168, 125, 25, show_editor_edit_custom_message, button_none, 34, 1},
    {630, 198, 125, 25, show_editor_edit_custom_message, button_none, 35, 1},
    {630, 228, 125, 25, show_editor_edit_custom_message, button_none, 36, 1},
    {630, 258, 125, 25, show_editor_edit_custom_message, button_none, 37, 1},
    {630, 288, 125, 25, show_editor_edit_custom_message, button_none, 38, 1},
    {630, 318, 125, 25, show_editor_edit_custom_message, button_none, 39, 1},
    {630, 348, 125, 25, show_editor_edit_custom_message, button_none, 39, 1},
    {630, 378, 125, 25, show_editor_edit_custom_message, button_none, 39, 1},
    {630, 408, 125, 25, show_editor_edit_custom_message, button_none, 39, 1},
    {630, 438, 125, 25, show_editor_edit_custom_message, button_none, 39, 1},
    {630, 468, 125, 25, show_editor_edit_custom_message, button_none, 39, 1},
};

static struct generic_button_t buttons_custom_message_text[] = {
    {-40, 48, 125, 25, show_editor_edit_custom_message, button_none, 0, 2},
    {-40, 78, 125, 25, show_editor_edit_custom_message, button_none, 1, 2},
    {-40, 108, 125, 25, show_editor_edit_custom_message, button_none, 2, 2},
    {-40, 138, 125, 25, show_editor_edit_custom_message, button_none, 3, 2},
    {-40, 168, 125, 25, show_editor_edit_custom_message, button_none, 4, 2},
    {-40, 198, 125, 25, show_editor_edit_custom_message, button_none, 5, 2},
    {-40, 228, 125, 25, show_editor_edit_custom_message, button_none, 6, 2},
    {-40, 258, 125, 25, show_editor_edit_custom_message, button_none, 7, 2},
    {-40, 288, 125, 25, show_editor_edit_custom_message, button_none, 8, 2},
    {-40, 318, 125, 25, show_editor_edit_custom_message, button_none, 9, 2},
    {-40, 348, 125, 25, show_editor_edit_custom_message, button_none, 10, 2},
    {-40, 378, 125, 25, show_editor_edit_custom_message, button_none, 11, 2},
    {-40, 408, 125, 25, show_editor_edit_custom_message, button_none, 12, 2},
    {-40, 438, 125, 25, show_editor_edit_custom_message, button_none, 13, 2},
    {-40, 468, 125, 25, show_editor_edit_custom_message, button_none, 14, 2},
    {360, 48, 125, 25, show_editor_edit_custom_message, button_none, 15, 2},
    {360, 78, 125, 25, show_editor_edit_custom_message, button_none, 16, 2},
    {360, 108, 125, 25, show_editor_edit_custom_message, button_none, 17, 2},
    {360, 138, 125, 25, show_editor_edit_custom_message, button_none, 18, 2},
    {360, 168, 125, 25, show_editor_edit_custom_message, button_none, 19, 2},
    {360, 198, 125, 25, show_editor_edit_custom_message, button_none, 20, 2},
    {360, 228, 125, 25, show_editor_edit_custom_message, button_none, 21, 2},
    {360, 258, 125, 25, show_editor_edit_custom_message, button_none, 22, 2},
    {360, 288, 125, 25, show_editor_edit_custom_message, button_none, 23, 2},
    {360, 318, 125, 25, show_editor_edit_custom_message, button_none, 24, 2},
    {360, 348, 125, 25, show_editor_edit_custom_message, button_none, 25, 2},
    {360, 378, 125, 25, show_editor_edit_custom_message, button_none, 26, 2},
    {360, 408, 125, 25, show_editor_edit_custom_message, button_none, 27, 2},
    {360, 438, 125, 25, show_editor_edit_custom_message, button_none, 28, 2},
    {360, 468, 125, 25, show_editor_edit_custom_message, button_none, 29, 2},
    {760, 48, 125, 25, show_editor_edit_custom_message, button_none, 30, 2},
    {760, 78, 125, 25, show_editor_edit_custom_message, button_none, 31, 2},
    {760, 108, 125, 25, show_editor_edit_custom_message, button_none, 32, 2},
    {760, 138, 125, 25, show_editor_edit_custom_message, button_none, 33, 2},
    {760, 168, 125, 25, show_editor_edit_custom_message, button_none, 34, 2},
    {760, 198, 125, 25, show_editor_edit_custom_message, button_none, 35, 2},
    {760, 228, 125, 25, show_editor_edit_custom_message, button_none, 36, 2},
    {760, 258, 125, 25, show_editor_edit_custom_message, button_none, 37, 2},
    {760, 288, 125, 25, show_editor_edit_custom_message, button_none, 38, 2},
    {760, 318, 125, 25, show_editor_edit_custom_message, button_none, 39, 2},
    {760, 348, 125, 25, show_editor_edit_custom_message, button_none, 39, 2},
    {760, 378, 125, 25, show_editor_edit_custom_message, button_none, 39, 2},
    {760, 408, 125, 25, show_editor_edit_custom_message, button_none, 39, 2},
    {760, 438, 125, 25, show_editor_edit_custom_message, button_none, 39, 2},
    {760, 468, 125, 25, show_editor_edit_custom_message, button_none, 39, 2},
};

static void draw_foreground_custom_messages(void)
{
    graphics_in_dialog();

    outer_panel_draw(-320, 0, 77, 32);
    text_draw_centered(custom_messages_strings[0], -320, 16, 1232, FONT_LARGE_BLACK, COLOR_BLACK);

    for (int i = 0; i < MAX_EDITOR_CUSTOM_MESSAGES; i++) {
        int x, y;
        if (i < 15) {
            x = -300;
            y = 48 + 30 * i;
        } else if (i < 30) {
            x = 100;
            y = 48 + 30 * (i - 15);
        } else {
            x = 500;
            y = 48 + 30 * (i - 30);
        }
        button_border_draw(x, y, 125, 25, focus_button_id_attr_custom_messages == i + 1);
        if (scenario.editor_custom_messages[i].enabled) {
            int width = lang_text_draw(25, scenario.editor_custom_messages[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            lang_text_draw_year(scenario.start_year + scenario.editor_custom_messages[i].year, x + width + 6, y + 6, FONT_NORMAL_BLACK);
        } else {
            text_draw_centered(custom_messages_strings[1], x, y + 6, 125, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
        int max_preview_length = 12;
        button_border_draw(x + 130, y, 125, 25, focus_button_id_title_custom_messages == i + 1);
        if (scenario.editor_custom_messages[i].title[0] != '\0') {
            char title_preview[max_preview_length + 3];
            string_copy(scenario.editor_custom_messages[i].title, title_preview, max_preview_length);
            if (string_length(scenario.editor_custom_messages[i].title) > max_preview_length) {
                title_preview[max_preview_length - 1] = '.';
                title_preview[max_preview_length] = '.';
                title_preview[max_preview_length + 1] = '.';
                title_preview[max_preview_length + 2] = '\0';
            }
            text_draw(title_preview, x + 138, y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
        } else {
            text_draw_centered(custom_messages_strings[2], x + 130, y + 6, 125, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
        button_border_draw(x + 260, y, 125, 25, focus_button_id_text_custom_messages == i + 1);
        if (scenario.editor_custom_messages[i].text[0] != '\0') {
            char text_preview[max_preview_length + 3];
            string_copy(scenario.editor_custom_messages[i].text, text_preview, max_preview_length);
            if (string_length(scenario.editor_custom_messages[i].text) > max_preview_length) {
                text_preview[max_preview_length - 1] = '.';
                text_preview[max_preview_length] = '.';
                text_preview[max_preview_length + 1] = '.';
                text_preview[max_preview_length + 2] = '\0';
            }
            text_draw(text_preview, x + 268, y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
        } else {
            text_draw_centered(custom_messages_strings[3], x + 260, y + 6, 125, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
    }

    graphics_reset_dialog();
}

static void handle_input_custom_messages(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_attributes, sizeof(buttons_attributes) / sizeof(struct generic_button_t), &focus_button_id_attr_custom_messages)) {
        return;
    }
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_custom_message_title, sizeof(buttons_custom_message_title) / sizeof(struct generic_button_t), &focus_button_id_title_custom_messages)) {
        return;
    }
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_custom_message_text, sizeof(buttons_custom_message_text) / sizeof(struct generic_button_t), &focus_button_id_text_custom_messages)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_attributes();
    }
}

static void show_editor_custom_messages(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_CUSTOM_MESSAGES,
        window_editor_map_draw_all,
        draw_foreground_custom_messages,
        handle_input_custom_messages,
    };
    window_show(&window);
}

static void button_custom_messages(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_custom_messages();
}

static void button_earthquake_severity(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.earthquakes[id_edit_earthquake].severity++;
    switch (scenario.earthquakes[id_edit_earthquake].severity) {
        case 1:
            scenario.earthquakes[id_edit_earthquake].state = 1;
            scenario.earthquakes[id_edit_earthquake].max_duration = 25 + rand() % 32;
            scenario.earthquakes[id_edit_earthquake].max_delay = 10;
            break;
        case 2:
            scenario.earthquakes[id_edit_earthquake].state = 1;
            scenario.earthquakes[id_edit_earthquake].max_duration = 100 + rand() % 64;
            scenario.earthquakes[id_edit_earthquake].max_delay = 8;
            break;
        case 3:
            scenario.earthquakes[id_edit_earthquake].state = 1;
            scenario.earthquakes[id_edit_earthquake].max_duration = 250 + rand() % 128;
            scenario.earthquakes[id_edit_earthquake].max_delay = 6;
            break;
        default:
            scenario.earthquakes[id_edit_earthquake].state = 0;
            scenario.earthquakes[id_edit_earthquake].severity = 0;
            scenario.earthquakes[id_edit_earthquake].max_duration = 0;
            scenario.earthquakes[id_edit_earthquake].max_delay = 0;
            break;
    }
    scenario.is_saved = 0;
    window_request_refresh();
}

static void set_month_earthquake(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.earthquakes[id_edit_earthquake].month = value - 1;
}

static void button_earthquake_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 95, 2, 12, set_month_earthquake);
}

static void set_year_earthquake(int value)
{
    scenario.earthquakes[id_edit_earthquake].year = value;
}

static void button_earthquake_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 65, 3, 999, set_year_earthquake);
}

static void set_earthquake_point(int value)
{
    scenario.earthquakes[id_edit_earthquake].point = value;
}

static void button_earthquake_point(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show_text(screen_dialog_offset_x() + 330, screen_dialog_offset_y() + 165, earthquake_point_names, MAX_EARTHQUAKE_POINTS, set_earthquake_point);
}

static void scenario_editor_sort_earthquakes(void)
{
    for (int i = 0; i < MAX_EARTHQUAKES; i++) {
        for (int j = MAX_EARTHQUAKES - 1; j > 0; j--) {
            if (scenario.earthquakes[j].state) {
                // if no previous earthquake scheduled, move current back until first; if previous earthquake is later than current, swap
                if (!scenario.earthquakes[j - 1].state || scenario.earthquakes[j - 1].year > scenario.earthquakes[j].year
                || (scenario.earthquakes[j - 1].year == scenario.earthquakes[j].year && scenario.earthquakes[j - 1].month > scenario.earthquakes[j].month)) {
                    struct earthquake_t tmp = scenario.earthquakes[j];
                    scenario.earthquakes[j] = scenario.earthquakes[j - 1];
                    scenario.earthquakes[j - 1] = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

static void button_delete_earthquake(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.earthquakes[id_edit_earthquake].state = 0;
    scenario.earthquakes[id_edit_earthquake].severity = 0;
    scenario.earthquakes[id_edit_earthquake].month = 0;
    scenario.earthquakes[id_edit_earthquake].year = 1;
    scenario.earthquakes[id_edit_earthquake].duration = 0;
    scenario.earthquakes[id_edit_earthquake].max_duration = 0;
    scenario.earthquakes[id_edit_earthquake].delay = 0;
    scenario.earthquakes[id_edit_earthquake].max_delay = 0;
    scenario.earthquakes[id_edit_earthquake].point = 0;
    for (int i = 0; i < MAX_EARTHQUAKE_BRANCHES; i++) {
        scenario.earthquakes[id_edit_earthquake].branch_coordinates[i].x = -1;
        scenario.earthquakes[id_edit_earthquake].branch_coordinates[i].y = -1;
    }
    scenario_editor_sort_earthquakes();
    show_editor_earthquakes();
}

static struct generic_button_t buttons_edit_earthquake[] = {
    {130, 152, 100, 25, button_earthquake_year, button_none, 0, 0},
    {130, 182, 100, 25, button_earthquake_month, button_none, 0, 0},
    {130, 212, 100, 25, button_earthquake_severity, button_none, 0, 0},
    {130, 242, 200, 25, button_earthquake_point, button_none, 0, 0},
    {130, 282, 200, 25, button_delete_earthquake, button_none, 0, 0},
};

static void draw_foreground_edit_earthquake(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 29, 14);
    // Scheduling an earthquake
    text_draw_centered("Scheduling an earthquake", 0, 116, 464, FONT_LARGE_BLACK, COLOR_BLACK);

    // Year offset
    text_draw(common_editor_strings[0], 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 152, 100, 25, focus_button_id_edit_earthquake == 1);
    text_draw_number_centered_prefix(scenario.earthquakes[id_edit_earthquake].year, '+', 132, 158, 100, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.earthquakes[id_edit_earthquake].year, 240, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(common_editor_strings[1], 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 182, 100, 25, focus_button_id_edit_earthquake == 2);
    text_draw_number_centered(scenario.earthquakes[id_edit_earthquake].month + 1, 130, 188, 100, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.earthquakes[id_edit_earthquake].year == 0 && scenario.earthquakes[id_edit_earthquake].month == 0) {
        text_draw(common_editor_strings[2], 245, 188, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // Severity
    lang_text_draw(38, 1, 30, 218, FONT_NORMAL_BLACK);
    button_border_draw(130, 212, 100, 25, focus_button_id_edit_earthquake == 3);
    lang_text_draw_centered(40, scenario.earthquakes[id_edit_earthquake].severity, 130, 218, 100, FONT_NORMAL_BLACK);

    // Point
    text_draw("Point", 30, 248, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 242, 200, 25, focus_button_id_edit_earthquake == 4);
    text_draw_centered(earthquakes_strings[scenario.earthquakes[id_edit_earthquake].point + 2], 130, 248, 200, FONT_NORMAL_BLACK, COLOR_BLACK);

    // Cancel earthquake
    button_border_draw(130, 282, 200, 25, focus_button_id_edit_earthquake == 5);
    text_draw_centered("Cancel earthquake", 130, 288, 200, FONT_NORMAL_BLACK, COLOR_BLACK);

    graphics_reset_dialog();
}

static void handle_input_edit_earthquake(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_earthquake, sizeof(buttons_edit_earthquake) / sizeof(struct generic_button_t), &focus_button_id_edit_earthquake)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        scenario_editor_sort_earthquakes();
        show_editor_earthquakes();
    }
}

static void show_editor_edit_earthquake(int id, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EDIT_EARTHQUAKE,
        window_editor_map_draw_all,
        draw_foreground_edit_earthquake,
        handle_input_edit_earthquake,
    };
    id_edit_earthquake = id;
    for (int i = 0; i <= MAX_EARTHQUAKE_POINTS; i++) {
        earthquake_point_names[i] = earthquakes_strings[i + 2];
    }
    window_show(&window);
}

static struct generic_button_t buttons_earthquake[] = {
    {24, 64, 350, 25, show_editor_edit_earthquake, button_none, 0, 0},
    {24, 94, 350, 25, show_editor_edit_earthquake, button_none, 1, 0},
    {24, 124, 350, 25, show_editor_edit_earthquake, button_none, 2, 0},
    {24, 154, 350, 25, show_editor_edit_earthquake, button_none, 3, 0},
    {24, 184, 350, 25, show_editor_edit_earthquake, button_none, 4, 0},
    {24, 214, 350, 25, show_editor_edit_earthquake, button_none, 5, 0},
    {24, 244, 350, 25, show_editor_edit_earthquake, button_none, 6, 0},
    {24, 274, 350, 25, show_editor_edit_earthquake, button_none, 7, 0},
    {24, 304, 350, 25, show_editor_edit_earthquake, button_none, 8, 0},
    {24, 334, 350, 25, show_editor_edit_earthquake, button_none, 9, 0},
};

static void draw_foreground_earthquakes(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 25, 25);
    text_draw_centered(earthquakes_strings[0], 0, 16, 400, FONT_LARGE_BLACK, COLOR_BLACK);

    for (int i = 0; i < MAX_EARTHQUAKES; i++) {
        int x, y;
        x = 24;
        y = 64 + 30 * i;
        button_border_draw(x, y, 350, 25, focus_button_id_earthquakes == i + 1);

        if (scenario.earthquakes[i].state) {
            int width = lang_text_draw(25, scenario.earthquakes[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.earthquakes[i].year, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw(40, scenario.earthquakes[i].severity, x + 15 + width, y + 6, FONT_NORMAL_BLACK);
            text_draw(earthquakes_strings[scenario.earthquakes[i].point + 2], x + 15 + width, y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
        } else {
            text_draw_centered(earthquakes_strings[1], x, y + 6, 350, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
    }

    graphics_reset_dialog();
}

static void handle_input_earthquakes(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_earthquake, sizeof(buttons_earthquake) / sizeof(struct generic_button_t), &focus_button_id_earthquakes)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_attributes();
    }
}

static void show_editor_earthquakes(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EARTHQUAKES,
        window_editor_map_draw_all,
        draw_foreground_earthquakes,
        handle_input_earthquakes,
    };
    window_show(&window);
}

static void button_earthquakes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_earthquakes();
}

static void set_year_invasion(int value)
{
    scenario.invasions[id_edit_invasion].year_offset = value;
}

static void button_year_invasion(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 65, 3, 999, set_year_invasion);
}

static void set_month_invasion(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.invasions[id_edit_invasion].month = value - 1;
}

static void button_month_invasion(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 95, 2, 12, set_month_invasion);
}

static void set_amount_invasion(int value)
{
    scenario.invasions[id_edit_invasion].amount = value;
}

static void button_amount_invasion(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    // if this is set to 0, you get the incoming battle messages, but the enemies never show up... could be a cool trick for a map
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 125, 3, 200, set_amount_invasion);
}

static void set_type_invasion(int value)
{
    scenario.invasions[id_edit_invasion].type = value;
}

static void button_type_invasion(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show_text(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 197, invasion_type_names, INVASION_TYPE_MAX_COUNT, set_type_invasion);
}

static void set_enemy_type(int value)
{
    scenario.invasions[id_edit_invasion].enemy_type = value;
}

static void button_enemy_type(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (scenario.invasions[id_edit_invasion].type == INVASION_TYPE_ENEMY_ARMY) {
        window_select_list_show_text(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 80, enemy_type_names, ENEMY_TYPE_MAX_COUNT, set_enemy_type);
    }
}

static void set_from_invasion(int value)
{
    scenario.invasions[id_edit_invasion].from = value;
}

static void button_from_invasion(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (scenario.invasions[id_edit_invasion].type != INVASION_TYPE_DISTANT_BATTLE && scenario.invasions[id_edit_invasion].type != INVASION_TYPE_CAESAR) {
        window_select_list_show(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 225, 35, 9, set_from_invasion);
    }
}

static void set_attack_type(int value)
{
    scenario.invasions[id_edit_invasion].target_type = value;
}

static void button_attack_type(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (scenario.invasions[id_edit_invasion].type != INVASION_TYPE_DISTANT_BATTLE) {
        window_select_list_show(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 285, 36, 5, set_attack_type);
    }
}

static void sort_editor_invasions(void)
{
    for (int i = 0; i < MAX_INVASIONS; i++) {
        for (int j = MAX_INVASIONS - 1; j > 0; j--) {
            if (scenario.invasions[j].type) {
                // if no previous invasion scheduled, move current back until first; if previous invasion is later than current, swap
                if (!scenario.invasions[j - 1].type || scenario.invasions[j - 1].year_offset > scenario.invasions[j].year_offset
                || (scenario.invasions[j - 1].year_offset == scenario.invasions[j].year_offset && scenario.invasions[j - 1].month > scenario.invasions[j].month)) {
                    struct invasion_t tmp = scenario.invasions[j];
                    scenario.invasions[j] = scenario.invasions[j - 1];
                    scenario.invasions[j - 1] = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

static void button_delete_invasion(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.invasions[id_edit_invasion].year_offset = 1;
    scenario.invasions[id_edit_invasion].month = 0;
    scenario.invasions[id_edit_invasion].amount = 0;
    scenario.invasions[id_edit_invasion].type = 0;
    scenario.invasions[id_edit_invasion].from = 8;
    scenario.invasions[id_edit_invasion].target_type = 0;
    sort_editor_invasions();
    show_editor_invasions();
}

static struct generic_button_t buttons_edit_invasion[] = {
    {145, 152, 60, 25, button_year_invasion, button_none, 0, 0},
    {145, 182, 60, 25, button_month_invasion, button_none, 0, 0},
    {145, 212, 60, 25, button_amount_invasion, button_none, 0, 0},
    {145, 242, 200, 25, button_type_invasion, button_none, 0, 0},
    {145, 272, 200, 25, button_enemy_type, button_none, 0, 0},
    {145, 302, 200, 25, button_from_invasion, button_none, 0, 0},
    {145, 332, 200, 25, button_attack_type, button_none, 0, 0},
    {90, 372, 200, 25, button_delete_invasion, button_none, 0, 0},
};

static void draw_foreground_edit_invasion(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 24, 20);
    // Scheduling an invasion
    lang_text_draw_centered(44, 22, 0, 116, 384, FONT_LARGE_BLACK);

    // Year offset
    text_draw(common_editor_strings[0], 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 152, 60, 25, focus_button_id_edit_invasion == 1);
    text_draw_number_centered_prefix(scenario.invasions[id_edit_invasion].year_offset, '+', 147, 158, 60, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.invasions[id_edit_invasion].year_offset, 215, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(common_editor_strings[1], 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 182, 60, 25, focus_button_id_edit_invasion == 2);
    text_draw_number_centered(scenario.invasions[id_edit_invasion].month + 1, 145, 188, 60, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.invasions[id_edit_invasion].year_offset == 0 && scenario.invasions[id_edit_invasion].month == 0) {
        text_draw(common_editor_strings[2], 220, 188, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // Amount
    text_draw(common_editor_strings[3], 30, 218, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 212, 60, 25, focus_button_id_edit_invasion == 3);
    text_draw_number_centered(scenario.invasions[id_edit_invasion].amount, 145, 218, 60, FONT_NORMAL_BLACK);

    // Type
    text_draw("Type:", 30, 248, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 242, 200, 25, focus_button_id_edit_invasion == 4);
    text_draw_centered(invasions_enemy_army_type_strings[scenario.invasions[id_edit_invasion].type], 145, 248, 200, FONT_NORMAL_BLACK, COLOR_BLACK);

    if (scenario.invasions[id_edit_invasion].type != INVASION_TYPE_DISTANT_BATTLE) {
        if (scenario.invasions[id_edit_invasion].type == INVASION_TYPE_LOCAL_UPRISING || scenario.invasions[id_edit_invasion].type == INVASION_TYPE_ENEMY_ARMY) {
            if (scenario.invasions[id_edit_invasion].type == INVASION_TYPE_ENEMY_ARMY) {
                // Enemy type
                button_border_draw(145, 272, 200, 25, focus_button_id_edit_invasion == 5);
                text_draw_centered(invasions_enemy_type_strings[scenario.invasions[id_edit_invasion].enemy_type], 145, 278, 200, FONT_NORMAL_BLACK, COLOR_BLACK);
            }
            // From
            text_draw("From:", 30, 308, FONT_NORMAL_BLACK, COLOR_BLACK);
            button_border_draw(145, 302, 200, 25, focus_button_id_edit_invasion == 6);
            lang_text_draw_centered(35, scenario.invasions[id_edit_invasion].from, 145, 308, 200, FONT_NORMAL_BLACK);
        }
        // Attack type
        text_draw("Target type:", 30, 338, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(145, 332, 200, 25, focus_button_id_edit_invasion == 7);
        lang_text_draw_centered(36, scenario.invasions[id_edit_invasion].target_type, 145, 338, 200, FONT_NORMAL_BLACK);
    }

    // Unschedule invasion
    button_border_draw(90, 372, 200, 25, focus_button_id_edit_invasion == 8);
    lang_text_draw_centered(44, 26, 90, 378, 200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_edit_invasion(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_invasion, sizeof(buttons_edit_invasion) / sizeof(struct generic_button_t), &focus_button_id_edit_invasion)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        sort_editor_invasions();
        show_editor_invasions();
    }
}

static void show_editor_edit_invasion(int id, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EDIT_INVASION,
        window_editor_map_draw_all,
        draw_foreground_edit_invasion,
        handle_input_edit_invasion,
    };
    id_edit_invasion = id;
    for (int i = 0; i < INVASION_TYPE_MAX_COUNT; i++) {
        invasion_type_names[i] = invasions_enemy_army_type_strings[i];
    }
    for (int i = 0; i < ENEMY_TYPE_MAX_COUNT; i++) {
        enemy_type_names[i] = invasions_enemy_type_strings[i];
    }
    window_show(&window);
}

static struct generic_button_t buttons_invasions[] = {
    {-300, 48, 290, 25, show_editor_edit_invasion, button_none, 0, 0},
    {-300, 78, 290, 25, show_editor_edit_invasion, button_none, 1, 0},
    {-300, 108, 290, 25, show_editor_edit_invasion, button_none, 2, 0},
    {-300, 138, 290, 25, show_editor_edit_invasion, button_none, 3, 0},
    {-300, 168, 290, 25, show_editor_edit_invasion, button_none, 4, 0},
    {-300, 198, 290, 25, show_editor_edit_invasion, button_none, 5, 0},
    {-300, 228, 290, 25, show_editor_edit_invasion, button_none, 6, 0},
    {-300, 258, 290, 25, show_editor_edit_invasion, button_none, 7, 0},
    {-300, 288, 290, 25, show_editor_edit_invasion, button_none, 8, 0},
    {-300, 318, 290, 25, show_editor_edit_invasion, button_none, 9, 0},
    {0, 48, 290, 25, show_editor_edit_invasion, button_none, 10, 0},
    {0, 78, 290, 25, show_editor_edit_invasion, button_none, 11, 0},
    {0, 108, 290, 25, show_editor_edit_invasion, button_none, 12, 0},
    {0, 138, 290, 25, show_editor_edit_invasion, button_none, 13, 0},
    {0, 168, 290, 25, show_editor_edit_invasion, button_none, 14, 0},
    {0, 198, 290, 25, show_editor_edit_invasion, button_none, 15, 0},
    {0, 228, 290, 25, show_editor_edit_invasion, button_none, 16, 0},
    {0, 258, 290, 25, show_editor_edit_invasion, button_none, 17, 0},
    {0, 288, 290, 25, show_editor_edit_invasion, button_none, 18, 0},
    {0, 318, 290, 25, show_editor_edit_invasion, button_none, 19, 0},
    {300, 48, 290, 25, show_editor_edit_invasion, button_none, 20, 0},
    {300, 78, 290, 25, show_editor_edit_invasion, button_none, 21, 0},
    {300, 108, 290, 25, show_editor_edit_invasion, button_none, 22, 0},
    {300, 138, 290, 25, show_editor_edit_invasion, button_none, 23, 0},
    {300, 168, 290, 25, show_editor_edit_invasion, button_none, 24, 0},
    {300, 198, 290, 25, show_editor_edit_invasion, button_none, 25, 0},
    {300, 228, 290, 25, show_editor_edit_invasion, button_none, 26, 0},
    {300, 258, 290, 25, show_editor_edit_invasion, button_none, 27, 0},
    {300, 288, 290, 25, show_editor_edit_invasion, button_none, 28, 0},
    {300, 318, 290, 25, show_editor_edit_invasion, button_none, 29, 0},
    {600, 48, 290, 25, show_editor_edit_invasion, button_none, 30, 0},
    {600, 78, 290, 25, show_editor_edit_invasion, button_none, 31, 0},
    {600, 108, 290, 25, show_editor_edit_invasion, button_none, 32, 0},
    {600, 138, 290, 25, show_editor_edit_invasion, button_none, 33, 0},
    {600, 168, 290, 25, show_editor_edit_invasion, button_none, 34, 0},
    {600, 198, 290, 25, show_editor_edit_invasion, button_none, 35, 0},
    {600, 228, 290, 25, show_editor_edit_invasion, button_none, 36, 0},
    {600, 258, 290, 25, show_editor_edit_invasion, button_none, 37, 0},
    {600, 288, 290, 25, show_editor_edit_invasion, button_none, 38, 0},
    {600, 318, 290, 25, show_editor_edit_invasion, button_none, 39, 0},
};

static void draw_foreground_invasions(void)
{
    graphics_in_dialog();

    outer_panel_draw(-320, 0, 77, 26);
    lang_text_draw_centered(44, 15, -320, 16, 1232, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_INVASIONS; i++) {
        int x, y;
        if (i < 10) {
            x = -300;
            y = 48 + 30 * i;
        } else if (i < 20) {
            x = 0;
            y = 48 + 30 * (i - 10);
        } else if (i < 30) {
            x = 300;
            y = 48 + 30 * (i - 20);
        } else {
            x = 600;
            y = 48 + 30 * (i - 30);
        }
        button_border_draw(x, y, 290, 25, focus_button_id_invasions == i + 1);
        if (scenario.invasions[i].type) {
            int width = lang_text_draw(25, scenario.invasions[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.invasions[i].year_offset, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            width += text_draw_number(scenario.invasions[i].amount, 0, 0, x + 12 + width, y + 6, FONT_NORMAL_BLACK);
            if (scenario.invasions[i].type == INVASION_TYPE_ENEMY_ARMY) {
                char *enemy_type_text = invasions_enemy_type_strings[scenario.invasions[i].enemy_type];
                text_draw(enemy_type_text, x - 12 + width + (290 - width - text_get_width(enemy_type_text, FONT_NORMAL_BLACK)), y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
            } else {
                char *invasions_type_text = invasions_enemy_army_type_strings[scenario.invasions[i].type];
                text_draw(invasions_type_text, x - 12 + width + (290 - width - text_get_width(invasions_type_text, FONT_NORMAL_BLACK)), y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
            }
        } else {
            lang_text_draw_centered(44, 23, x, y + 6, 290, FONT_NORMAL_BLACK);
        }
    }

    // invasions hint
    lang_text_draw_multiline(152, 2, -280, 360, 1200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_invasions(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_invasions, sizeof(buttons_invasions) / sizeof(struct generic_button_t), &focus_button_id_invasions)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_attributes();
    }
}

static void show_editor_invasions(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_INVASIONS,
        window_editor_map_draw_all,
        draw_foreground_invasions,
        handle_input_invasions,
    };
    window_show(&window);
}

static void show_editor_edit_invasions(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_invasions();
}

static void set_year_price_change(int value)
{
    scenario.price_changes[id_edit_price_change].year = value;
}

static void button_year_price_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 65, 3, 999, set_year_price_change);
}

static void set_month_price_change(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.price_changes[id_edit_price_change].month = value - 1;
}

static void button_month_price_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 95, 2, 12, set_month_price_change);
}

static void set_resource_price_change(int value)
{
    scenario.price_changes[id_edit_price_change].resource = value;
}

static void button_resource_price_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show_text(screen_dialog_offset_x() + 80, screen_dialog_offset_y() + 55, resource_strings, RESOURCE_TYPES_MAX, set_resource_price_change);
}

static void button_toggle_rise_price_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.price_changes[id_edit_price_change].is_rise = !scenario.price_changes[id_edit_price_change].is_rise;
}

static void set_amount_price_change(int value)
{
    // don't allow 0
    scenario.price_changes[id_edit_price_change].amount = value ? value : 1;
}

static void button_amount_price_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 330, screen_dialog_offset_y() + 125, 2, 99, set_amount_price_change);
}

static void sort_editor_price_changes(void)
{
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        for (int j = MAX_PRICE_CHANGES - 1; j > 0; j--) {
            if (scenario.price_changes[j].resource) {
                // if no previous price change scheduled, move current back until first; if previous price change is later than current, swap
                if (!scenario.price_changes[j - 1].resource || scenario.price_changes[j - 1].year > scenario.price_changes[j].year
                || (scenario.price_changes[j - 1].year == scenario.price_changes[j].year && scenario.price_changes[j - 1].month > scenario.price_changes[j].month)) {
                    struct price_change_t tmp = scenario.price_changes[j];
                    scenario.price_changes[j] = scenario.price_changes[j - 1];
                    scenario.price_changes[j - 1] = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

static void button_delete_price_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.price_changes[id_edit_price_change].year = 1;
    scenario.price_changes[id_edit_price_change].month = 0;
    scenario.price_changes[id_edit_price_change].resource = 0;
    scenario.price_changes[id_edit_price_change].is_rise = 0;
    scenario.price_changes[id_edit_price_change].amount = 1;
    sort_editor_price_changes();
    show_editor_price_changes();
}

static struct generic_button_t buttons_edit_price_change[] = {
    {130, 152, 100, 25, button_year_price_change, button_none, 0, 0},
    {130, 182, 100, 25, button_month_price_change, button_none, 0, 0},
    {130, 212, 100, 25, button_resource_price_change, button_none, 0, 0},
    {250, 212, 100, 25, button_toggle_rise_price_change, button_none, 0, 0},
    {355, 212, 100, 25, button_amount_price_change, button_none, 0, 0},
    {138, 252, 200, 25, button_delete_price_change, button_none, 0, 0},
};

static void draw_foreground_edit_price_change(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 29, 12);
    // Price changes
    lang_text_draw_centered(44, 95, 0, 116, 464, FONT_LARGE_BLACK);

    // Year offset
    text_draw(common_editor_strings[0], 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 152, 100, 25, focus_button_id_edit_price_change == 1);
    text_draw_number_centered_prefix(scenario.price_changes[id_edit_price_change].year, '+', 132, 158, 100, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.price_changes[id_edit_price_change].year, 240, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(common_editor_strings[1], 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 182, 100, 25, focus_button_id_edit_price_change == 2);
    text_draw_number_centered(scenario.price_changes[id_edit_price_change].month + 1, 130, 188, 100, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.price_changes[id_edit_price_change].year == 0 && scenario.price_changes[id_edit_price_change].month == 0) {
        text_draw(common_editor_strings[2], 245, 188, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // Resource
    text_draw(common_editor_strings[4], 30, 218, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 212, 100, 25, focus_button_id_edit_price_change == 3);
    text_draw_centered(resource_strings[scenario.price_changes[id_edit_price_change].resource], 130, 218, 100, FONT_NORMAL_BLACK, COLOR_BLACK);

    // Rises/falls by
    button_border_draw(235, 212, 100, 25, focus_button_id_edit_price_change == 4);
    lang_text_draw_centered(44, scenario.price_changes[id_edit_price_change].is_rise ? 104 : 103, 235, 218, 100, FONT_NORMAL_BLACK);

    // Amount
    button_border_draw(340, 212, 100, 25, focus_button_id_edit_price_change == 5);
    text_draw_number_centered(scenario.price_changes[id_edit_price_change].amount, 340, 218, 100, FONT_NORMAL_BLACK);

    // Cancel price change
    button_border_draw(130, 252, 200, 25, focus_button_id_edit_price_change == 6);
    lang_text_draw_centered(44, 105, 130, 258, 200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_edit_price_change(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_price_change, sizeof(buttons_edit_price_change) / sizeof(struct generic_button_t), &focus_button_id_edit_price_change)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        sort_editor_price_changes();
        show_editor_price_changes();
    }
}

static void show_editor_edit_price_change(int id, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EDIT_PRICE_CHANGE,
        window_editor_map_draw_all,
        draw_foreground_edit_price_change,
        handle_input_edit_price_change,
    };
    id_edit_price_change = id;
    window_show(&window);
}

static struct generic_button_t buttons_price_changes[] = {
    {-300, 48, 290, 25, show_editor_edit_price_change, button_none, 0, 0},
    {-300, 78, 290, 25, show_editor_edit_price_change, button_none, 1, 0},
    {-300, 108, 290, 25, show_editor_edit_price_change, button_none, 2, 0},
    {-300, 138, 290, 25, show_editor_edit_price_change, button_none, 3, 0},
    {-300, 168, 290, 25, show_editor_edit_price_change, button_none, 4, 0},
    {-300, 198, 290, 25, show_editor_edit_price_change, button_none, 5, 0},
    {-300, 228, 290, 25, show_editor_edit_price_change, button_none, 6, 0},
    {-300, 258, 290, 25, show_editor_edit_price_change, button_none, 7, 0},
    {-300, 288, 290, 25, show_editor_edit_price_change, button_none, 8, 0},
    {-300, 318, 290, 25, show_editor_edit_price_change, button_none, 9, 0},
    {0, 48, 290, 25, show_editor_edit_price_change, button_none, 10, 0},
    {0, 78, 290, 25, show_editor_edit_price_change, button_none, 11, 0},
    {0, 108, 290, 25, show_editor_edit_price_change, button_none, 12, 0},
    {0, 138, 290, 25, show_editor_edit_price_change, button_none, 13, 0},
    {0, 168, 290, 25, show_editor_edit_price_change, button_none, 14, 0},
    {0, 198, 290, 25, show_editor_edit_price_change, button_none, 15, 0},
    {0, 228, 290, 25, show_editor_edit_price_change, button_none, 16, 0},
    {0, 258, 290, 25, show_editor_edit_price_change, button_none, 17, 0},
    {0, 288, 290, 25, show_editor_edit_price_change, button_none, 18, 0},
    {0, 318, 290, 25, show_editor_edit_price_change, button_none, 19, 0},
    {300, 48, 290, 25, show_editor_edit_price_change, button_none, 20, 0},
    {300, 78, 290, 25, show_editor_edit_price_change, button_none, 21, 0},
    {300, 108, 290, 25, show_editor_edit_price_change, button_none, 22, 0},
    {300, 138, 290, 25, show_editor_edit_price_change, button_none, 23, 0},
    {300, 168, 290, 25, show_editor_edit_price_change, button_none, 24, 0},
    {300, 198, 290, 25, show_editor_edit_price_change, button_none, 25, 0},
    {300, 228, 290, 25, show_editor_edit_price_change, button_none, 26, 0},
    {300, 258, 290, 25, show_editor_edit_price_change, button_none, 27, 0},
    {300, 288, 290, 25, show_editor_edit_price_change, button_none, 28, 0},
    {300, 318, 290, 25, show_editor_edit_price_change, button_none, 29, 0},
    {600, 48, 290, 25, show_editor_edit_price_change, button_none, 30, 0},
    {600, 78, 290, 25, show_editor_edit_price_change, button_none, 31, 0},
    {600, 108, 290, 25, show_editor_edit_price_change, button_none, 32, 0},
    {600, 138, 290, 25, show_editor_edit_price_change, button_none, 33, 0},
    {600, 168, 290, 25, show_editor_edit_price_change, button_none, 34, 0},
    {600, 198, 290, 25, show_editor_edit_price_change, button_none, 35, 0},
    {600, 228, 290, 25, show_editor_edit_price_change, button_none, 36, 0},
    {600, 258, 290, 25, show_editor_edit_price_change, button_none, 37, 0},
    {600, 288, 290, 25, show_editor_edit_price_change, button_none, 38, 0},
    {600, 318, 290, 25, show_editor_edit_price_change, button_none, 39, 0},
};

static void draw_foreground_price_changes(void)
{
    graphics_in_dialog();

    outer_panel_draw(-320, 0, 77, 26);
    lang_text_draw_centered(44, 95, -320, 16, 1232, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        int x, y;
        if (i < 10) {
            x = -300;
            y = 48 + 30 * i;
        } else if (i < 20) {
            x = 0;
            y = 48 + 30 * (i - 10);
        } else if (i < 30) {
            x = 300;
            y = 48 + 30 * (i - 20);
        } else {
            x = 600;
            y = 48 + 30 * (i - 30);
        }
        button_border_draw(x, y, 290, 25, focus_button_id_price_changes == i + 1);

        if (scenario.price_changes[i].resource) {
            int width = lang_text_draw(25, scenario.price_changes[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.price_changes[i].year, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            image_draw(resource_images[scenario.price_changes[i].resource].editor_icon_img_id + resource_image_offset(scenario.price_changes[i].resource, RESOURCE_IMAGE_ICON), x + 12 + width, y + 3);
            width += lang_text_draw(44, scenario.price_changes[i].is_rise ? 104 : 103, x + 45 + width, y + 6, FONT_NORMAL_BLACK);
            text_draw_number(scenario.price_changes[i].amount, 0, 0, x + 45 + width, y + 6, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_centered(44, 102, x, y + 6, 290, FONT_NORMAL_BLACK);
        }
    }

    // price changes hint
    lang_text_draw_multiline(152, 3, -280, 360, 1200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_price_changes(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_price_changes, sizeof(buttons_price_changes) / sizeof(struct generic_button_t), &focus_button_id_price_changes)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_attributes();
    }
}

static void show_editor_price_changes(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_PRICE_CHANGES,
        window_editor_map_draw_all,
        draw_foreground_price_changes,
        handle_input_price_changes,
    };
    window_show(&window);
}

static void button_price_changes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_price_changes();
}

static void set_year_demand_change(int value)
{
    scenario.demand_changes[id_demand_route].year = value;
}

static void button_year_demand_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 65, 3, 999, set_year_demand_change);
}

static void set_month_demand_change(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.demand_changes[id_demand_route].month = value - 1;
}

static void button_month_demand_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 95, 2, 12, set_month_demand_change);
}

static void set_resource_demand_change(int value)
{
    // reset trade_city_id to force re-selection (upon which valid routes for the resource are determined)
    scenario.demand_changes[id_demand_route].trade_city_id = 0;

    scenario.demand_changes[id_demand_route].resource = value;
}

static void button_resource_demand_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show_text(screen_dialog_offset_x() + 230, screen_dialog_offset_y() + 55, resource_strings, RESOURCE_TYPES_MAX, set_resource_demand_change);
}

static void set_route_id(int index)
{
    scenario.demand_changes[id_demand_route].trade_city_id = demand_routes_route_ids[index];
}

static void create_route_names(void)
{
    demand_routes_num_routes = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        struct empire_object_t *object = &empire_objects[i];
        if (object && (object->city_type == EMPIRE_CITY_TRADE || object->city_type == EMPIRE_CITY_FUTURE_TRADE)) {
            if (object->resource_sell_limit[scenario.demand_changes[id_demand_route].resource] || object->resource_buy_limit[scenario.demand_changes[id_demand_route].resource]) {
                char *dst = route_display_names[i];
                int offset = string_from_int(dst, i, 0);
                dst[offset++] = ' ';
                dst[offset++] = '-';
                dst[offset++] = ' ';
                string_copy(lang_get_string(21, object->city_name_id), &dst[offset], DEMAND_ROUTE_MAX_NAME_LENGTH - offset);
                demand_routes_route_ids[demand_routes_num_routes] = i;
                demand_routes_route_names[demand_routes_num_routes] = route_display_names[i];
                demand_routes_num_routes++;
            }
        }
    }
}

static void button_route(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    create_route_names();
    // if no routes available, reset to default text in pop-up window
    if (!demand_routes_num_routes) {
        demand_routes_route_ids[0] = 0;
        demand_routes_route_names[0] = route_display_names[0];
        demand_routes_num_routes = 1;
    }
    window_select_list_show_text(screen_dialog_offset_x() + 330, screen_dialog_offset_y() + 205, demand_routes_route_names, demand_routes_num_routes, set_route_id);
}

static void button_toggle_rise_demand_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.demand_changes[id_demand_route].is_rise = !scenario.demand_changes[id_demand_route].is_rise;
}

static void sort_editor_demand_changes(void)
{
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        for (int j = MAX_DEMAND_CHANGES - 1; j > 0; j--) {
            if (scenario.demand_changes[j].resource && scenario.demand_changes[j].trade_city_id) {
                // if no previous demand change scheduled, move current back until first; if previous demand change is later than current, swap
                if (!scenario.demand_changes[j - 1].resource || !scenario.demand_changes[j - 1].trade_city_id || scenario.demand_changes[j - 1].year > scenario.demand_changes[j].year
                || (scenario.demand_changes[j - 1].year == scenario.demand_changes[j].year && scenario.demand_changes[j - 1].month > scenario.demand_changes[j].month)) {
                    struct demand_change_t tmp = scenario.demand_changes[j];
                    scenario.demand_changes[j] = scenario.demand_changes[j - 1];
                    scenario.demand_changes[j - 1] = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

static void button_delete_demand_change(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.demand_changes[id_demand_route].year = 1;
    scenario.demand_changes[id_demand_route].month = 0;
    scenario.demand_changes[id_demand_route].resource = 0;
    scenario.demand_changes[id_demand_route].trade_city_id = 0;
    scenario.demand_changes[id_demand_route].is_rise = 0;
    sort_editor_demand_changes();
    show_editor_demand_changes();
}

static struct generic_button_t buttons_edit_demand_change[] = {
    {130, 152, 100, 25, button_year_demand_change, button_none, 0, 0},
    {130, 182, 100, 25, button_month_demand_change, button_none, 0, 0},
    {130, 212, 100, 25, button_resource_demand_change, button_none, 0, 0},
    {130, 242, 200, 25, button_route, button_none, 0, 0},
    {230, 272, 100, 25, button_toggle_rise_demand_change, button_none, 0, 0},
    {80, 312, 250, 25, button_delete_demand_change, button_none, 0, 0},
};

static void draw_foreground_edit_demand_change(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 26, 16);
    // Demand changes
    lang_text_draw_centered(44, 94, 0, 116, 416, FONT_LARGE_BLACK);

    // Year offset
    text_draw(common_editor_strings[0], 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 152, 100, 25, focus_button_id_demand_route == 1);
    text_draw_number_centered_prefix(scenario.demand_changes[id_demand_route].year, '+', 132, 158, 100, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.demand_changes[id_demand_route].year, 240, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(common_editor_strings[1], 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 182, 100, 25, focus_button_id_demand_route == 2);
    text_draw_number_centered(scenario.demand_changes[id_demand_route].month + 1, 130, 188, 100, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.demand_changes[id_demand_route].year == 0 && scenario.demand_changes[id_demand_route].month == 0) {
        text_draw(common_editor_strings[2], 245, 188, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // Resource
    text_draw(common_editor_strings[4], 30, 218, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 212, 100, 25, focus_button_id_demand_route == 3);
    text_draw_centered(resource_strings[scenario.demand_changes[id_demand_route].resource], 130, 218, 100, FONT_NORMAL_BLACK, COLOR_BLACK);

    // in route
    lang_text_draw(44, 97, 30, 248, FONT_NORMAL_BLACK);
    button_border_draw(130, 242, 200, 25, focus_button_id_demand_route == 4);
    if (scenario.demand_changes[id_demand_route].trade_city_id) {
        text_draw_centered(route_display_names[scenario.demand_changes[id_demand_route].trade_city_id], 130, 248, 200, FONT_NORMAL_BLACK, 0);
    }

    // demand for this good rises/falls
    lang_text_draw(44, 100, 30, 278, FONT_NORMAL_BLACK);
    button_border_draw(230, 272, 100, 25, focus_button_id_demand_route == 5);
    lang_text_draw_centered(44, scenario.demand_changes[id_demand_route].is_rise ? 99 : 98, 230, 278, 100, FONT_NORMAL_BLACK);

    // Cancel fluctuation
    button_border_draw(80, 312, 250, 25, focus_button_id_demand_route == 6);
    lang_text_draw_centered(44, 101, 80, 318, 250, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input_edit_demand_change(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_demand_change, sizeof(buttons_edit_demand_change) / sizeof(struct generic_button_t), &focus_button_id_demand_route)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        sort_editor_demand_changes();
        show_editor_demand_changes();
    }
}

static void show_editor_edit_demand_change(int id, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EDIT_DEMAND_CHANGE,
        window_editor_map_draw_all,
        draw_foreground_edit_demand_change,
        handle_input_edit_demand_change,
    };
    id_demand_route = id;
    create_route_names();
    window_show(&window);
}

static struct generic_button_t buttons_demand_changes[] = {
    {-300, 48, 290, 25, show_editor_edit_demand_change, button_none, 0, 0},
    {-300, 78, 290, 25, show_editor_edit_demand_change, button_none, 1, 0},
    {-300, 108, 290, 25, show_editor_edit_demand_change, button_none, 2, 0},
    {-300, 138, 290, 25, show_editor_edit_demand_change, button_none, 3, 0},
    {-300, 168, 290, 25, show_editor_edit_demand_change, button_none, 4, 0},
    {-300, 198, 290, 25, show_editor_edit_demand_change, button_none, 5, 0},
    {-300, 228, 290, 25, show_editor_edit_demand_change, button_none, 6, 0},
    {-300, 258, 290, 25, show_editor_edit_demand_change, button_none, 7, 0},
    {-300, 288, 290, 25, show_editor_edit_demand_change, button_none, 8, 0},
    {-300, 318, 290, 25, show_editor_edit_demand_change, button_none, 9, 0},
    {0, 48, 290, 25, show_editor_edit_demand_change, button_none, 10, 0},
    {0, 78, 290, 25, show_editor_edit_demand_change, button_none, 11, 0},
    {0, 108, 290, 25, show_editor_edit_demand_change, button_none, 12, 0},
    {0, 138, 290, 25, show_editor_edit_demand_change, button_none, 13, 0},
    {0, 168, 290, 25, show_editor_edit_demand_change, button_none, 14, 0},
    {0, 198, 290, 25, show_editor_edit_demand_change, button_none, 15, 0},
    {0, 228, 290, 25, show_editor_edit_demand_change, button_none, 16, 0},
    {0, 258, 290, 25, show_editor_edit_demand_change, button_none, 17, 0},
    {0, 288, 290, 25, show_editor_edit_demand_change, button_none, 18, 0},
    {0, 318, 290, 25, show_editor_edit_demand_change, button_none, 19, 0},
    {300, 48, 290, 25, show_editor_edit_demand_change, button_none, 20, 0},
    {300, 78, 290, 25, show_editor_edit_demand_change, button_none, 21, 0},
    {300, 108, 290, 25, show_editor_edit_demand_change, button_none, 22, 0},
    {300, 138, 290, 25, show_editor_edit_demand_change, button_none, 23, 0},
    {300, 168, 290, 25, show_editor_edit_demand_change, button_none, 24, 0},
    {300, 198, 290, 25, show_editor_edit_demand_change, button_none, 25, 0},
    {300, 228, 290, 25, show_editor_edit_demand_change, button_none, 26, 0},
    {300, 258, 290, 25, show_editor_edit_demand_change, button_none, 27, 0},
    {300, 288, 290, 25, show_editor_edit_demand_change, button_none, 28, 0},
    {300, 318, 290, 25, show_editor_edit_demand_change, button_none, 29, 0},
    {600, 48, 290, 25, show_editor_edit_demand_change, button_none, 30, 0},
    {600, 78, 290, 25, show_editor_edit_demand_change, button_none, 31, 0},
    {600, 108, 290, 25, show_editor_edit_demand_change, button_none, 32, 0},
    {600, 138, 290, 25, show_editor_edit_demand_change, button_none, 33, 0},
    {600, 168, 290, 25, show_editor_edit_demand_change, button_none, 34, 0},
    {600, 198, 290, 25, show_editor_edit_demand_change, button_none, 35, 0},
    {600, 228, 290, 25, show_editor_edit_demand_change, button_none, 36, 0},
    {600, 258, 290, 25, show_editor_edit_demand_change, button_none, 37, 0},
    {600, 288, 290, 25, show_editor_edit_demand_change, button_none, 38, 0},
    {600, 318, 290, 25, show_editor_edit_demand_change, button_none, 39, 0},
};

static void draw_foreground_demand_changes(void)
{
    graphics_in_dialog();

    outer_panel_draw(-320, 0, 77, 23);
    lang_text_draw_centered(44, 94, -320, 16, 1232, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        int x, y;
        if (i < 10) {
            x = -300;
            y = 48 + 30 * i;
        } else if (i < 20) {
            x = 0;
            y = 48 + 30 * (i - 10);
        } else if (i < 30) {
            x = 300;
            y = 48 + 30 * (i - 20);
        } else {
            x = 600;
            y = 48 + 30 * (i - 30);
        }
        button_border_draw(x, y, 290, 25, focus_button_id_demand_changes == i + 1);

        if (scenario.demand_changes[i].resource && scenario.demand_changes[i].trade_city_id) {
            int width = lang_text_draw(25, scenario.demand_changes[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.demand_changes[i].year, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            image_draw(resource_images[scenario.demand_changes[i].resource].editor_icon_img_id + resource_image_offset(scenario.demand_changes[i].resource, RESOURCE_IMAGE_ICON), x + 12 + width, y + 3);
            width += lang_text_draw(44, 97, x + 45 + width, y + 6, FONT_NORMAL_BLACK);
            width += text_draw_number(scenario.demand_changes[i].trade_city_id, 0, 0, x + 48 + width, y + 6, FONT_NORMAL_BLACK);
            lang_text_draw(44, scenario.demand_changes[i].is_rise ? 99 : 98, x + 48 + width, y + 6, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_centered(44, 96, x, y + 6, 290, FONT_NORMAL_BLACK);
        }
    }

    graphics_reset_dialog();
}

static void handle_input_demand_changes(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_demand_changes, sizeof(buttons_demand_changes) / sizeof(struct generic_button_t), &focus_button_id_demand_changes)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        show_editor_attributes();
    }
}

static void show_editor_demand_changes(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_DEMAND_CHANGES,
        window_editor_map_draw_all,
        draw_foreground_demand_changes,
        handle_input_demand_changes,
    };
    window_show(&window);
}

static void button_demand_changes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    show_editor_demand_changes();
}

static struct generic_button_t buttons_editor_attributes[] = {
    {213, 60, 195, 30, button_briefing, button_none, 0, 0},
    {213, 100, 195, 30, change_climate, button_none, 0, 0},
    {213, 140, 195, 30, button_starting_conditions, button_none, 0, 0},
    {213, 180, 195, 30, button_win_criteria, button_none, 0, 0},
    {17, 220, 185, 30, button_allowed_buildings, button_none, 0, 0},
    {213, 220, 195, 30, button_special_events, button_none, 0, 0},
    {17, 260, 185, 30, show_editor_edit_requests, button_none, 0, 0},
    {213, 260, 195, 30, button_custom_messages, button_none, 0, 0},
    {17, 300, 185, 30, button_earthquakes, button_none, 0, 0},
    {213, 300, 195, 30, show_editor_edit_invasions, button_none, 0, 0},
    {17, 340, 185, 30, button_price_changes, button_none, 0, 0},
    {213, 340, 195, 30, button_demand_changes, button_none, 0, 0},
};

static void change_image_attributes(int value, __attribute__((unused)) int param2)
{
    if (scenario.brief_description_image_id == 15 && value == 1) {
        scenario.brief_description_image_id = 0;
    } else if (scenario.brief_description_image_id == 0 && value == -1) {
        scenario.brief_description_image_id = 15;
    } else {
        scenario.brief_description_image_id += value;
    }
    scenario.is_saved = 0;
    window_request_refresh();
}

static struct arrow_button_t image_arrows_attributes[] = {
    {19, 16, 19, 24, change_image_attributes, -1, 0, 0, 0},
    {43, 16, 21, 24, change_image_attributes, 1, 0, 0, 0},
};

static void draw_foreground_attributes(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 27, 25);

    arrow_buttons_draw(0, 0, image_arrows_attributes, 2);

    // Brief description
    input_box_draw(&scenario_description_input);

    // Brief description image
    button_border_draw(18, 60, 184, 144, 0);
    image_draw(image_group(GROUP_EDITOR_SCENARIO_IMAGE) + scenario.brief_description_image_id, 20, 62);

    // Briefing
    button_border_draw(213, 60, 195, 30, focus_button_id_attributes == 1);
    text_draw_centered(attribute_window_strings[0], 213, 69, 195, FONT_NORMAL_BLACK, COLOR_BLACK);

    // Terrain set
    button_border_draw(213, 100, 195, 30, focus_button_id_attributes == 2);
    text_draw_centered(climate_types_strings[scenario.climate], 213, 109, 195, FONT_NORMAL_BLACK, COLOR_BLACK);

    // Starting conditions
    button_border_draw(213, 140, 195, 30, focus_button_id_attributes == 3);
    lang_text_draw_centered(44, 88, 213, 149, 195, FONT_NORMAL_BLACK);

    // Win criteria
    button_border_draw(213, 180, 195, 30, focus_button_id_attributes == 4);
    lang_text_draw_centered(44, 45, 213, 189, 195, FONT_NORMAL_BLACK);

    // Buildings allowed
    button_border_draw(17, 220, 185, 30, focus_button_id_attributes == 5);
    lang_text_draw_centered(44, 44, 17, 229, 185, FONT_NORMAL_BLACK);

    // Special events
    button_border_draw(213, 220, 195, 30, focus_button_id_attributes == 6);
    lang_text_draw_centered(44, 49, 213, 229, 195, FONT_NORMAL_BLACK);

    // Requests
    button_border_draw(17, 260, 185, 30, focus_button_id_attributes == 7);
    if (scenario.requests[0].resource) {
        text_draw_centered(attribute_window_strings[1], 17, 269, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[2], 17, 269, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Custom messages
    button_border_draw(213, 260, 195, 30, focus_button_id_attributes == 8);
    if (scenario.editor_custom_messages[0].enabled) {
        text_draw_centered(attribute_window_strings[3], 213, 269, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[4], 213, 269, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Earthquakes
    button_border_draw(17, 300, 185, 30, focus_button_id_attributes == 9);
    if (scenario.earthquakes[0].state) {
        text_draw_centered(attribute_window_strings[5], 17, 309, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[6], 17, 309, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Invasions
    button_border_draw(213, 300, 195, 30, focus_button_id_attributes == 10);
    if (scenario.invasions[0].type) {
        text_draw_centered(attribute_window_strings[7], 213, 309, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[8], 213, 309, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Price changes
    button_border_draw(17, 340, 185, 30, focus_button_id_attributes == 11);
    if (scenario.price_changes[0].resource) {
        text_draw_centered(attribute_window_strings[9], 17, 349, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[10], 17, 349, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Demand changes
    button_border_draw(213, 340, 195, 30, focus_button_id_attributes == 12);
    if (scenario.demand_changes[0].resource && scenario.demand_changes[0].trade_city_id) {
        text_draw_centered(attribute_window_strings[11], 213, 349, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[12], 213, 349, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    graphics_reset_dialog();
}

static void handle_input_attributes(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_editor_attributes, sizeof(buttons_editor_attributes) / sizeof(struct generic_button_t), &focus_button_id_attributes) ||
        arrow_buttons_handle_mouse(m_dialog, 0, 0, image_arrows_attributes, 2, 0) ||
        image_buttons_handle_mouse(m, sidebar_common_get_x_offset_expanded(), 24, buttons_build, 1, 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        stop_brief_description_box_input();
        show_editor_map();
    }
}

static void show_editor_attributes(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_ATTRIBUTES,
        window_editor_map_draw_all,
        draw_foreground_attributes,
        handle_input_attributes,
    };
    string_copy(scenario.brief_description, brief_description, MAX_BRIEF_DESCRIPTION);
    input_box_start(&scenario_description_input);
    window_show(&window);
}

static void draw_foreground_top_menu_editor(void)
{
    if (!open_sub_menu_top_menu_editor) {
        return;
    }
    menu_draw(&top_menu_editor[open_sub_menu_top_menu_editor - 1], focus_sub_menu_id_top_menu_editor);
}

static void handle_input_top_menu_editor(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (open_sub_menu_top_menu_editor) {
        if (m->right.went_up || h->escape_pressed) {
            clear_state();
            window_go_back();
            return;
        }
        int menu_id = menu_bar_handle_mouse(m, top_menu_editor, sizeof(top_menu_editor) / sizeof(struct menu_bar_item_t), &focus_menu_id_top_menu_editor);
        if (menu_id && menu_id != open_sub_menu_top_menu_editor) {
            window_request_refresh();
            open_sub_menu_top_menu_editor = menu_id;
        }
        if (!menu_handle_mouse(m, &top_menu_editor[open_sub_menu_top_menu_editor - 1], &focus_sub_menu_id_top_menu_editor)) {
            if (m->left.went_up) {
                clear_state();
                window_go_back();
                return;
            }
        }
    } else {
        int menu_id = menu_bar_handle_mouse(m, top_menu_editor, sizeof(top_menu_editor) / sizeof(struct menu_bar_item_t), &focus_menu_id_top_menu_editor);
        if (menu_id && m->left.went_up) {
            open_sub_menu_top_menu_editor = menu_id;
            show_editor_top_menu_window();
        }
    }
}

static void show_editor_top_menu_window(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_TOP_MENU,
        window_editor_map_draw_all,
        draw_foreground_top_menu_editor,
        handle_input_top_menu_editor,
    };
    top_menu_editor[1].items[0].hidden = 0;
    window_show(&window);
}

static void place_rock_at(int x, int y, int grid_offset, int size)
{
    int x_size_adjusted = size > 1 ? size - 1 : size; // use x - 1 for medium/large rocks; technically causes image overlap but looks better
    // check if all needed tiles are clear and within the map grid
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) { // separate full x-size check to avoid protrusion beyond the map grid
            if (!map_grid_is_inside(x + dx, y - dy, 1)) {
                return;
            }
        }
        for (int dx = 0; dx < x_size_adjusted; dx++) {
            if (map_terrain_is(map_grid_offset(x + dx, y - dy), TERRAIN_NOT_DISPLACEABLE)) {
                return;
            }
        }
    }
    // set terrain rock for all covered tiles
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < x_size_adjusted; dx++) {
            terrain_grid.items[map_grid_offset(x + dx, y - dy)] = TERRAIN_ROCK;
        }
    }
    // set multitile rock image
    int image_id;
    if (size == 1) { // small rock
        image_id = map_random_get(grid_offset) & 7;
    } else if (size == 2) { // medium rock
        image_id = 8 + (map_random_get(grid_offset) & 3);
    } else { // large rock
        image_id = 12 + (map_random_get(grid_offset) & 1);
    }
    if (map_terrain_exists_tile_in_radius_with_type(x, y, size, 4, TERRAIN_ELEVATION)) {
        image_id += image_group(GROUP_TERRAIN_ELEVATION_ROCK);
    } else {
        image_id += image_group(GROUP_TERRAIN_ROCK);
    }
    map_image_set(grid_offset, image_id);
}

static void add_terrain_at(int x, int y)
{
    if (!map_grid_is_inside(x, y, 1)) {
        return;
    }
    int grid_offset = map_grid_offset(x, y);
    int shrub_rnd = map_random_get(grid_offset) & 7;
    int elevation = terrain_elevation.items[grid_offset];
    switch (tool_data.type) {
        case TOOL_GRASS:
            if (!map_terrain_is(grid_offset, TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP)) {
                if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
                    map_building_tiles_remove(0, x, y);
                }
                terrain_grid.items[grid_offset] = 0;
            }
            break;
        case TOOL_SMALL_SHRUB:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_SHRUB;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_SHRUB) + shrub_rnd);
            }
            break;
        case TOOL_MEDIUM_SHRUB:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_SHRUB;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_SHRUB) + shrub_rnd + 8);
            }
            break;
        case TOOL_LARGE_SHRUB:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_SHRUB;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_SHRUB) + shrub_rnd + 16);
            }
            break;
        case TOOL_LARGEST_SHRUB:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_SHRUB;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_SHRUB) + shrub_rnd + 24);
            }
            break;
        case TOOL_SMALL_ROCK:
            place_rock_at(x, y, grid_offset, 1);
            break;
        case TOOL_MEDIUM_ROCK:
            place_rock_at(x, y, grid_offset, 2);
            break;
        case TOOL_LARGE_ROCK:
            place_rock_at(x, y, grid_offset, 3);
            break;
        case TOOL_WATER:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_WATER;
            }
            break;
        case TOOL_TREES:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_TREE;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_TREE) + (map_random_get(grid_offset) & 7));
            }
            break;
        case TOOL_MEADOW:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_MEADOW;
                // dupl with set_meadow_image
                int random = map_random_get(grid_offset) & 3;
                int image_id = image_group(GROUP_TERRAIN_MEADOW);
                if (map_terrain_all_tiles_in_radius_are(x, y, 1, 2, TERRAIN_MEADOW)) {
                    map_image_set(grid_offset, image_id + random + 8);
                } else if (map_terrain_all_tiles_in_radius_are(x, y, 1, 1, TERRAIN_MEADOW)) {
                    map_image_set(grid_offset, image_id + random + 4);
                } else {
                    map_image_set(grid_offset, image_id + random);
                }
            }
            break;
        case TOOL_RAISE_LAND:
            if (elevation < 5 && elevation == tool_data.start_elevation) {
                map_property_set_multi_tile_size(grid_offset, 1);
                terrain_elevation.items[grid_offset] = elevation + 1;
                terrain_grid.items[grid_offset] = TERRAIN_ELEVATION;
            }
            break;
        case TOOL_LOWER_LAND:
            if (elevation == tool_data.start_elevation) {
                if (elevation == 1) {
                    terrain_elevation.items[grid_offset] = 0;
                    terrain_grid.items[grid_offset] = 0;
                } else if (elevation) {
                    terrain_elevation.items[grid_offset] = elevation - 1;
                    terrain_grid.items[grid_offset] = TERRAIN_ELEVATION;
                }
            }
            break;
        default:
            break;
    }
}

static void map_elevation_remove_cliffs(void)
{
    // elevation is max 5, so we need 4 passes to fix the lot
    for (int level = 0; level < 4; level++) {
        int grid_offset = map_data.start_offset;
        for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
            for (int x = 0; x < map_data.width; x++, grid_offset++) {
                if (terrain_elevation.items[grid_offset] > 0) {
                    // reduce elevation when the surrounding tiles are at least 2 lower
                    int max = terrain_elevation.items[grid_offset] - 1;
                    if (terrain_elevation.items[grid_offset + map_grid_delta(-1, 0)] < max ||
                        terrain_elevation.items[grid_offset + map_grid_delta(0, -1)] < max ||
                        terrain_elevation.items[grid_offset + map_grid_delta(1, 0)] < max ||
                        terrain_elevation.items[grid_offset + map_grid_delta(0, 1)] < max) {
                        terrain_elevation.items[grid_offset]--;
                    }
                }
            }
        }
    }
}

static void update_terrain_after_elevation_changes(void)
{
    map_elevation_remove_cliffs();
    map_image_context_reset_water();
    map_image_context_reset_elevation();
    map_tiles_update_all_elevation();
    map_tiles_update_all_empty_land();
    scenario.is_saved = 0;
}

static void end_editor_tool_use(const struct map_tile_t *tile)
{
    if (!tool_data.build_in_progress) {
        return;
    }
    tool_data.build_in_progress = 0;
    if (!tile->grid_offset) {
        return;
    }

    int warning = 0;
    switch (tool_data.type) {
        case TOOL_ENTRY_POINT:
            if (can_place_flag_editor(tool_data.type, tile, &warning)) {
                scenario.entry_point.x = tile->x;
                scenario.entry_point.y = tile->y;
            }
            break;
        case TOOL_EXIT_POINT:
            if (can_place_flag_editor(tool_data.type, tile, &warning)) {
                scenario.exit_point.x = tile->x;
                scenario.exit_point.y = tile->y;
            }
            break;
        case TOOL_RIVER_ENTRY_POINT:
            if (can_place_flag_editor(tool_data.type, tile, &warning)) {
                scenario.river_entry_point.x = tile->x;
                scenario.river_entry_point.y = tile->y;
                figure_create_flotsam();
                map_routing_update_water();
            }
            break;
        case TOOL_RIVER_EXIT_POINT:
            if (can_place_flag_editor(tool_data.type, tile, &warning)) {
                scenario.river_exit_point.x = tile->x;
                scenario.river_exit_point.y = tile->y;
                figure_create_flotsam();
                map_routing_update_water();
            }
            break;
        case TOOL_EARTHQUAKE_POINT:
            if (can_place_flag_editor(tool_data.type, tile, &warning)) {
                scenario.earthquake_points[tool_data.id].x = tile->x;
                scenario.earthquake_points[tool_data.id].y = tile->y;
                window_request_refresh();
            }
            break;
        case TOOL_INVASION_POINT:
            if (can_place_flag_editor(tool_data.type, tile, &warning)) {
                scenario.invasion_points[tool_data.id].x = tile->x;
                scenario.invasion_points[tool_data.id].y = tile->y;
            }
            break;
        case TOOL_FISHING_POINT:
            if (can_place_flag_editor(tool_data.type, tile, &warning)) {
                scenario.fishing_points[tool_data.id].x = tile->x;
                scenario.fishing_points[tool_data.id].y = tile->y;
            }
            break;
        case TOOL_HERD_POINT:
            if (can_place_flag_editor(tool_data.type, tile, &warning)) {
                scenario.herd_points[tool_data.id].x = tile->x;
                scenario.herd_points[tool_data.id].y = tile->y;
            }
            break;
        case TOOL_NATIVE_CENTER:
        case TOOL_NATIVE_FIELD:
        case TOOL_NATIVE_HUT:
        case TOOL_HOUSE_VACANT_LOT:
            int image_id;
            int size;
            int type;
            switch (tool_data.type) {
                case TOOL_NATIVE_HUT:
                    type = BUILDING_NATIVE_HUT;
                    image_id = image_group(GROUP_EDITOR_BUILDING_NATIVE) + (random_byte() & 1);
                    size = 1;
                    break;
                case TOOL_NATIVE_CENTER:
                    type = BUILDING_NATIVE_MEETING;
                    image_id = image_group(GROUP_EDITOR_BUILDING_NATIVE) + 2;
                    size = 2;
                    break;
                case TOOL_NATIVE_FIELD:
                    type = BUILDING_NATIVE_CROPS;
                    image_id = image_group(GROUP_EDITOR_BUILDING_CROPS);
                    size = 1;
                    break;
                case TOOL_HOUSE_VACANT_LOT:
                    type = BUILDING_HOUSE_VACANT_LOT;
                    image_id = image_group(GROUP_EDITOR_BUILDING_NATIVE) - 4;
                    size = 1;
                    break;
                default:
                    return;
            }
            if (can_place_building_editor(tile, size * size, 0)) {
                struct building_t *b = building_create(type, tile->x, tile->y);
                map_building_tiles_add(b->id, tile->x, tile->y, size, image_id, TERRAIN_BUILDING);
                scenario.is_saved = 0;
            } else {
                city_warning_show(WARNING_EDITOR_CANNOT_PLACE);
            }
            break;
        case TOOL_RAISE_LAND:
        case TOOL_LOWER_LAND:
            update_terrain_after_elevation_changes();
            break;
        case TOOL_ACCESS_RAMP:
            int orientation = 0;
            if (can_place_access_ramp_editor(tile, &orientation)) {
                int terrain_mask = ~(TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_GARDEN | TERRAIN_AQUEDUCT);
                for (int dy = 0; dy < 2; dy++) {
                    for (int dx = 0; dx < 2; dx++) {
                        int grid_offset = tile->grid_offset + map_grid_delta(dx, dy);
                        terrain_grid.items[grid_offset] = terrain_grid.items[grid_offset] & terrain_mask;
                    }
                }
                map_building_tiles_add(0, tile->x, tile->y, 2,
                    image_group(GROUP_TERRAIN_ACCESS_RAMP) + orientation, TERRAIN_ACCESS_RAMP);

                update_terrain_after_elevation_changes();
                scenario.is_saved = 0;
            } else {
                city_warning_show(WARNING_EDITOR_CANNOT_PLACE);
            }
            break;
        case TOOL_ROAD:
            building_construction_place_road(0, tool_data.start_tile.x, tool_data.start_tile.y, tile->x, tile->y);
            break;
        default:
            break;
    }
    scenario.is_saved = 0;
    if (warning) {
        city_warning_show(warning);
    }
}

static void editor_tool_deactivate(void)
{
    if (tool_data.type == TOOL_ROAD && tool_data.build_in_progress) {
        game_undo_restore_map(1);
        tool_data.build_in_progress = 0;
    } else {
        tool_data.active = 0;
    }
}

static void handle_input_editor_map(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (h->load_file) {
        window_file_dialog_show(FILE_TYPE_SCENARIO, FILE_DIALOG_LOAD);
        return;
    }
    if (h->save_file) {
        window_file_dialog_show(FILE_TYPE_SCENARIO, FILE_DIALOG_SAVE);
        return;
    }
    if (h->show_empire_map) {
        show_editor_empire(0, 0);
        return;
    }
    if (widget_minimap_handle_mouse(m)) {
        return;
    }
    if (image_buttons_handle_mouse(m, sidebar_common_get_x_offset_expanded(), 24, buttons_build, sizeof(buttons_build) / sizeof(struct image_button_t), 0)) {
        return;
    }
    int x_offset, y_offset, width, height;
    city_view_get_viewport(&x_offset, &y_offset, &width, &height);
    if (m->right.went_down
    && (m->x - x_offset >= 0 && m->x - x_offset < width && m->y - y_offset >= 0 && m->y - y_offset < height) // mouse on map
    && !tool_data.active) {
        scroll_drag_start();
    }

    struct map_tile_t *tile = &current_tile;
    update_city_view_coords(m->x, m->y, tile);
    if (m->left.went_up && tool_data.build_in_progress) {
        end_editor_tool_use(tile);
        play_sound_effect(SOUND_EFFECT_BUILD);
    }
    if (m->right.went_up) {
        if (!tool_data.active) {
            int has_scrolled = scroll_drag_end();
            if (!has_scrolled) {
                editor_tool_deactivate();
            }
        } else {
            editor_tool_deactivate();
            end_editor_tool_use(tile);
        }
    }
    if (h->escape_pressed) {
        if (tool_data.active) {
            editor_tool_deactivate();
            end_editor_tool_use(tile);
        } else {
            request_exit_editor();
        }
    }
    if (tile->grid_offset) {
        if (m->left.went_down) {
            if (!tool_data.build_in_progress) {
                // start tool use
                if (!tool_data.active) {
                    return;
                }
                tool_data.build_in_progress = 1;
                tool_data.start_elevation = terrain_elevation.items[tile->grid_offset];
                tool_data.start_tile = *tile;
                if (tool_data.type == TOOL_ROAD) {
                    game_undo_start_build(BUILDING_ROAD);
                    map_routing_update_land();
                }
            }
        } else if (m->left.is_down || tool_data.build_in_progress) {
            if (tool_data.type == TOOL_ROAD) {
                building_construction_place_road(1, tool_data.start_tile.x, tool_data.start_tile.y, tile->x, tile->y);
                return;
            }
            switch (tool_data.type) {
                case TOOL_GRASS:
                case TOOL_SMALL_SHRUB:
                case TOOL_MEDIUM_SHRUB:
                case TOOL_LARGE_SHRUB:
                case TOOL_LARGEST_SHRUB:
                case TOOL_WATER:
                case TOOL_TREES:
                case TOOL_SMALL_ROCK:
                case TOOL_MEDIUM_ROCK:
                case TOOL_LARGE_ROCK:
                case TOOL_MEADOW:
                case TOOL_RAISE_LAND:
                case TOOL_LOWER_LAND:
                    break;
                default:
                    return;
            }
            add_terrain_at(tile->x, tile->y);
            int tiles_remaining = tool_data.brush_size;
            while (tiles_remaining) {
                for (int i = 1; i <= tiles_remaining; i++) {
                    add_terrain_at(tile->x + (tool_data.brush_size - tiles_remaining), tile->y - i); // top to right
                    add_terrain_at(tile->x + i, tile->y + (tool_data.brush_size - tiles_remaining)); // right to bottom
                    add_terrain_at(tile->x - (tool_data.brush_size - tiles_remaining), tile->y + i); // bottom to left
                    add_terrain_at(tile->x - i, tile->y - (tool_data.brush_size - tiles_remaining)); // left to top
                }
                tiles_remaining--;
            }
            int x_min = tile->x - tool_data.brush_size;
            int x_max = tile->x + tool_data.brush_size;
            int y_min = tile->y - tool_data.brush_size;
            int y_max = tile->y + tool_data.brush_size;
            map_image_context_reset_water();
            foreach_region_tile(x_min, y_min, x_max, y_max, update_water_tile);
            switch (tool_data.type) {
                case TOOL_GRASS:
                    foreach_region_tile(x_min, y_min, x_max, y_max, update_water_tile);
                    map_tiles_update_region_empty_land(x_min, y_min, x_max, y_max);
                    break;
                case TOOL_RAISE_LAND:
                case TOOL_LOWER_LAND:
                    map_image_context_reset_water();
                    map_image_context_reset_elevation();
                    map_tiles_update_all_elevation();
                    foreach_region_tile(x_min, y_min, x_max, y_max, update_water_tile);
                    map_tiles_update_region_empty_land(x_min, y_min, x_max, y_max);
                    break;
                default:
                    break;
            }
            scenario.is_saved = 0;
            widget_minimap_invalidate();
        }
    }
    scroll_map(m);
    handle_input_top_menu_editor(m, h);
}

void show_editor_map(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_MAP,
        draw_background_editor_map,
        draw_foreground_editor_map,
        handle_input_editor_map,
    };
    window_show(&window);
}







static int editor_active;

void editor_set_active(int active)
{
    editor_active = active;
}

int editor_is_active(void)
{
    return editor_active;
}
