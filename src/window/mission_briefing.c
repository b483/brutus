#include "mission_briefing.h"

#include "core/image_group.h"
#include "core/lang.h"
#include "game/file.h"
#include "graphics/graphics.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/rich_text.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/criteria.h"
#include "scenario/property.h"
#include "sound/music.h"
#include "sound/speech.h"
#include "window/cck_selection.h"
#include "window/city.h"
#include "window/intermezzo.h"

static void button_back(int param1, int param2);
static void button_start_mission(int param1, int param2);

static const int GOAL_OFFSETS_X[] = { 32, 288, 32, 288, 288, 288 };
static const int GOAL_OFFSETS_Y[] = { 95, 95, 117, 117, 73, 135 };

static image_button image_button_back = {
    0, 0, 31, 20, IB_NORMAL, GROUP_ARROW_MESSAGE_PROBLEMS, 8, button_back, button_none, 0, 0, 1
};
static image_button image_button_start_mission = {
    0, 0, 27, 27, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 56, button_start_mission, button_none, 1, 0, 1
};

static struct {
    int is_review;
    int focus_button;
} data;

static void init(void)
{
    data.focus_button = 0;
    rich_text_reset(0);
}

static void draw_background(void)
{
    window_draw_underlying_window();

    graphics_in_dialog();

    outer_panel_draw(16, 32, 38, 27);

    // Player name
    text_draw(scenario_player_name(), 50, 48, FONT_LARGE_BLACK, 0);

    // Scenario name
    text_draw(scenario_name(), 50, 78, FONT_NORMAL_BLACK, 0);

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

    // To the city / cancel
    lang_text_draw(62, 7, 376, 433, FONT_NORMAL_BLACK);
    if (!data.is_review) {
        lang_text_draw(13, 4, 66, 435, FONT_NORMAL_BLACK);
    }

    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    rich_text_draw_scrollbar();
    image_buttons_draw(516, 426, &image_button_start_mission, 1);
    if (!data.is_review) {
        image_buttons_draw(26, 428, &image_button_back, 1);
    }

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog(m);

    if (image_buttons_handle_mouse(m_dialog, 516, 426, &image_button_start_mission, 1, 0)) {
        return;
    }
    if (!data.is_review) {
        if (image_buttons_handle_mouse(m_dialog, 26, 428, &image_button_back, 1, 0)) {
            return;
        }
    }
    rich_text_handle_mouse(m_dialog);
}

static void button_back(int param1, int param2)
{
    if (!data.is_review) {
        rich_text_reset(0);
        window_cck_selection_show();
        sound_music_play_intro();
    }
}

static void button_start_mission(int param1, int param2)
{
    rich_text_reset(0);
    sound_speech_stop();
    sound_music_update(1);
    window_city_show();
}

static void show(void)
{
    window_type window = {
        WINDOW_MISSION_BRIEFING,
        draw_background,
        draw_foreground,
        handle_input
    };
    init();
    window_show(&window);
}

void window_mission_briefing_show(void)
{
    data.is_review = 0;
    window_intermezzo_show(INTERMEZZO_MISSION_BRIEFING, show);
}

void window_mission_briefing_show_review(void)
{
    data.is_review = 1;
    window_intermezzo_show(INTERMEZZO_MISSION_BRIEFING, show);
}
