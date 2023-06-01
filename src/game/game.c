#include "game.h"

#include "city/view.h"
#include "core/config.h"
#include "core/hotkey_config.h"
#include "core/image.h"
#include "core/lang.h"
#include "core/log.h"
#include "core/random.h"
#include "editor/editor.h"
#include "game/animation.h"
#include "game/file_editor.h"
#include "game/settings.h"
#include "game/speed.h"
#include "game/state.h"
#include "game/tick.h"
#include "graphics/font.h"
#include "graphics/video.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "scenario/scenario.h"
#include "sound/city.h"
#include "sound/system.h"
#include "window/editor/map.h"
#include "window/logo.h"
#include "window/main_menu.h"

int game_pre_init(void)
{
    settings_load();
    config_load();
    hotkey_config_load();
    game_state_unpause();

    if (!lang_load(0)) {
        log_error("'c3.eng' or 'c3_mm.eng' files not found or too large.", 0, 0);
        return 0;
    }
    font_set_encoding();
    random_init();
    return 1;
}

static int is_unpatched(void)
{
    const char *difficulty_option = lang_get_string(2, 6);
    const char *help_menu = lang_get_string(3, 0);
    // Without patch, the difficulty option string does not exist and
    // getting it "falls through" to the next text group
    return difficulty_option == help_menu;
}

int game_init(void)
{
    if (!image_init()) {
        log_error("unable to init graphics", 0, 0);
        return 0;
    }
    if (!image_load_climate(CLIMATE_NORTHERN, 0, 1)) {
        log_error("unable to load main graphics", 0, 0);
        return 0;
    }
    if (!image_load_enemy()) {
        log_error("unable to load enemy graphics", 0, 0);
        return 0;
    }

    sound_system_init();
    game_state_init();
    window_logo_show((is_unpatched() ? MESSAGE_MISSING_PATCH : MESSAGE_NONE));

    return 1;
}

static int reload_language(int is_editor, int reload_images)
{
    if (!lang_load(is_editor)) {
        if (is_editor) {
            log_error("'c3_map.eng' or 'c3_map_mm.eng' files not found or too large.", 0, 0);
        } else {
            log_error("'c3.eng' or 'c3_mm.eng' files not found or too large.", 0, 0);
        }
        return 0;
    }

    if (!image_load_climate(scenario.climate, is_editor, reload_images)) {
        log_error("unable to load main graphics", 0, 0);
        return 0;
    }
    return 1;
}

int game_init_editor(void)
{
    if (!reload_language(1, 0)) {
        return 0;
    }

    game_file_editor_clear_data();
    game_file_editor_create_scenario(2);

    if (city_view_is_sidebar_collapsed()) {
        city_view_toggle_sidebar();
    }

    editor_set_active(1);
    window_editor_map_show();
    return 1;
}

void game_exit_editor(void)
{
    if (!reload_language(0, 0)) {
        return;
    }
    editor_set_active(0);
    window_main_menu_show(1);
}

void game_run(void)
{
    game_animation_update();
    int num_ticks = game_speed_get_elapsed_ticks();
    for (int i = 0; i < num_ticks; i++) {
        game_tick_run();

        if (window_is_invalid()) {
            break;
        }
    }
}

void game_draw(void)
{
    window_draw(0);
    sound_city_play();
}

void game_exit(void)
{
    video_shutdown();
    settings_save();
    config_save();
    sound_system_shutdown();
}
