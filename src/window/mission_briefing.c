#include "mission_briefing.h"

#include "core/image_group.h"
#include "core/lang.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/rich_text.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/scenario.h"
#include "sound/music.h"
#include "sound/speech.h"
#include "window/cck_selection.h"
#include "window/city.h"
#include "window/intermezzo.h"

static void draw_background(void)
{
    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND) + 16);

    graphics_in_dialog();

    outer_panel_draw(-100, -75, 52, 39);

    // Scenario name
    text_draw(scenario.scenario_name, -84, -59, FONT_LARGE_BLACK, 0);

    // Scenario brief description
    text_draw(scenario.brief_description, -84, -27, FONT_NORMAL_BLACK, 0);

    // Player name
    text_draw(scenario_settings.player_name, -84, 5, FONT_NORMAL_BLACK, 0);

    // Objectives
    inner_panel_draw(-84, 37, 50, 5);
    lang_text_draw(62, 10, -60, 72, FONT_NORMAL_WHITE);
    if (scenario.culture_win_criteria.enabled) {
        label_draw(52, 51, 13, 1);
        int width = lang_text_draw(62, 12, 60, 55, FONT_NORMAL_RED);
        text_draw_number(scenario.culture_win_criteria.goal, 0, 0, width + 60, 55, FONT_NORMAL_RED);
    }
    if (scenario.prosperity_win_criteria.enabled) {
        label_draw(52, 83, 13, 1);
        int width = lang_text_draw(62, 13, 60, 87, FONT_NORMAL_RED);
        text_draw_number(scenario.prosperity_win_criteria.goal, 0, 0, width + 60, 87, FONT_NORMAL_RED);
    }
    if (scenario.peace_win_criteria.enabled) {
        label_draw(268, 51, 13, 1);
        int width = lang_text_draw(62, 14, 276, 55, FONT_NORMAL_RED);
        text_draw_number(scenario.peace_win_criteria.goal, 0, 0, width + 276, 55, FONT_NORMAL_RED);
    }
    if (scenario.favor_win_criteria.enabled) {
        label_draw(268, 83, 13, 1);
        int width = lang_text_draw(62, 15, 276, 87, FONT_NORMAL_RED);
        text_draw_number(scenario.favor_win_criteria.goal, 0, 0, width + 276, 87, FONT_NORMAL_RED);
    }
    if (scenario.population_win_criteria.enabled) {
        label_draw(492, 51, 13, 1);
        int width = lang_text_draw(62, 11, 500, 55, FONT_NORMAL_RED);
        text_draw_number(scenario.population_win_criteria.goal, 0, 0, width + 500, 55, FONT_NORMAL_RED);
    }
    if (scenario.time_limit_win_criteria.enabled) {
        label_draw(492, 83, 13, 2);
        int width = text_draw("Fired after", 500, 87, FONT_NORMAL_RED, COLOR_BLACK);
        text_draw_number(scenario.time_limit_win_criteria.years, 0, " Years", width + 500, 87, FONT_NORMAL_RED);
    } else if (scenario.survival_time_win_criteria.enabled) {
        label_draw(492, 83, 13, 2);
        int width = text_draw("Survive for", 500, 87, FONT_NORMAL_RED, COLOR_BLACK);
        text_draw_number(scenario.survival_time_win_criteria.years, 0, " Years", width + 500, 87, FONT_NORMAL_RED);
    }

    inner_panel_draw(-84, 141, 50, 24);
    // Text body (map description)
    rich_text_set_fonts(FONT_NORMAL_WHITE, FONT_NORMAL_RED, 5);
    rich_text_init(scenario.briefing, -60, 141, 46, 24, 0);
    graphics_set_clip_rectangle(-68, 157, 800, 365);
    rich_text_draw(scenario.briefing, -68, 157, 800, 384, 0);
    graphics_reset_clip_rectangle();
    rich_text_draw_scrollbar();

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    rich_text_handle_mouse(mouse_in_dialog(m));
    if (m->right.went_up || h->escape_pressed || h->enter_pressed) {
        rich_text_reset(0);
        sound_speech_stop();
        sound_music_update(1);
        window_city_show();
        return;
    }
}

void window_mission_briefing_show(void)
{
    struct window_type_t window = {
        WINDOW_MISSION_BRIEFING,
        draw_background,
        0,
        handle_input,
    };
    rich_text_reset(0);
    window_show(&window);
}

