#include "advisors.h"

#include "city/city_new.h"
#include "core/image.h"
#include "core/image_group.h"
#include "figure/formation.h"
#include "game/game.h"
#include "graphics/graphics.h"
#include "window/city.h"
#include "window/message_dialog.h"
#include "window/advisor/chief.h"
#include "window/advisor/education.h"
#include "window/advisor/entertainment.h"
#include "window/advisor/financial.h"
#include "window/advisor/health.h"
#include "window/advisor/imperial.h"
#include "window/advisor/labor.h"
#include "window/advisor/military.h"
#include "window/advisor/population.h"
#include "window/advisor/ratings.h"
#include "window/advisor/religion.h"
#include "window/advisor/trade.h"
#include "window/empire.h"

static void button_change_advisor(int advisor, int param2);
static void button_help(int param1, int param2);

static struct image_button_t help_button = {
    11, -7, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help, button_none, 0, 0, 1, 0, 0, 0
};

static struct generic_button_t advisor_buttons[] = {
    {12, 1, 40, 40, button_change_advisor, button_none, ADVISOR_LABOR, 0},
    {60, 1, 40, 40, button_change_advisor, button_none, ADVISOR_MILITARY, 0},
    {108, 1, 40, 40, button_change_advisor, button_none, ADVISOR_IMPERIAL, 0},
    {156, 1, 40, 40, button_change_advisor, button_none, ADVISOR_RATINGS, 0},
    {204, 1, 40, 40, button_change_advisor, button_none, ADVISOR_TRADE, 0},
    {252, 1, 40, 40, button_change_advisor, button_none, ADVISOR_POPULATION, 0},
    {300, 1, 40, 40, button_change_advisor, button_none, ADVISOR_HEALTH, 0},
    {348, 1, 40, 40, button_change_advisor, button_none, ADVISOR_EDUCATION, 0},
    {396, 1, 40, 40, button_change_advisor, button_none, ADVISOR_ENTERTAINMENT, 0},
    {444, 1, 40, 40, button_change_advisor, button_none, ADVISOR_RELIGION, 0},
    {492, 1, 40, 40, button_change_advisor, button_none, ADVISOR_FINANCIAL, 0},
    {540, 1, 40, 40, button_change_advisor, button_none, ADVISOR_CHIEF, 0},
    {588, 1, 40, 40, button_change_advisor, button_none, 0, 0},
};

struct advisor_window_type_t *(*sub_advisors[])(void) = {
    0,
    window_advisor_labor,
    window_advisor_military,
    window_advisor_imperial,
    window_advisor_ratings,
    window_advisor_trade,
    window_advisor_population,
    window_advisor_health,
    window_advisor_education,
    window_advisor_entertainment,
    window_advisor_religion,
    window_advisor_financial,
    window_advisor_chief
};

static const int ADVISOR_TO_MESSAGE_TEXT[] = {
    MESSAGE_DIALOG_ABOUT,
    MESSAGE_DIALOG_ADVISOR_LABOR,
    MESSAGE_DIALOG_ADVISOR_MILITARY,
    MESSAGE_DIALOG_ADVISOR_IMPERIAL,
    MESSAGE_DIALOG_ADVISOR_RATINGS,
    MESSAGE_DIALOG_ADVISOR_TRADE,
    MESSAGE_DIALOG_ADVISOR_POPULATION,
    MESSAGE_DIALOG_ADVISOR_HEALTH,
    MESSAGE_DIALOG_ADVISOR_EDUCATION,
    MESSAGE_DIALOG_ADVISOR_ENTERTAINMENT,
    MESSAGE_DIALOG_ADVISOR_RELIGION,
    MESSAGE_DIALOG_ADVISOR_FINANCIAL,
    MESSAGE_DIALOG_ADVISOR_CHIEF
};

static struct advisor_window_type_t *current_advisor_window = 0;
static int current_advisor = ADVISOR_NONE;

static int focus_button_id;
static int advisor_height;

static void set_advisor_window(void)
{
    if (sub_advisors[current_advisor]) {
        current_advisor_window = sub_advisors[current_advisor]();
    } else {
        current_advisor_window = 0;
    }
}

static void set_advisor(int advisor)
{
    current_advisor = advisor;
    setting_set_last_advisor(advisor);
    set_advisor_window();
}

static void init(void)
{
    city_labor_allocate_workers();

    city_finance_estimate_taxes();
    city_finance_estimate_wages();
    city_data.finance.this_year.expenses.interest = city_data.finance.interest_so_far;
    city_data.finance.this_year.expenses.salary = city_data.finance.salary_so_far;
    city_finance_calculate_totals();

    switch (city_data.sentiment.low_mood_cause) {
        case LOW_MOOD_CAUSE_NO_FOOD:
            city_data.migration.no_immigration_cause = 2;
            break;
        case LOW_MOOD_CAUSE_NO_JOBS:
            city_data.migration.no_immigration_cause = 1;
            break;
        case LOW_MOOD_CAUSE_HIGH_TAXES:
            city_data.migration.no_immigration_cause = 3;
            break;
        case LOW_MOOD_CAUSE_LOW_WAGES:
            city_data.migration.no_immigration_cause = 0;
            break;
        case LOW_MOOD_CAUSE_MANY_TENTS:
            city_data.migration.no_immigration_cause = 4;
            break;
        default:
            city_data.migration.no_immigration_cause = 5;
            break;
    }

    // health
    city_data.houses.health = 0;
    int max = 0;
    if (city_data.houses.missing.bathhouse > max) {
        city_data.houses.health = 1;
        max = city_data.houses.missing.bathhouse;
    }
    if (city_data.houses.missing.barber > max) {
        city_data.houses.health = 2;
        max = city_data.houses.missing.barber;
    }
    if (city_data.houses.missing.clinic > max) {
        city_data.houses.health = 3;
        max = city_data.houses.missing.clinic;
    }
    if (city_data.houses.missing.hospital > max) {
        city_data.houses.health = 4;
    }
    // education
    city_data.houses.education = 0;
    if (city_data.houses.missing.more_education > city_data.houses.missing.education) {
        city_data.houses.education = 1; // schools(academies?)
    } else if (city_data.houses.missing.more_education < city_data.houses.missing.education) {
        city_data.houses.education = 2; // libraries
    } else if (city_data.houses.missing.more_education || city_data.houses.missing.education) {
        city_data.houses.education = 3; // more education
    }
    // entertainment
    city_data.houses.entertainment = 0;
    if (city_data.houses.missing.entertainment > city_data.houses.missing.more_entertainment) {
        city_data.houses.entertainment = 1;
    } else if (city_data.houses.missing.more_entertainment) {
        city_data.houses.entertainment = 2;
    }
    // religion
    city_data.houses.religion = 0;
    max = 0;
    if (city_data.houses.missing.religion > max) {
        city_data.houses.religion = 1;
        max = city_data.houses.missing.religion;
    }
    if (city_data.houses.missing.second_religion > max) {
        city_data.houses.religion = 2;
        max = city_data.houses.missing.second_religion;
    }
    if (city_data.houses.missing.third_religion > max) {
        city_data.houses.religion = 3;
    }
    city_culture_update_coverage();

    city_resource_calculate_food_stocks_and_supply_wheat();

    city_ratings_update_explanations();
}

void window_advisors_draw_dialog_background(void)
{
    image_draw_fullscreen_background(image_group(GROUP_ADVISOR_BACKGROUND));
    graphics_in_dialog();
    image_draw(image_group(GROUP_PANEL_WINDOWS) + 13, 0, 432);

    for (int i = 0; i < 13; i++) {
        int selected_offset = 0;
        if (current_advisor && i == current_advisor - 1) {
            selected_offset = 13;
        }
        image_draw(image_group(GROUP_ADVISOR_ICONS) + i + selected_offset, 48 * i + 12, 441);
    }
    graphics_reset_dialog();
}

static void draw_background(void)
{
    window_advisors_draw_dialog_background();
    graphics_in_dialog();
    advisor_height = current_advisor_window->draw_background();
    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();
    image_buttons_draw(0, BLOCK_SIZE * (advisor_height - 2), &help_button, 1);
    graphics_reset_dialog();

    if (current_advisor_window->draw_foreground) {
        graphics_in_dialog();
        current_advisor_window->draw_foreground();
        graphics_reset_dialog();
    }
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (h->show_last_advisor) {
        window_city_show();
        return;
    }
    if (h->show_empire_map) {
        window_empire_show();
        return;
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 440, advisor_buttons, 13, &focus_button_id)) {
        return;
    }
    int button_id;
    image_buttons_handle_mouse(m_dialog, 0, BLOCK_SIZE * (advisor_height - 2), &help_button, 1, &button_id);
    if (button_id) {
        focus_button_id = -1;
    }
    if (current_advisor_window->handle_mouse && current_advisor_window->handle_mouse(m_dialog)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_city_show();
        return;
    }
}

static void button_change_advisor(int advisor, __attribute__((unused)) int param2)
{
    if (advisor) {
        set_advisor(advisor);
        window_invalidate();
    } else {
        window_city_show();
    }
}

static void button_help(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (current_advisor > 0 && current_advisor < 13) {
        window_message_dialog_show(ADVISOR_TO_MESSAGE_TEXT[current_advisor], 0);
    }
}

int window_advisors_get_advisor(void)
{
    return current_advisor;
}

void window_advisors_show(int advisor)
{
    struct window_type_t window = {
        WINDOW_ADVISORS,
        draw_background,
        draw_foreground,
        handle_input,
    };
    if (advisor) {
        set_advisor(advisor);
    } else {
        set_advisor(ADVISOR_CHIEF);
    }
    init();
    window_show(&window);
}
