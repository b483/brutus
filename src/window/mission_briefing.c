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
#include "input/input.h"
#include "scenario/criteria.h"
#include "scenario/data.h"
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
    lang_text_draw(62, 10, -60, 55, FONT_NORMAL_WHITE);
    if (scenario.population_win_criteria.enabled) {
        label_draw(-60, 83, 15, 1);
        int width = lang_text_draw(62, 11, -52, 87, FONT_NORMAL_RED);
        text_draw_number(scenario.population_win_criteria.goal, 0, 0, width - 52, 87, FONT_NORMAL_RED);
    }
    if (scenario.culture_win_criteria.enabled) {
        label_draw(196, 51, 15, 1);
        int width = lang_text_draw(62, 12, 204, 55, FONT_NORMAL_RED);
        text_draw_number(scenario.culture_win_criteria.goal, 0, 0, 204 + width, 55, FONT_NORMAL_RED);
    }
    if (scenario.prosperity_win_criteria.enabled) {
        label_draw(196, 83, 15, 1);
        int width = lang_text_draw(62, 13, 204, 87, FONT_NORMAL_RED);
        text_draw_number(scenario.prosperity_win_criteria.goal, 0, 0, 204 + width, 87, FONT_NORMAL_RED);
    }
    if (scenario.peace_win_criteria.enabled) {
        label_draw(452, 51, 15, 1);
        int width = lang_text_draw(62, 14, 460, 55, FONT_NORMAL_RED);
        text_draw_number(scenario.peace_win_criteria.goal, 0, 0, 460 + width, 55, FONT_NORMAL_RED);
    }
    if (scenario.favor_win_criteria.enabled) {
        label_draw(452, 83, 15, 1);
        int width = lang_text_draw(62, 15, 460, 87, FONT_NORMAL_RED);
        text_draw_number(scenario.favor_win_criteria.goal, 0, 0, 460 + width, 87, FONT_NORMAL_RED);
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

static void handle_input(const mouse *m, const hotkeys *h)
{
    rich_text_handle_mouse(mouse_in_dialog(m));
    if (input_go_back_requested(m, h)) {
        rich_text_reset(0);
        sound_speech_stop();
        sound_music_update(1);
        window_city_show();
        return;
    }
}

void window_mission_briefing_show(void)
{
    window_type window = {
        WINDOW_MISSION_BRIEFING,
        draw_background,
        0,
        handle_input,
        0
    };
    rich_text_reset(0);
    window_show(&window);
}

