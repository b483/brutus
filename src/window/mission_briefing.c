#include "mission_briefing.h"

#include "core/image_group.h"
#include "core/lang.h"
#include "graphics/graphics.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/rich_text.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "scenario/criteria.h"
#include "scenario/property.h"
#include "sound/music.h"
#include "sound/speech.h"
#include "window/cck_selection.h"
#include "window/city.h"
#include "window/intermezzo.h"

static const int GOAL_OFFSETS_X[] = { 32, 288, 32, 288, 288, 288 };
static const int GOAL_OFFSETS_Y[] = { 95, 95, 117, 117, 73, 135 };

static void init(void)
{
    rich_text_reset(0);
}

static void draw_background(void)
{
    window_draw_underlying_window();

    graphics_in_dialog();

    outer_panel_draw(16, 32, 38, 26);

    // Player name
    text_draw(scenario_settings_player_name(), 50, 48, FONT_LARGE_BLACK, 0);

    // Scenario name
    text_draw(scenario_get_name(), 50, 78, FONT_NORMAL_BLACK, 0);

    // Objectives
    inner_panel_draw(32, 96, 33, 5);
    lang_text_draw(62, 10, 48, 104, FONT_NORMAL_WHITE);
    int goal_index = 0;
    if (scenario_criteria_population_enabled()) {
        int x = GOAL_OFFSETS_X[goal_index];
        int y = GOAL_OFFSETS_Y[goal_index];
        goal_index++;
        label_draw(16 + x, 32 + y, 15, 1);
        int width = lang_text_draw(62, 11, 16 + x + 8, 32 + y + 3, FONT_NORMAL_RED);
        text_draw_number(scenario_criteria_population(), '@', " ", 16 + x + 8 + width, 32 + y + 3, FONT_NORMAL_RED);
    }
    if (scenario_criteria_culture_enabled()) {
        int x = GOAL_OFFSETS_X[goal_index];
        int y = GOAL_OFFSETS_Y[goal_index];
        goal_index++;
        label_draw(16 + x, 32 + y, 15, 1);
        int width = lang_text_draw(62, 12, 16 + x + 8, 32 + y + 3, FONT_NORMAL_RED);
        text_draw_number(scenario_criteria_culture(), '@', " ", 16 + x + 8 + width, 32 + y + 3, FONT_NORMAL_RED);
    }
    if (scenario_criteria_prosperity_enabled()) {
        int x = GOAL_OFFSETS_X[goal_index];
        int y = GOAL_OFFSETS_Y[goal_index];
        goal_index++;
        label_draw(16 + x, 32 + y, 15, 1);
        int width = lang_text_draw(62, 13, 16 + x + 8, 32 + y + 3, FONT_NORMAL_RED);
        text_draw_number(scenario_criteria_prosperity(), '@', " ", 16 + x + 8 + width, 32 + y + 3, FONT_NORMAL_RED);
    }
    if (scenario_criteria_peace_enabled()) {
        int x = GOAL_OFFSETS_X[goal_index];
        int y = GOAL_OFFSETS_Y[goal_index];
        goal_index++;
        label_draw(16 + x, 32 + y, 15, 1);
        int width = lang_text_draw(62, 14, 16 + x + 8, 32 + y + 3, FONT_NORMAL_RED);
        text_draw_number(scenario_criteria_peace(), '@', " ", 16 + x + 8 + width, 32 + y + 3, FONT_NORMAL_RED);
    }
    if (scenario_criteria_favor_enabled()) {
        int x = GOAL_OFFSETS_X[goal_index];
        int y = GOAL_OFFSETS_Y[goal_index];
        goal_index++;
        label_draw(16 + x, 32 + y, 15, 1);
        int width = lang_text_draw(62, 15, 16 + x + 8, 32 + y + 3, FONT_NORMAL_RED);
        text_draw_number(scenario_criteria_favor(), '@', " ", 16 + x + 8 + width, 32 + y + 3, FONT_NORMAL_RED);
    }

    inner_panel_draw(32, 184, 33, 15);
    // Text body (map description)
    rich_text_set_fonts(FONT_NORMAL_WHITE, FONT_NORMAL_RED, 5);
    rich_text_init(scenario_briefing(), 64, 184, 31, 15, 0);
    graphics_set_clip_rectangle(35, 187, 522, 234);
    rich_text_draw(scenario_briefing(), 48, 196, 496, 14, 0);
    graphics_reset_clip_rectangle();

    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    rich_text_draw_scrollbar();

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog(m);
    rich_text_handle_mouse(m_dialog);
    if (input_go_back_requested(m, h)) {
        rich_text_reset(0);
        sound_speech_stop();
        sound_music_update(1);
        window_city_show();
        return;
    }
}

static void show(void)
{
    window_type window = {
        WINDOW_MISSION_BRIEFING,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    init();
    window_show(&window);
}

void window_mission_briefing_show(void)
{
    window_intermezzo_show(INTERMEZZO_MISSION_BRIEFING, show);
}
