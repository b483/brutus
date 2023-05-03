#include "financial.h"

#include "city/data_private.h"
#include "city/finance.h"
#include "core/calc.h"
#include "graphics/arrow_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"

#define ADVISOR_HEIGHT 26

static void button_change_taxes(int value, int param2);

static arrow_button arrow_buttons_taxes[] = {
    {180, 75, 17, 24, button_change_taxes, -1, 0, 0, 0},
    {204, 75, 15, 24, button_change_taxes, 1, 0, 0, 0}
};

static int arrow_button_focus;

static void draw_row(int group, int number, int y, int value_last_year, int value_this_year)
{
    lang_text_draw(group, number, 80, y, FONT_NORMAL_BLACK);
    text_draw_number(value_last_year, '@', " ", 290, y, FONT_NORMAL_BLACK);
    text_draw_number(value_this_year, '@', " ", 430, y, FONT_NORMAL_BLACK);
}

static int draw_background(void)
{
    outer_panel_draw(0, 0, 40, ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 10, 10, 10);

    lang_text_draw(60, 0, 60, 12, FONT_LARGE_BLACK);
    inner_panel_draw(64, 48, 34, 5);

    int width;
    if (city_data.finance.treasury < 0) {
        width = lang_text_draw(60, 3, 70, 58, FONT_NORMAL_RED);
        lang_text_draw_amount(8, 0, -city_data.finance.treasury, 72 + width, 58, FONT_NORMAL_RED);
    } else {
        width = lang_text_draw(60, 2, 70, 58, FONT_NORMAL_WHITE);
        lang_text_draw_amount(8, 0, city_data.finance.treasury, 72 + width, 58, FONT_NORMAL_WHITE);
    }

    // tax percentage and estimated income
    lang_text_draw(60, 1, 70, 81, FONT_NORMAL_WHITE);
    width = text_draw_percentage(city_data.finance.tax_percentage, 240, 81, FONT_NORMAL_WHITE);
    width += lang_text_draw(60, 4, 240 + width, 81, FONT_NORMAL_WHITE);
    lang_text_draw_amount(8, 0, city_data.finance.estimated_tax_income, 240 + width, 81, FONT_NORMAL_WHITE);

    // percentage taxpayers
    width = text_draw_percentage(city_data.taxes.percentage_taxed_people, 70, 103, FONT_NORMAL_WHITE);
    lang_text_draw(60, 5, 70 + width, 103, FONT_NORMAL_WHITE);

    // table headers
    lang_text_draw(60, 6, 270, 133, FONT_NORMAL_BLACK);
    lang_text_draw(60, 7, 400, 133, FONT_NORMAL_BLACK);

    // income
    draw_row(60, 8, 155, city_data.finance.last_year.income.taxes, city_data.finance.this_year.income.taxes);
    draw_row(60, 9, 170, city_data.finance.last_year.income.exports, city_data.finance.this_year.income.exports);
    draw_row(60, 20, 185, city_data.finance.last_year.income.donated, city_data.finance.this_year.income.donated);

    graphics_draw_horizontal_line(280, 350, 198, COLOR_BLACK);
    graphics_draw_horizontal_line(420, 490, 198, COLOR_BLACK);

    draw_row(60, 10, 203, city_data.finance.last_year.income.total, city_data.finance.this_year.income.total);

    // expenses
    draw_row(60, 11, 227, city_data.finance.last_year.expenses.imports, city_data.finance.this_year.expenses.imports);
    draw_row(60, 12, 242, city_data.finance.last_year.expenses.wages, city_data.finance.this_year.expenses.wages);
    draw_row(60, 13, 257, city_data.finance.last_year.expenses.construction, city_data.finance.this_year.expenses.construction);

    // interest (with percentage)
    width = lang_text_draw(60, 14, 80, 272, FONT_NORMAL_BLACK);
    text_draw_percentage(10, 80 + width, 272, FONT_NORMAL_BLACK);
    text_draw_number(city_data.finance.last_year.expenses.interest, '@', " ", 290, 272, FONT_NORMAL_BLACK);
    text_draw_number(city_data.finance.last_year.expenses.interest, '@', " ", 430, 272, FONT_NORMAL_BLACK);

    draw_row(60, 15, 287, city_data.finance.last_year.expenses.salary, city_data.finance.this_year.expenses.salary);
    draw_row(60, 16, 302, city_data.finance.last_year.expenses.sundries, city_data.finance.this_year.expenses.sundries);
    draw_row(60, 21, 317, city_data.finance.last_year.expenses.tribute, city_data.finance.this_year.expenses.tribute);

    graphics_draw_horizontal_line(280, 350, 330, COLOR_BLACK);
    graphics_draw_horizontal_line(420, 490, 330, COLOR_BLACK);

    draw_row(60, 17, 335, city_data.finance.last_year.expenses.total, city_data.finance.this_year.expenses.total);
    draw_row(60, 18, 358, city_data.finance.last_year.net_in_out, city_data.finance.this_year.net_in_out);
    draw_row(60, 19, 381, city_data.finance.last_year.balance, city_data.finance.this_year.balance);

    return ADVISOR_HEIGHT;
}

static void draw_foreground(void)
{
    arrow_buttons_draw(0, 0, arrow_buttons_taxes, 2);
}

static int handle_mouse(const mouse *m)
{
    return arrow_buttons_handle_mouse(m, 0, 0, arrow_buttons_taxes, 2, &arrow_button_focus);
}

static void button_change_taxes(int value, __attribute__((unused)) int param2)
{
    city_data.finance.tax_percentage = calc_bound(city_data.finance.tax_percentage + value, 0, 25);
    city_finance_estimate_taxes();
    city_finance_calculate_totals();
    window_invalidate();
}

static int get_tooltip_text(void)
{
    if (arrow_button_focus) {
        return 120;
    } else {
        return 0;
    }
}

const advisor_window_type *window_advisor_financial(void)
{
    static const advisor_window_type window = {
        draw_background,
        draw_foreground,
        handle_mouse,
        get_tooltip_text
    };
    return &window;
}
