#include "military.h"

#include "city/data_private.h"
#include "city/military.h"
#include "city/view.h"
#include "figure/formation_legion.h"
#include "graphics/generic_button.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "map/grid.h"
#include "scenario/data.h"
#include "window/city.h"

#define ADVISOR_HEIGHT 26

static void button_go_to_legion(int legion_id, int param2);
static void button_return_to_fort(int legion_id, int param2);
static void button_empire_service(int legion_id, int param2);

static generic_button fort_buttons[] = {
    {400, 83, 30, 30, button_go_to_legion, button_none, 1, 0},
    {480, 83, 30, 30, button_return_to_fort, button_none, 1, 0},
    {560, 83, 30, 30, button_empire_service, button_none, 1, 0},
    {400, 127, 30, 30, button_go_to_legion, button_none, 2, 0},
    {480, 127, 30, 30, button_return_to_fort, button_none, 2, 0},
    {560, 127, 30, 30, button_empire_service, button_none, 2, 0},
    {400, 171, 30, 30, button_go_to_legion, button_none, 3, 0},
    {480, 171, 30, 30, button_return_to_fort, button_none, 3, 0},
    {560, 171, 30, 30, button_empire_service, button_none, 3, 0},
    {400, 215, 30, 30, button_go_to_legion, button_none, 4, 0},
    {480, 215, 30, 30, button_return_to_fort, button_none, 4, 0},
    {560, 215, 30, 30, button_empire_service, button_none, 4, 0},
    {400, 259, 30, 30, button_go_to_legion, button_none, 5, 0},
    {480, 259, 30, 30, button_return_to_fort, button_none, 5, 0},
    {560, 259, 30, 30, button_empire_service, button_none, 5, 0},
    {400, 303, 30, 30, button_go_to_legion, button_none, 6, 0},
    {480, 303, 30, 30, button_return_to_fort, button_none, 6, 0},
    {560, 303, 30, 30, button_empire_service, button_none, 6, 0},
};

static int focus_button_id;
static int num_legions;

static struct formation_t *find_next_smallest_legion_id_formation_to_draw(int from_index, int last_legion_id_drawn)
{
    int smallest_legion_id = MAX_LEGIONS;
    struct formation_t *eligible_formation = 0;
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && formations[i].is_legion
        && formations[i].legion_id >= from_index
        && formations[i].legion_id > last_legion_id_drawn
        && formations[i].legion_id < smallest_legion_id) {
            smallest_legion_id = formations[i].legion_id;
            eligible_formation = &formations[i];

        }
    }
    return eligible_formation;
}

static int draw_background(void)
{
    outer_panel_draw(0, 0, 40, ADVISOR_HEIGHT);
    // Military advisor icon
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 1, 10, 10);
    // Morale
    lang_text_draw(138, 36, 290, 58, FONT_SMALL_PLAIN);
    // Legion status
    lang_text_draw(51, 0, 60, 12, FONT_LARGE_BLACK);
    // Button headers
    lang_text_draw(51, 1, 390, 43, FONT_SMALL_PLAIN);
    lang_text_draw(51, 2, 390, 58, FONT_SMALL_PLAIN);
    lang_text_draw(51, 3, 470, 43, FONT_SMALL_PLAIN);
    lang_text_draw(51, 4, 470, 58, FONT_SMALL_PLAIN);
    lang_text_draw(51, 5, 550, 43, FONT_SMALL_PLAIN);
    lang_text_draw(51, 6, 550, 58, FONT_SMALL_PLAIN);

    int enemy_text_id;
    if (city_data.figure.enemies) {
        enemy_text_id = 10;
    } else if (city_data.figure.imperial_soldiers) {
        enemy_text_id = 11;
    } else if (scenario.invasion_upcoming) {
        enemy_text_id = 9;
    } else {
        enemy_text_id = 8;
    }
    int distant_battle_text_id;
    if (city_data.distant_battle.roman_months_to_travel_back) {
        distant_battle_text_id = 15;
    } else if (city_data.distant_battle.roman_months_to_travel_forth) {
        distant_battle_text_id = 14;
    } else if (city_data.distant_battle.months_until_battle) {
        distant_battle_text_id = 13;
    } else {
        distant_battle_text_id = 12;
    }

    inner_panel_draw(32, 70, 36, 17);
    if (num_legions) {
        int last_legion_id_drawn = -1;
        for (int i = 0; i < num_legions; i++) {
            struct formation_t *m = find_next_smallest_legion_id_formation_to_draw(i, last_legion_id_drawn);
            if (m) {
                button_border_draw(38, 33 + 44 * (i + 1), 560, 40, 0);
                image_draw(image_group(GROUP_FIGURE_FORT_STANDARD_ICONS) + m->legion_id, 48, 38 + 44 * (i + 1));
                lang_text_draw(138, m->legion_id, 100, 39 + 44 * (i + 1), FONT_NORMAL_WHITE);
                int width = text_draw_number(m->num_figures, ' ', "", 100, 56 + 44 * (i + 1), FONT_NORMAL_GREEN);
                switch (m->figure_type) {
                    case FIGURE_FORT_LEGIONARY:
                        lang_text_draw(138, 33, 100 + width, 56 + 44 * (i + 1), FONT_NORMAL_GREEN);
                        break;
                    case FIGURE_FORT_MOUNTED:
                        lang_text_draw(138, 34, 100 + width, 56 + 44 * (i + 1), FONT_NORMAL_GREEN);
                        break;
                    case FIGURE_FORT_JAVELIN:
                        lang_text_draw(138, 35, 100 + width, 56 + 44 * (i + 1), FONT_NORMAL_GREEN);
                        break;
                }
                lang_text_draw_centered(138, 37 + m->morale / 5, 240, 47 + 44 * (i + 1), 150, FONT_NORMAL_GREEN);

                int image_id = image_group(GROUP_FORT_ICONS);
                button_border_draw(400, 39 + 44 * (i + 1), 30, 30, 0);
                image_draw(image_id, 403, 42 + 44 * (i + 1));

                button_border_draw(480, 39 + 44 * (i + 1), 30, 30, 0);
                if (m->is_at_rest || m->in_distant_battle) {
                    image_draw(image_id + 2, 483, 42 + 44 * (i + 1));
                } else {
                    image_draw(image_id + 1, 483, 42 + 44 * (i + 1));
                }

                button_border_draw(560, 39 + 44 * (i + 1), 30, 30, 0);
                if (m->empire_service) {
                    image_draw(image_id + 3, 563, 42 + 44 * (i + 1));
                } else {
                    image_draw(image_id + 4, 563, 42 + 44 * (i + 1));
                }
                last_legion_id_drawn = m->legion_id;
            }
        }
        // x soldiers in y legions
        image_draw(image_group(GROUP_BULLET), 60, 349);
        int total_soldiers = 0;
        for (int i = 1; i < MAX_FORMATIONS; i++) {
            if (formations[i].in_use && formations[i].is_legion) {
                total_soldiers += formations[i].num_figures;
            }
        }
        int width = lang_text_draw_amount(8, 46, total_soldiers, 80, 348, FONT_NORMAL_BLACK);
        width += lang_text_draw(51, 7, 80 + width, 348, FONT_NORMAL_BLACK);
        lang_text_draw_amount(8, 48, city_data.military.total_legions, 80 + width, 348, FONT_NORMAL_BLACK);
        // Enemy threat status
        image_draw(image_group(GROUP_BULLET), 60, 369);
        lang_text_draw(51, enemy_text_id, 80, 368, FONT_NORMAL_BLACK);
        // Distant battle status
        image_draw(image_group(GROUP_BULLET), 60, 389);
        lang_text_draw(51, distant_battle_text_id, 80, 388, FONT_NORMAL_BLACK);
    } else {
        // You have no legions to command
        lang_text_draw_multiline(51, 16, 64, 200, 496, FONT_NORMAL_GREEN);
        // Enemy threat status
        image_draw(image_group(GROUP_BULLET), 60, 359);
        lang_text_draw(51, enemy_text_id, 80, 358, FONT_NORMAL_BLACK);
        // Distant battle status
        image_draw(image_group(GROUP_BULLET), 60, 379);
        lang_text_draw(51, distant_battle_text_id, 80, 378, FONT_NORMAL_BLACK);
    }

    return ADVISOR_HEIGHT;
}

static void draw_foreground(void)
{
    for (int i = 0; i < num_legions; i++) {
        button_border_draw(400, 83 + 44 * i, 30, 30, focus_button_id == 3 * i + 1);
        button_border_draw(480, 83 + 44 * i, 30, 30, focus_button_id == 3 * i + 2);
        button_border_draw(560, 83 + 44 * i, 30, 30, focus_button_id == 3 * i + 3);
    }
}

static int handle_mouse(const mouse *m)
{
    return generic_buttons_handle_mouse(m, 0, 0, fort_buttons, 3 * num_legions, &focus_button_id);
}

static void button_go_to_legion(int legion_id, __attribute__((unused)) int param2)
{
    const struct formation_t *m = &formations[get_legion_formation_by_index(legion_id)];
    city_view_go_to_grid_offset(map_grid_offset(m->standard_x, m->standard_y));
    window_city_show();
}

static void button_return_to_fort(int legion_id, __attribute__((unused)) int param2)
{
    struct formation_t *m = &formations[get_legion_formation_by_index(legion_id)];
    if (!m->in_distant_battle && !m->is_at_rest) {
        formation_legion_return_home(m);
        window_invalidate();
    }
}

static void button_empire_service(int legion_id, __attribute__((unused)) int param2)
{
    int formation_id = get_legion_formation_by_index(legion_id);
    if (!formations[formation_id].in_distant_battle) {
        formations[formation_id].empire_service = formations[formation_id].empire_service ? 0 : 1;
        window_invalidate();
    }

}

const advisor_window_type *window_advisor_military(void)
{
    num_legions = formation_get_num_legions();
    static const advisor_window_type window = {
        draw_background,
        draw_foreground,
        handle_mouse,
        0
    };
    return &window;
}
