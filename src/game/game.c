#include "game.h"

#include "city/view.h"
#include "core/config.h"
#include "core/hotkey_config.h"
#include "core/image.h"
#include "core/lang.h"
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
#include "platform/brutus.h"
#include "scenario/scenario.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "window/editor/map.h"
#include "window/logo.h"
#include "window/main_menu.h"

#include "SDL_mixer.h"

static char sound_channel_filenames[SOUND_CHANNEL_MAX][CHANNEL_FILENAME_MAX] = {
    "", // speech channel
    "wavs/panel1.wav",
    "wavs/panel3.wav",
    "wavs/icon1.wav",
    "wavs/build1.wav",
    "wavs/explod1.wav",
    "wavs/fanfare.wav",
    "wavs/fanfare2.wav",
    "wavs/arrow.wav",
    "wavs/arrow_hit.wav",
    "wavs/axe.wav",
    "wavs/ballista.wav",
    "wavs/ballista_hit_ground.wav",
    "wavs/ballista_hit_person.wav",
    "wavs/club.wav",
    "wavs/camel1.wav",
    "wavs/elephant.wav",
    "wavs/elephant_hit.wav",
    "wavs/elephant_die.wav",
    "wavs/horse.wav",
    "wavs/horse2.wav",
    "wavs/horse_mov.wav",
    "wavs/javelin.wav",
    "wavs/lion_attack.wav",
    "wavs/lion_die.wav",
    "wavs/horn3.wav",
    "wavs/sword.wav",
    "wavs/sword_swing.wav",
    "wavs/sword_light.wav",
    "wavs/spear_attack.wav",
    "wavs/wolf_attack.wav",
    "wavs/wolf_attack2.wav",
    "wavs/wolf_die.wav",
    "wavs/die1.wav",
    "wavs/die2.wav",
    "wavs/die4.wav",
    "wavs/die10.wav",
    "wavs/die3.wav",
    "wavs/die5.wav",
    "wavs/die8.wav",
    "wavs/die9.wav",
    "wavs/sheep_die.wav",
    "wavs/zebra_die.wav",
    "wavs/wolf_howl.wav",
    "wavs/fire_splash.wav",
    "wavs/formation_shield.wav",
    // city sounds
    "wavs/house_slum1.wav",
    "wavs/house_slum2.wav",
    "wavs/house_slum3.wav",
    "wavs/house_slum4.wav",
    "wavs/house_poor1.wav",
    "wavs/house_poor2.wav",
    "wavs/house_poor3.wav",
    "wavs/house_poor4.wav",
    "wavs/house_mid1.wav",
    "wavs/house_mid2.wav",
    "wavs/house_mid3.wav",
    "wavs/house_mid4.wav",
    "wavs/house_good1.wav",
    "wavs/house_good2.wav",
    "wavs/house_good3.wav",
    "wavs/house_good4.wav",
    "wavs/house_posh1.wav",
    "wavs/house_posh2.wav",
    "wavs/house_posh3.wav",
    "wavs/house_posh4.wav",
    "wavs/empty_land1.wav",
    "wavs/resevoir.wav",
    "wavs/aquaduct.wav", // same as river.wav
    "wavs/fountain.wav",
    "wavs/well.wav",
    "wavs/barber.wav",
    "wavs/baths.wav",
    "wavs/clinic.wav",
    "wavs/hospital.wav",
    "wavs/temp_farm.wav",
    "wavs/temp_ship.wav",
    "wavs/temple_ship.wav",
    "wavs/temp_comm.wav",
    "wavs/temp_war.wav",
    "wavs/temple_war.wav",
    "wavs/temp_love.wav",
    "wavs/oracle.wav",
    "wavs/school.wav",
    "wavs/academy.wav",
    "wavs/library.wav",
    "wavs/theatre.wav",
    "wavs/ampitheatre.wav",
    "wavs/colloseum.wav",
    "wavs/hippodrome.wav",
    "wavs/glad_pit.wav",
    "wavs/lion_pit.wav",
    "wavs/art_pit.wav",
    "wavs/char_pit.wav",
    "wavs/forum.wav",
    "wavs/senate.wav",
    "wavs/palace.wav",
    "wavs/statue.wav",
    "wavs/gardens1.wav", // same as emptyland1.wav
    "wavs/gardens2.wav", // same as emptyland2.wav
    "wavs/gardens3.wav", // same as emptyland3.wav
    "wavs/gardens4.wav", // same as emptyland4.wav
    "wavs/shipyard.wav",
    "wavs/shipyard1.wav",
    "wavs/shipyard2.wav",
    "wavs/dock.wav",
    "wavs/dock1.wav",
    "wavs/dock2.wav",
    "wavs/wharf.wav",
    "wavs/wharf1.wav",
    "wavs/wharf2.wav",
    "wavs/tower1.wav",
    "wavs/tower2.wav",
    "wavs/tower3.wav",
    "wavs/tower4.wav",
    "wavs/fort1.wav",
    "wavs/fort2.wav",
    "wavs/fort3.wav",
    "wavs/fort4.wav",
    "wavs/mil_acad.wav",
    "wavs/barracks.wav",
    "wavs/wheat.wav",
    "wavs/wheat_farm.wav",
    "wavs/veg_farm.wav",
    "wavs/figs_farm.wav",
    "wavs/olives_farm.wav",
    "wavs/vines_farm.wav",
    "wavs/meat_farm.wav",
    "wavs/clay_pit.wav",
    "wavs/quarry.wav",
    "wavs/mine.wav",
    "wavs/lumber_mill.wav",
    "wavs/wine_workshop.wav",
    "wavs/oil_workshop.wav",
    "wavs/weap_workshop.wav",
    "wavs/weapons_workshop.wav",
    "wavs/furn_workshop.wav",
    "wavs/furniture_workshop.wav",
    "wavs/pott_workshop.wav",
    "wavs/pottery_workshop.wav",
    "wavs/market1.wav",
    "wavs/market2.wav",
    "wavs/market3.wav",
    "wavs/market4.wav",
    "wavs/granary.wav",
    "wavs/granary1.wav",
    "wavs/granary2.wav",
    "wavs/warehouse.wav",
    "wavs/warehouse1.wav",
    "wavs/warehouse2.wav",
    "wavs/burning_ruin.wav",
};

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

    // initialize sound
    open_sound_device();
    init_sound_device_channels(SOUND_CHANNEL_MAX, sound_channel_filenames);
    set_city_sounds_volume(get_sound(SOUND_CITY)->volume);
    set_sound_effect_volume(get_sound(SOUND_EFFECTS)->volume);
    Mix_VolumeMusic(percentage_to_volume(get_sound(SOUND_MUSIC)->volume));
    set_channel_volume(SOUND_CHANNEL_SPEECH, get_sound(SOUND_SPEECH)->volume);

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
    play_city_sounds();
}

void game_exit(void)
{
    video_shutdown();
    settings_save();
    config_save();
    close_sound_device();
}
