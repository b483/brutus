#include "mission_end.h"

#include "city/data_private.h"
#include "city/finance.h"
#include "city/population.h"
#include "city/ratings.h"
#include "city/victory.h"
#include "game/settings.h"
#include "game/state.h"
#include "game/undo.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "sound/music.h"
#include "sound/speech.h"
#include "window/intermezzo.h"
#include "window/main_menu.h"
#include "window/victory_video.h"


static void draw_lost(void)
{
    outer_panel_draw(48, 16, 34, 13);
    lang_text_draw_centered(62, 1, 48, 32, 544, FONT_LARGE_BLACK);
    lang_text_draw_multiline(62, 16, 72, 75, 496, FONT_NORMAL_BLACK);
}

static void draw_won(void)
{
    outer_panel_draw(48, 128, 34, 17);
    lang_text_draw_centered(62, 0, 48, 144, 544, FONT_LARGE_BLACK);

    inner_panel_draw(64, 184, 32, 7);

    lang_text_draw_multiline(147, 20, 80, 192, 488, FONT_NORMAL_WHITE);

    int left_offset = 68;
    int right_offset = 315;
    int width = lang_text_draw(148, 0, left_offset, 308, FONT_NORMAL_BLACK);
    text_draw_number(city_data.ratings.culture, '@', " ", left_offset + width, 308, FONT_NORMAL_BLACK);

    width = lang_text_draw(148, 1, right_offset, 308, FONT_NORMAL_BLACK);
    text_draw_number(city_data.ratings.prosperity, '@', " ", right_offset + width, 308, FONT_NORMAL_BLACK);

    width = lang_text_draw(148, 2, left_offset, 328, FONT_NORMAL_BLACK);
    text_draw_number(city_data.ratings.peace, '@', " ", left_offset + width, 328, FONT_NORMAL_BLACK);

    width = lang_text_draw(148, 3, right_offset, 328, FONT_NORMAL_BLACK);
    text_draw_number(city_data.ratings.favor, '@', " ", right_offset + width, 328, FONT_NORMAL_BLACK);

    width = lang_text_draw(148, 4, left_offset, 348, FONT_NORMAL_BLACK);
    text_draw_number(city_data.population.population, '@', " ", left_offset + width, 348, FONT_NORMAL_BLACK);

    width = lang_text_draw(148, 5, right_offset, 348, FONT_NORMAL_BLACK);
    text_draw_number(city_finance_treasury(), '@', " ", right_offset + width, 348, FONT_NORMAL_BLACK);
}

static void draw_background(void)
{
    graphics_in_dialog();
    if (city_victory_state() == VICTORY_STATE_WON) {
        draw_won();
    } else {
        draw_lost();
    }
    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (m->right.went_up || h->escape_pressed) {
        sound_music_stop();
        sound_speech_stop();
        city_victory_stop_governing();
        game_undo_disable();
        game_state_reset_overlay();
        window_main_menu_show(1);
    }
}

static void show_end_dialog(void)
{
    window_type window = {
        WINDOW_MISSION_END,
        draw_background,
        0,
        handle_input,
        0
    };
    window_show(&window);
}

static void show_intermezzo(void)
{
    window_intermezzo_show(INTERMEZZO_WON, show_end_dialog);
}


void window_mission_end_show_won(void)
{
    mouse_reset_up_state();
    if (setting_victory_video()) {
        window_victory_video_show("smk/victory_balcony.smk", 400, 292, show_intermezzo);
    } else {
        window_victory_video_show("smk/victory_senate.smk", 400, 292, show_intermezzo);
    }
}

void window_mission_end_show_fired(void)
{
    window_intermezzo_show(INTERMEZZO_FIRED, show_end_dialog);
}
