#include "set_salary.h"

#include "city/data.h"
#include "city/ratings.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "window/advisors.h"

static void button_set_salary(int rank, int param2);

static struct generic_button_t buttons_set_salary[] = {
    {196, 96, 250, 15, button_set_salary, button_none, 0, 0},
    {196, 116, 250, 15, button_set_salary, button_none, 1, 0},
    {196, 136, 250, 15, button_set_salary, button_none, 2, 0},
    {196, 156, 250, 15, button_set_salary, button_none, 3, 0},
    {196, 176, 250, 15, button_set_salary, button_none, 4, 0},
    {196, 196, 250, 15, button_set_salary, button_none, 5, 0},
    {196, 216, 250, 15, button_set_salary, button_none, 6, 0},
    {196, 236, 250, 15, button_set_salary, button_none, 7, 0},
    {196, 256, 250, 15, button_set_salary, button_none, 8, 0},
    {196, 276, 250, 15, button_set_salary, button_none, 9, 0},
    {196, 296, 250, 15, button_set_salary, button_none, 10, 0},
};

static int focus_button_id;

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(164, 32, 20, 23);

    // Coin image
    image_draw(COIN_IMAGE_ID, 180, 48);

    // Set salary level
    lang_text_draw_centered(52, 15, 164, 48, 320, FONT_LARGE_BLACK);

    inner_panel_draw(180, 80, 18, 15);

    for (int rank = 0; rank < 11; rank++) {
        int font = focus_button_id == rank + 1 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        lang_text_draw(52, rank + 4, 196, 96 + 20 * rank, font);
        text_draw_money(city_emperor_salary_for_rank(rank), 385, 96 + 20 * rank, font);
    }

    if (!city_data.mission.has_won) {
        if (city_data.emperor.salary_rank <= city_data.emperor.player_rank) {
            lang_text_draw_multiline(52, 76, 185, 336, 288, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_multiline(52, 71, 185, 336, 288, FONT_NORMAL_BLACK);
        }
    } else {
        graphics_shade_rect(180, 80, 288, 240, 0);
        lang_text_draw_multiline(52, 77, 185, 336, 288, FONT_NORMAL_BLACK);
    }

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }

    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_set_salary, sizeof(buttons_set_salary) / sizeof(struct generic_button_t), &focus_button_id)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 164 || m_dialog->y < 32 || m_dialog->x > 484 || m_dialog->y > 400)) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
}

static void button_set_salary(int rank, __attribute__((unused)) int param2)
{
    if (!city_data.mission.has_won) {
        city_emperor_set_salary_rank(rank);
        city_data.finance.this_year.expenses.salary = city_data.finance.salary_so_far;
        city_ratings_update_favor_explanation();
        window_advisors_show(ADVISOR_IMPERIAL);
    }
}

void window_set_salary_show(void)
{
    struct window_type_t window = {
        WINDOW_SET_SALARY,
        window_advisors_draw_dialog_background,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}
