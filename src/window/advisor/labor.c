#include "labor.h"

#include "city/data.h"
#include "city/finance.h"
#include "core/calc.h"
#include "graphics/arrow_button.h"
#include "graphics/generic_button.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "window/labor_priority.h"

static void arrow_button_wages(int value, int param2);
static void button_priority(int category, int param2);

static struct generic_button_t category_buttons[] = {
    {40, 77, 560, 22, button_priority, button_none, 0, 0},
    {40, 102, 560, 22, button_priority, button_none, 1, 0},
    {40, 127, 560, 22, button_priority, button_none, 2, 0},
    {40, 152, 560, 22, button_priority, button_none, 3, 0},
    {40, 177, 560, 22, button_priority, button_none, 4, 0},
    {40, 202, 560, 22, button_priority, button_none, 5, 0},
    {40, 227, 560, 22, button_priority, button_none, 6, 0},
    {40, 252, 560, 22, button_priority, button_none, 7, 0},
    {40, 277, 560, 22, button_priority, button_none, 8, 0},
};

static struct arrow_button_t wage_arrow_buttons[] = {
    {158, 354, 17, 24, arrow_button_wages, -1, 0, 0, 0},
    {182, 354, 15, 24, arrow_button_wages, 1, 0, 0, 0}
};

static int focus_button_id;
static int arrow_button_focus;

static int draw_background(void)
{
    outer_panel_draw(0, 0, 40, 26);

    // Labor advisor icon
    image_draw(image_group(GROUP_ADVISOR_ICONS), 10, 10);

    // Labor Allocation
    lang_text_draw(50, 0, 60, 12, FONT_LARGE_BLACK);

    // Priority/Sector/Need/Have
    lang_text_draw(50, 21, 60, 56, FONT_SMALL_PLAIN);
    lang_text_draw(50, 22, 170, 56, FONT_SMALL_PLAIN);
    lang_text_draw(50, 23, 400, 56, FONT_SMALL_PLAIN);
    lang_text_draw(50, 24, 500, 56, FONT_SMALL_PLAIN);

    inner_panel_draw(32, 70, 36, 15);

    // Employed workforce
    int width = text_draw_number(city_data.labor.workers_employed, 0, 0, 32, 320, FONT_NORMAL_BLACK);
    lang_text_draw(50, 12, 32 + width, 320, FONT_NORMAL_BLACK);
    // Unemployed workforce
    width = text_draw_number(city_data.labor.workers_unemployed, 0, 0, 320, 320, FONT_NORMAL_BLACK);
    width += lang_text_draw(50, 13, 320 + width, 320, FONT_NORMAL_BLACK);
    text_draw_number(city_data.labor.unemployment_percentage, 0, "%)", 314 + width, 320, FONT_NORMAL_BLACK);

    // Wages panel
    inner_panel_draw(64, 350, 32, 2);
    lang_text_draw(50, 14, 80, 359, FONT_NORMAL_WHITE);
    text_draw_number(city_data.labor.wages, 0, 0, 222, 359, FONT_NORMAL_WHITE);
    lang_text_draw(50, 15, 254, 359, FONT_NORMAL_WHITE);
    lang_text_draw(50, 18, 330, 359, FONT_NORMAL_WHITE);
    text_draw_number(city_data.labor.wages_rome, 0, ")", 430, 359, FONT_NORMAL_WHITE);

    // Estimated annual bill
    lang_text_draw(50, 19, 64, 388, FONT_NORMAL_BLACK);
    text_draw_money(city_data.finance.estimated_wages, 255, 388, FONT_NORMAL_BLACK);

    // outer panel draw height
    return 26;
}

static void draw_foreground(void)
{
    // Industry stats
    for (int i = 0; i < 9; i++) {
        button_border_draw(40, 77 + 25 * i, 560, 22, i == focus_button_id - 1);
        struct labor_category_data_t *cat = city_labor_category(i);
        if (cat->priority) {
            image_draw(image_group(GROUP_LABOR_PRIORITY_LOCK), 70, 80 + 25 * i);
            text_draw_number(cat->priority, 0, 0, 90, 82 + 25 * i, FONT_NORMAL_WHITE);
        }
        lang_text_draw(50, i + 1, 170, 82 + 25 * i, FONT_NORMAL_WHITE);
        text_draw_number(cat->workers_needed, 0, 0, 410, 82 + 25 * i, FONT_NORMAL_WHITE);
        if (cat->workers_needed > cat->workers_allocated) {
            text_draw_number(cat->workers_allocated, 0, 0, 510, 82 + 25 * i, FONT_NORMAL_RED);
        } else {
            text_draw_number(cat->workers_allocated, 0, 0, 510, 82 + 25 * i, FONT_NORMAL_WHITE);
        }
    }

    arrow_buttons_draw(0, 0, wage_arrow_buttons, 2);
}

static int handle_mouse(const struct mouse_t *m)
{
    if (generic_buttons_handle_mouse(m, 0, 0, category_buttons, sizeof(category_buttons) / sizeof(struct generic_button_t), &focus_button_id)) {
        return 1;
    }
    if (arrow_buttons_handle_mouse(m, 0, 0, wage_arrow_buttons, 2, &arrow_button_focus)) {
        return 1;
    }
    return 0;
}

static void arrow_button_wages(int value, __attribute__((unused)) int param2)
{
    city_data.labor.wages = calc_bound(city_data.labor.wages + value, 0, 100);
    city_finance_estimate_wages();
    city_finance_calculate_totals();
    window_invalidate();
}

static void button_priority(int category, __attribute__((unused)) int param2)
{
    window_labor_priority_show(category);
}

struct advisor_window_type_t *window_advisor_labor(void)
{
    static struct advisor_window_type_t window = {
        draw_background,
        draw_foreground,
        handle_mouse,
    };
    return &window;
}
