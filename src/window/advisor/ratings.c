#include "ratings.h"

#include "city/data_private.h"
#include "city/ratings.h"
#include "graphics/generic_button.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"

#define ADVISOR_HEIGHT 27

static void button_rating(int rating, int param2);

static struct generic_button_t rating_buttons[] = {
    { 80, 286, 110, 66, button_rating, button_none, SELECTED_RATING_CULTURE, 0},
    {200, 286, 110, 66, button_rating, button_none, SELECTED_RATING_PROSPERITY, 0},
    {320, 286, 110, 66, button_rating, button_none, SELECTED_RATING_PEACE, 0},
    {440, 286, 110, 66, button_rating, button_none, SELECTED_RATING_FAVOR, 0},
};

static int focus_button_id;

static void draw_rating_column(int x_offset, int y_offset, int value, int has_reached)
{
    int image_base = image_group(GROUP_RATINGS_COLUMN);
    int y = y_offset - image_get(image_base)->height;
    image_draw(image_base, x_offset, y);
    for (int i = 0; i < 2 * value; i++) {
        image_draw(image_base + 1, x_offset + 11, --y);
    }
    if (value >= 30 && has_reached) {
        image_draw(image_base + 2, x_offset - 6, y);
    }
}

static int draw_background(void)
{
    outer_panel_draw(0, 0, 40, ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 3, 10, 10);
    int width = lang_text_draw(53, 0, 60, 12, FONT_LARGE_BLACK);
    if (scenario.population_win_criteria.enabled) {
        width += lang_text_draw(53, 6, 80 + width, 17, FONT_NORMAL_BLACK);
        text_draw_number(scenario.population_win_criteria.goal, '@', ")", 80 + width, 17, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw(53, 7, 80 + width, 17, FONT_NORMAL_BLACK);
    }

    image_draw(image_group(GROUP_RATINGS_BACKGROUND), 60, 48);

    // culture
    button_border_draw(80, 286, 110, 66, focus_button_id == SELECTED_RATING_CULTURE);
    lang_text_draw_centered(53, 1, 80, 294, 110, FONT_NORMAL_BLACK);
    text_draw_number_centered(city_data.ratings.culture, 80, 309, 100, FONT_LARGE_BLACK);
    width = text_draw_number(scenario.culture_win_criteria.enabled ? scenario.culture_win_criteria.goal : 0,
            '@', " ", 85, 334, FONT_NORMAL_BLACK);
    lang_text_draw(53, 5, 85 + width, 334, FONT_NORMAL_BLACK);
    int has_reached = !scenario.culture_win_criteria.enabled || city_data.ratings.culture >= scenario.culture_win_criteria.goal;
    draw_rating_column(110, 274, city_data.ratings.culture, has_reached);

    // prosperity
    button_border_draw(200, 286, 110, 66, focus_button_id == SELECTED_RATING_PROSPERITY);
    lang_text_draw_centered(53, 2, 200, 294, 110, FONT_NORMAL_BLACK);
    text_draw_number_centered(city_data.ratings.prosperity, 200, 309, 100, FONT_LARGE_BLACK);
    width = text_draw_number(scenario.prosperity_win_criteria.enabled ? scenario.prosperity_win_criteria.goal : 0,
            '@', " ", 205, 334, FONT_NORMAL_BLACK);
    lang_text_draw(53, 5, 205 + width, 334, FONT_NORMAL_BLACK);
    has_reached = !scenario.prosperity_win_criteria.enabled || city_data.ratings.prosperity >= scenario.prosperity_win_criteria.goal;
    draw_rating_column(230, 274, city_data.ratings.prosperity, has_reached);

    // peace
    button_border_draw(320, 286, 110, 66, focus_button_id == SELECTED_RATING_PEACE);
    lang_text_draw_centered(53, 3, 320, 294, 110, FONT_NORMAL_BLACK);
    text_draw_number_centered(city_data.ratings.peace, 320, 309, 100, FONT_LARGE_BLACK);
    width = text_draw_number(scenario.peace_win_criteria.enabled ? scenario.peace_win_criteria.goal : 0,
            '@', " ", 325, 334, FONT_NORMAL_BLACK);
    lang_text_draw(53, 5, 325 + width, 334, FONT_NORMAL_BLACK);
    has_reached = !scenario.peace_win_criteria.enabled || city_data.ratings.peace >= scenario.peace_win_criteria.goal;
    draw_rating_column(350, 274, city_data.ratings.peace, has_reached);

    // favor
    button_border_draw(440, 286, 110, 66, focus_button_id == SELECTED_RATING_FAVOR);
    lang_text_draw_centered(53, 4, 440, 294, 110, FONT_NORMAL_BLACK);
    text_draw_number_centered(city_data.ratings.favor, 440, 309, 100, FONT_LARGE_BLACK);
    width = text_draw_number(scenario.favor_win_criteria.enabled ? scenario.favor_win_criteria.goal : 0,
            '@', " ", 445, 334, FONT_NORMAL_BLACK);
    lang_text_draw(53, 5, 445 + width, 334, FONT_NORMAL_BLACK);
    has_reached = !scenario.favor_win_criteria.enabled || city_data.ratings.favor >= scenario.favor_win_criteria.goal;
    draw_rating_column(470, 274, city_data.ratings.favor, has_reached);

    // bottom info box
    inner_panel_draw(64, 356, 32, 4);
    switch (city_data.ratings.selected) {
        case SELECTED_RATING_CULTURE:
            lang_text_draw(53, 1, 72, 359, FONT_NORMAL_WHITE);
            if (city_data.ratings.culture <= 90) {
                lang_text_draw_multiline(53, 9 + city_rating_selected_explanation(),
                    72, 374, 496, FONT_NORMAL_WHITE);
            } else {
                lang_text_draw_multiline(53, 50, 72, 374, 496, FONT_NORMAL_WHITE);
            }
            break;
        case SELECTED_RATING_PROSPERITY:
            lang_text_draw(53, 2, 72, 359, FONT_NORMAL_WHITE);
            if (city_data.ratings.prosperity <= 90) {
                lang_text_draw_multiline(53, 16 + city_rating_selected_explanation(),
                    72, 374, 496, FONT_NORMAL_WHITE);
            } else {
                lang_text_draw_multiline(53, 51, 72, 374, 496, FONT_NORMAL_WHITE);
            }
            break;
        case SELECTED_RATING_PEACE:
            lang_text_draw(53, 3, 72, 359, FONT_NORMAL_WHITE);
            if (city_data.ratings.peace <= 90) {
                lang_text_draw_multiline(53, 41 + city_rating_selected_explanation(),
                    72, 374, 496, FONT_NORMAL_WHITE);
            } else {
                lang_text_draw_multiline(53, 52, 72, 374, 496, FONT_NORMAL_WHITE);
            }
            break;
        case SELECTED_RATING_FAVOR:
            lang_text_draw(53, 4, 72, 359, FONT_NORMAL_WHITE);
            if (city_data.ratings.favor <= 90) {
                lang_text_draw_multiline(53, 27 + city_rating_selected_explanation(),
                    72, 374, 496, FONT_NORMAL_WHITE);
            } else {
                lang_text_draw_multiline(53, 53, 72, 374, 496, FONT_NORMAL_WHITE);
            }
            break;
        default:
            lang_text_draw_centered(53, 8, 72, 380, 496, FONT_NORMAL_WHITE);
            break;
    }

    return ADVISOR_HEIGHT;
}

static void draw_foreground(void)
{
    button_border_draw(80, 286, 110, 66, focus_button_id == SELECTED_RATING_CULTURE);
    button_border_draw(200, 286, 110, 66, focus_button_id == SELECTED_RATING_PROSPERITY);
    button_border_draw(320, 286, 110, 66, focus_button_id == SELECTED_RATING_PEACE);
    button_border_draw(440, 286, 110, 66, focus_button_id == SELECTED_RATING_FAVOR);
}

static int handle_mouse(const struct mouse_t *m)
{
    return generic_buttons_handle_mouse(m, 0, 0, rating_buttons, 4, &focus_button_id);
}

static void button_rating(int rating, __attribute__((unused)) int param2)
{
    city_data.ratings.selected = rating;
    window_invalidate();
}

struct advisor_window_type_t *window_advisor_ratings(void)
{
    static struct advisor_window_type_t window = {
        draw_background,
        draw_foreground,
        handle_mouse,
    };
    return &window;
}
