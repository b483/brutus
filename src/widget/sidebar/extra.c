#include "extra.h"

#include "city/city_new.h"
#include "core/config.h"
#include "core/lang.h"
#include "core/string.h"
#include "game/game.h"
#include "graphics/graphics.h"
#include "scenario/scenario.h"


#define EXTRA_INFO_LINE_SPACE 16
#define EXTRA_INFO_HEIGHT_GAME_SPEED 64
#define EXTRA_INFO_HEIGHT_UNEMPLOYMENT 48
#define EXTRA_INFO_HEIGHT_RATINGS 176
#define EXTRA_INFO_VERTICAL_PADDING 8

static void button_game_speed(int is_down, int param2);

static struct arrow_button_t arrow_buttons_speed[] = {
    {11, 30, 17, 24, button_game_speed, 1, 0, 0, 0},
    {35, 30, 15, 24, button_game_speed, 0, 0, 0, 0},
};

struct objective_t {
    int value;
    int target;
};

static struct {
    int x_offset;
    int y_offset;
    int width;
    int height;
    int is_collapsed;
    int info_to_display;
    int game_speed;
    int unemployment_percentage;
    int unemployment_amount;
    struct objective_t culture;
    struct objective_t prosperity;
    struct objective_t peace;
    struct objective_t favor;
    struct objective_t population;
} data;

static int calculate_displayable_info(int info_to_display, int available_height)
{
    if (data.is_collapsed || !config_get(CONFIG_UI_SIDEBAR_INFO) || info_to_display == SIDEBAR_EXTRA_DISPLAY_NONE) {
        return SIDEBAR_EXTRA_DISPLAY_NONE;
    }
    int result = SIDEBAR_EXTRA_DISPLAY_NONE;
    if (available_height >= EXTRA_INFO_HEIGHT_GAME_SPEED) {
        if (info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
            available_height -= EXTRA_INFO_HEIGHT_GAME_SPEED;
            result |= SIDEBAR_EXTRA_DISPLAY_GAME_SPEED;
        }
    } else {
        return result;
    }
    if (available_height >= EXTRA_INFO_HEIGHT_UNEMPLOYMENT) {
        if (info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
            available_height -= EXTRA_INFO_HEIGHT_UNEMPLOYMENT;
            result |= SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT;
        }
    } else {
        return result;
    }
    if (available_height >= EXTRA_INFO_HEIGHT_RATINGS) {
        if (info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
            result |= SIDEBAR_EXTRA_DISPLAY_RATINGS;
        }
    }
    return result;
}

static int calculate_extra_info_height(void)
{
    if (data.info_to_display == SIDEBAR_EXTRA_DISPLAY_NONE) {
        return 0;
    }
    int height = 0;
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        height += EXTRA_INFO_HEIGHT_GAME_SPEED;
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        height += EXTRA_INFO_HEIGHT_UNEMPLOYMENT;
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        height += EXTRA_INFO_HEIGHT_RATINGS;
    }
    return height;
}

static void set_extra_info_objectives(void)
{
    data.culture.target = 0;
    data.prosperity.target = 0;
    data.peace.target = 0;
    data.favor.target = 0;
    data.population.target = 0;

    if (scenario.culture_win_criteria.enabled) {
        data.culture.target = scenario.culture_win_criteria.goal;
    }
    if (scenario.prosperity_win_criteria.enabled) {
        data.prosperity.target = scenario.prosperity_win_criteria.goal;
    }
    if (scenario.peace_win_criteria.enabled) {
        data.peace.target = scenario.peace_win_criteria.goal;
    }
    if (scenario.favor_win_criteria.enabled) {
        data.favor.target = scenario.favor_win_criteria.goal;
    }
    if (scenario.population_win_criteria.enabled) {
        data.population.target = scenario.population_win_criteria.goal;
    }
}

static int update_extra_info_value(int value, int *field)
{
    if (value == *field) {
        return 0;
    } else {
        *field = value;
        return 1;
    }
}

static int update_extra_info(int is_background)
{
    int changed = 0;
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        changed |= update_extra_info_value(setting_game_speed(), &data.game_speed);
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        changed |= update_extra_info_value(city_data.labor.unemployment_percentage, &data.unemployment_percentage);
        changed |= update_extra_info_value(
                       city_data.labor.workers_unemployed - city_data.labor.workers_needed,
                       &data.unemployment_amount
        );
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        if (is_background) {
            set_extra_info_objectives();
        }
        changed |= update_extra_info_value(city_data.ratings.culture, &data.culture.value);
        changed |= update_extra_info_value(city_data.ratings.prosperity, &data.prosperity.value);
        changed |= update_extra_info_value(city_data.ratings.peace, &data.peace.value);
        changed |= update_extra_info_value(city_data.ratings.favor, &data.favor.value);
        changed |= update_extra_info_value(city_data.population.population, &data.population.value);
    }
    return changed;
}

static int draw_extra_info_objective(
    int x_offset, int y_offset, int text_group, int text_id, struct objective_t *obj)
{
    lang_text_draw(text_group, text_id, x_offset + 11, y_offset, FONT_NORMAL_WHITE);
    int font = obj->value >= obj->target ? FONT_NORMAL_GREEN : FONT_NORMAL_RED;
    int width = text_draw_number(obj->value, '@', "", x_offset + 11, y_offset + EXTRA_INFO_LINE_SPACE, font);
    text_draw_number(obj->target, '(', ")", x_offset + 11 + width, y_offset + EXTRA_INFO_LINE_SPACE, font);
    return EXTRA_INFO_LINE_SPACE * 2;
}

static void draw_extra_info_panel(void)
{
    int panel_blocks = data.height / BLOCK_SIZE;
    graphics_draw_vertical_line(data.x_offset, data.y_offset, data.y_offset + data.height, COLOR_WHITE);
    graphics_draw_vertical_line(data.x_offset + data.width - 1, data.y_offset,
        data.y_offset + data.height, COLOR_SIDEBAR);
    inner_panel_draw(data.x_offset + 1, data.y_offset, data.width / BLOCK_SIZE, panel_blocks);

    int y_current_line = data.y_offset + EXTRA_INFO_VERTICAL_PADDING;

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        y_current_line += EXTRA_INFO_VERTICAL_PADDING;

        lang_text_draw(45, 2, data.x_offset + 10, y_current_line, FONT_NORMAL_WHITE);
        y_current_line += EXTRA_INFO_LINE_SPACE + EXTRA_INFO_VERTICAL_PADDING;

        text_draw_percentage(data.game_speed, data.x_offset + 60, y_current_line - 2, FONT_NORMAL_GREEN);

        y_current_line += EXTRA_INFO_VERTICAL_PADDING * 3;
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        y_current_line += EXTRA_INFO_VERTICAL_PADDING;

        lang_text_draw(68, 148, data.x_offset + 10, y_current_line, FONT_NORMAL_WHITE);
        y_current_line += EXTRA_INFO_LINE_SPACE;

        int text_width = text_draw_percentage(data.unemployment_percentage,
            data.x_offset + 10, y_current_line, FONT_NORMAL_GREEN);
        text_draw_number(data.unemployment_amount, '(', ")",
            data.x_offset + 10 + text_width, y_current_line, FONT_NORMAL_GREEN);

        y_current_line += EXTRA_INFO_VERTICAL_PADDING * 3;
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        y_current_line += EXTRA_INFO_VERTICAL_PADDING;

        y_current_line += draw_extra_info_objective(data.x_offset, y_current_line, 53, 1, &data.culture);
        y_current_line += draw_extra_info_objective(data.x_offset, y_current_line, 53, 2, &data.prosperity);
        y_current_line += draw_extra_info_objective(data.x_offset, y_current_line, 53, 3, &data.peace);
        y_current_line += draw_extra_info_objective(data.x_offset, y_current_line, 53, 4, &data.favor);
        draw_extra_info_objective(data.x_offset, y_current_line, 4, 6, &data.population);
    }
}

int sidebar_extra_draw_background(int x_offset, int y_offset, int width, int available_height,
    int is_collapsed, int info_to_display)
{
    data.is_collapsed = is_collapsed;
    data.x_offset = x_offset;
    data.y_offset = y_offset;
    data.width = width;
    data.info_to_display = calculate_displayable_info(info_to_display, available_height);
    data.height = calculate_extra_info_height();

    if (data.info_to_display != SIDEBAR_EXTRA_DISPLAY_NONE) {
        update_extra_info(1);
        draw_extra_info_panel();
    }
    return data.height;
}

static void draw_extra_info_buttons(void)
{
    if (update_extra_info(0)) {
        // Updates displayed speed % after clicking the arrows
        draw_extra_info_panel();
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        arrow_buttons_draw(data.x_offset, data.y_offset, arrow_buttons_speed, 2);
    }
}

void sidebar_extra_draw_foreground(void)
{
    draw_extra_info_buttons();
}

int sidebar_extra_handle_mouse(const struct mouse_t *m)
{
    if (!(data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED)) {
        return 0;
    }
    return arrow_buttons_handle_mouse(m, data.x_offset, data.y_offset, arrow_buttons_speed, 2, 0);
}

static void button_game_speed(int is_down, __attribute__((unused)) int param2)
{
    if (is_down) {
        setting_decrease_game_speed();
    } else {
        setting_increase_game_speed();
    }
}
