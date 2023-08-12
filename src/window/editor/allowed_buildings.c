#include "allowed_buildings.h"

#include "building/building.h"
#include "empire/object.h"
#include "graphics/button.h"
#include "graphics/color.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/scenario.h"
#include "window/editor/attributes.h"
#include "window/editor/map.h"

static void toggle_building(int id, int param2);

static struct generic_button_t buttons[] = {
    {-138, 50, 190, 20, toggle_building, button_none, 1, 0},
    {-138, 70, 190, 20, toggle_building, button_none, 2, 0},
    {-138, 90, 190, 20, toggle_building, button_none, 3, 0},
    {-138, 110, 190, 20, toggle_building, button_none, 4, 0},
    {-138, 130, 190, 20, toggle_building, button_none, 5, 0},
    {-138, 150, 190, 20, toggle_building, button_none, 6, 0},
    {-138, 170, 190, 20, toggle_building, button_none, 7, 0},
    {-138, 190, 190, 20, toggle_building, button_none, 8, 0},
    {-138, 210, 190, 20, toggle_building, button_none, 9, 0},
    {-138, 230, 190, 20, toggle_building, button_none, 10, 0},
    {-138, 250, 190, 20, toggle_building, button_none, 11, 0},
    {-138, 270, 190, 20, toggle_building, button_none, 12, 0},
    {-138, 290, 190, 20, toggle_building, button_none, 13, 0},
    {-138, 310, 190, 20, toggle_building, button_none, 14, 0},
    {-138, 330, 190, 20, toggle_building, button_none, 15, 0},
    {-138, 350, 190, 20, toggle_building, button_none, 16, 0},
    {-138, 370, 190, 20, toggle_building, button_none, 17, 0},
    {-138, 390, 190, 20, toggle_building, button_none, 18, 0},
    {-138, 410, 190, 20, toggle_building, button_none, 19, 0},
    {-138, 430, 190, 20, toggle_building, button_none, 20, 0},
    {61, 50, 190, 20, toggle_building, button_none, 21, 0},
    {61, 70, 190, 20, toggle_building, button_none, 22, 0},
    {61, 90, 190, 20, toggle_building, button_none, 23, 0},
    {61, 110, 190, 20, toggle_building, button_none, 24, 0},
    {61, 130, 190, 20, toggle_building, button_none, 25, 0},
    {61, 150, 190, 20, toggle_building, button_none, 26, 0},
    {61, 170, 190, 20, toggle_building, button_none, 27, 0},
    {61, 190, 190, 20, toggle_building, button_none, 28, 0},
    {61, 210, 190, 20, toggle_building, button_none, 29, 0},
    {61, 230, 190, 20, toggle_building, button_none, 30, 0},
    {61, 250, 190, 20, toggle_building, button_none, 31, 0},
    {61, 270, 190, 20, toggle_building, button_none, 32, 0},
    {61, 290, 190, 20, toggle_building, button_none, 33, 0},
    {61, 310, 190, 20, toggle_building, button_none, 34, 0},
    {61, 330, 190, 20, toggle_building, button_none, 35, 0},
    {61, 350, 190, 20, toggle_building, button_none, 36, 0},
    {61, 370, 190, 20, toggle_building, button_none, 37, 0},
    {61, 390, 190, 20, toggle_building, button_none, 38, 0},
    {61, 410, 190, 20, toggle_building, button_none, 39, 0},
    {61, 430, 190, 20, toggle_building, button_none, 40, 0},
    {260, 50, 190, 20, toggle_building, button_none, 41, 0},
    {260, 70, 190, 20, toggle_building, button_none, 42, 0},
    {260, 90, 190, 20, toggle_building, button_none, 43, 0},
    {260, 110, 190, 20, toggle_building, button_none, 44, 0},
    {260, 130, 190, 20, toggle_building, button_none, 45, 0},
    {260, 150, 190, 20, toggle_building, button_none, 46, 0},
    {260, 170, 190, 20, toggle_building, button_none, 47, 0},
    {260, 190, 190, 20, toggle_building, button_none, 48, 0},
    {260, 210, 190, 20, toggle_building, button_none, 49, 0},
    {260, 230, 190, 20, toggle_building, button_none, 50, 0},
    {260, 250, 190, 20, toggle_building, button_none, 51, 0},
    {260, 270, 190, 20, toggle_building, button_none, 52, 0},
    {260, 290, 190, 20, toggle_building, button_none, 53, 0},
    {260, 310, 190, 20, toggle_building, button_none, 54, 0},
    {260, 330, 190, 20, toggle_building, button_none, 55, 0},
    {260, 350, 190, 20, toggle_building, button_none, 56, 0},
    {260, 370, 190, 20, toggle_building, button_none, 57, 0},
    {260, 390, 190, 20, toggle_building, button_none, 58, 0},
    {260, 410, 190, 20, toggle_building, button_none, 59, 0},
    {260, 430, 190, 20, toggle_building, button_none, 60, 0},
    {459, 50, 190, 20, toggle_building, button_none, 61, 0},
    {459, 70, 190, 20, toggle_building, button_none, 62, 0},
    {459, 90, 190, 20, toggle_building, button_none, 63, 0},
    {459, 110, 190, 20, toggle_building, button_none, 64, 0},
    {459, 130, 190, 20, toggle_building, button_none, 65, 0},
    {459, 150, 190, 20, toggle_building, button_none, 66, 0},
    {459, 170, 190, 20, toggle_building, button_none, 67, 0},
    {459, 190, 190, 20, toggle_building, button_none, 68, 0},
    {459, 210, 190, 20, toggle_building, button_none, 69, 0},
    {459, 230, 190, 20, toggle_building, button_none, 70, 0},
    {459, 250, 190, 20, toggle_building, button_none, 71, 0},
    {459, 270, 190, 20, toggle_building, button_none, 72, 0},
    {459, 290, 190, 20, toggle_building, button_none, 73, 0},
    {459, 310, 190, 20, toggle_building, button_none, 74, 0},
    {459, 330, 190, 20, toggle_building, button_none, 75, 0},
    {459, 350, 190, 20, toggle_building, button_none, 76, 0},
    {459, 370, 190, 20, toggle_building, button_none, 77, 0},
    {459, 390, 190, 20, toggle_building, button_none, 78, 0},
};

static int focus_button_id;

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
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
        button_border_draw(x, y, 190, 20, focus_button_id == i + 1);
        int building_index = align_bulding_type_index_to_strings(i + 1);
        if (scenario.allowed_buildings[building_index]) {
            text_draw_centered(all_buildings_strings[building_index], x, y + 5, 190, FONT_NORMAL_BLACK, 0);
        } else {
            text_draw_centered(all_buildings_strings[building_index], x, y + 5, 190, FONT_NORMAL_PLAIN, COLOR_FONT_RED);
        }

    }

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons, MAX_ALLOWED_BUILDINGS, &focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        empire_object_our_city_set_resources_sell();
        window_editor_attributes_show();
    }
}

void toggle_building(int id, __attribute__((unused)) int param2)
{
    // sync with building types index
    int building_index = align_bulding_type_index_to_strings(id);

    scenario.allowed_buildings[building_index] = !scenario.allowed_buildings[building_index];
    scenario.is_saved = 0;
}

void window_editor_allowed_buildings_show(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_ALLOWED_BUILDINGS,
        draw_background,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}
