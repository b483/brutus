#include "game.h"

#include "city/city_new.h"
#include "core/calc.h"
#include "core/config.h"
#include "core/file.h"
#include "core/hotkey_config.h"
#include "core/image.h"
#include "core/lang.h"
#include "core/string.h"
#include "core/random.h"
#include "core/time.h"
#include "editor/editor.h"
#include "empire/empire.h"
#include "figure/combat.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/formation_enemy.h"
#include "figure/formation_herd.h"
#include "figure/formation_legion.h"
#include "figure/movement.h"
#include "figure/name.h"
#include "figure/route.h"
#include "figure/sound.h"
#include "figure/trader.h"
#include "figuretype/figuretype.h"
#include "platform/brutus.h"
#include "scenario/scenario.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "window/window.h"

#include "SDL_mixer.h"

#define MAX_HOUSE_LEVELS 20
#define MAX_ANIM_TIMERS 51
#define COMPRESS_BUFFER_SIZE 600000
#define UNCOMPRESSED 0x80000000
#define INF_SIZE 62
#define MAX_TICKS_PER_FRAME 20
#define MAX_DIR 4
#define MAX_UNDO_BUILDINGS 50
#define PREFECT_LEASH_RANGE 20
#define MAP_FLAG_IMG_ID 2916

static const int FLOTSAM_TYPE_0[] = { 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0 };
static const int FLOTSAM_TYPE_12[] = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 3, 2, 1, 0, 0, 1, 1, 2, 2, 1, 1, 0, 0, 0 };
static const int FLOTSAM_TYPE_3[] = { 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

static const int BALLISTA_FIRING_OFFSETS[] = {
    0, 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const int CLOUD_IMAGE_OFFSETS[] = {
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2,
    2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 7
};

static const int CRIMINAL_OFFSETS[] = {
    0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1
};

static const int CART_OFFSET_MULTIPLE_LOADS_FOOD[] = { 0, 0, 8, 16, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const int CART_OFFSET_MULTIPLE_LOADS_NON_FOOD[] = { 0, 0, 0, 0, 0, 8, 0, 16, 24, 32, 40, 48, 56, 64, 72, 80 };
static const int CART_OFFSET_8_LOADS_FOOD[] = { 0, 40, 48, 56, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static const int SHEEP_IMAGE_OFFSETS[] = {
    0,  0,  1,  1,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  4,  4,  5,  5, -1, -1, -1, -1, -1, -1, -1, -1
};

static const struct map_point_t SEAGULL_OFFSETS[] = {
    {0, 0}, {0, -2}, {-2, 0}, {1, 2}, {2, 0}, {-3, 1}, {4, -3}, {-2, 4}, {0, 0}
};

static struct {
    uint32_t last_update;
    int should_update;
} timers[MAX_ANIM_TIMERS];

struct file_piece_t {
    struct buffer_t buf;
};

struct scenario_state_t {
    struct buffer_t *graphic_ids;
    struct buffer_t *edge;
    struct buffer_t *terrain;
    struct buffer_t *bitfields;
    struct buffer_t *random;
    struct buffer_t *random_iv;
    struct buffer_t *camera;
    struct buffer_t *scenario;
    struct buffer_t *empire_objects;
};

static struct {
    int num_pieces;
    struct file_piece_t pieces[11];
    struct scenario_state_t state;
} scenario_data = { 0 };

struct savegame_state_t {
    struct buffer_t *image_grid;
    struct buffer_t *edge_grid;
    struct buffer_t *building_grid;
    struct buffer_t *terrain_grid;
    struct buffer_t *aqueduct_grid;
    struct buffer_t *figure_grid;
    struct buffer_t *bitfields_grid;
    struct buffer_t *sprite_grid;
    struct buffer_t *random_grid;
    struct buffer_t *desirability_grid;
    struct buffer_t *building_damage_grid;
    struct buffer_t *aqueduct_backup_grid;
    struct buffer_t *sprite_backup_grid;
    struct buffer_t *figures;
    struct buffer_t *route_figures;
    struct buffer_t *route_paths;
    struct buffer_t *legion_formations;
    struct buffer_t *herd_formations;
    struct buffer_t *enemy_formations;
    struct buffer_t *city_data;
    struct buffer_t *player_name;
    struct buffer_t *buildings;
    struct buffer_t *city_view_orientation;
    struct buffer_t *game_time;
    struct buffer_t *building_extra_highest_id_ever;
    struct buffer_t *random_iv;
    struct buffer_t *city_view_camera;
    struct buffer_t *building_count_culture1;
    struct buffer_t *city_graph_order;
    struct buffer_t *empire;
    struct buffer_t *empire_objects;
    struct buffer_t *building_count_industry;
    struct buffer_t *trade_prices;
    struct buffer_t *figure_names;
    struct buffer_t *culture_coverage;
    struct buffer_t *scenario;
    struct buffer_t *messages;
    struct buffer_t *message_extra;
    struct buffer_t *population_messages;
    struct buffer_t *message_counts;
    struct buffer_t *message_delays;
    struct buffer_t *building_list_burning_totals;
    struct buffer_t *city_sounds;
    struct buffer_t *building_extra_highest_id;
    struct buffer_t *figure_traders;
    struct buffer_t *building_list_burning;
    struct buffer_t *building_list_small;
    struct buffer_t *building_list_large;
    struct buffer_t *building_count_military;
    struct buffer_t *building_storages;
    struct buffer_t *building_count_culture2;
    struct buffer_t *building_count_support;
    struct buffer_t *building_barracks_tower_sentry;
    struct buffer_t *building_extra_sequence;
    struct buffer_t *routing_counters;
    struct buffer_t *building_count_culture3;
    struct buffer_t *building_extra_corrupt_houses;
    struct buffer_t *bookmarks;
};

static struct {
    int num_pieces;
    struct file_piece_t pieces[100];
    struct savegame_state_t state;
} savegame_data = { 0 };

static struct {
    // display settings
    int fullscreen;
    int window_width;
    int window_height;
    // sound settings
    struct set_sound_t sound_effects;
    struct set_sound_t sound_music;
    struct set_sound_t sound_speech;
    struct set_sound_t sound_city;
    // speed settings
    int game_speed;
    int scroll_speed;
    // misc settings
    int monthly_autosave;
    int warnings;
    int victory_video;
    // persistent game state
    int last_advisor;
    // file data
    uint8_t inf_file[INF_SIZE];
} settings_data;

static const uint32_t MILLIS_PER_TICK_PER_SPEED[] = {
    702, 502, 352, 242, 162, 112, 82, 57, 37, 22, 16
};
static const uint32_t MILLIS_PER_HYPER_SPEED[] = {
    702, 16, 8, 5, 3, 2
};

static struct {
    int last_check_was_valid;
    uint32_t last_update;
} speed_data;

static struct {
    int paused;
    int current_overlay;
    int previous_overlay;
} state_data = { 0, OVERLAY_NONE, OVERLAY_NONE };

enum {
    EVOLVE = 1,
    NONE = 0,
    DEVOLVE = -1
};

static int fire_spread_direction = 0;

static const struct {
    int x;
    int y;
    int offset;
} EXPAND_DIRECTION_DELTA[MAX_DIR] = { {0, 0, 0}, {-1, -1, -GRID_SIZE - 1}, {-1, 0, -1}, {0, -1, -GRID_SIZE} };

static struct {
    int x;
    int y;
    int inventory[INVENTORY_MAX];
    int population;
} merge_data;

static const int HOUSE_TILE_OFFSETS[] = {
    OFFSET(0,0), OFFSET(1,0), OFFSET(0,1), OFFSET(1,1), // 2x2
    OFFSET(2,0), OFFSET(2,1), OFFSET(2,2), OFFSET(1,2), OFFSET(0,2), // 3x3
    OFFSET(3,0), OFFSET(3,1), OFFSET(3,2), OFFSET(3,3), OFFSET(2,3), OFFSET(1,3), OFFSET(0,3) // 4x4
};

static const struct {
    int group;
    int offset;
    int num_types;
} HOUSE_IMAGE[20] = {
    {GROUP_BUILDING_HOUSE_TENT, 0, 2}, {GROUP_BUILDING_HOUSE_TENT, 2, 2},
    {GROUP_BUILDING_HOUSE_SHACK, 0, 2}, {GROUP_BUILDING_HOUSE_SHACK, 2, 2},
    {GROUP_BUILDING_HOUSE_HOVEL, 0, 2}, {GROUP_BUILDING_HOUSE_HOVEL, 2, 2},
    {GROUP_BUILDING_HOUSE_CASA, 0, 2}, {GROUP_BUILDING_HOUSE_CASA, 2, 2},
    {GROUP_BUILDING_HOUSE_INSULA_1, 0, 2}, {GROUP_BUILDING_HOUSE_INSULA_1, 2, 2},
    {GROUP_BUILDING_HOUSE_INSULA_2, 0, 2}, {GROUP_BUILDING_HOUSE_INSULA_2, 2, 2},
    {GROUP_BUILDING_HOUSE_VILLA_1, 0, 2}, {GROUP_BUILDING_HOUSE_VILLA_1, 2, 2},
    {GROUP_BUILDING_HOUSE_VILLA_2, 0, 1}, {GROUP_BUILDING_HOUSE_VILLA_2, 1, 1},
    {GROUP_BUILDING_HOUSE_PALACE_1, 0, 1}, {GROUP_BUILDING_HOUSE_PALACE_1, 1, 1},
    {GROUP_BUILDING_HOUSE_PALACE_2, 0, 1}, {GROUP_BUILDING_HOUSE_PALACE_2, 1, 1},
};

static struct {
    int tick; // 50 ticks in a day
    int day; // 16 days in a month
    int month; // 12 months in a year
    int year;
    int total_days;
} time_data;

static struct {
    int available;
    int ready;
    int timeout_ticks;
    int building_cost;
    int num_buildings;
    int type;
    struct building_t buildings[MAX_UNDO_BUILDINGS];
} undo_data;

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

static void game_state_init(void)
{
    city_victory_reset();
    city_view_set_camera(76, 152);
    random_generate_pool();
    city_warning_clear_all();
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

    // Without patch, the difficulty option string does not exist and
    // getting it "falls through" to the next text group
    const char *difficulty_option = lang_get_string(2, 6);
    const char *help_menu = lang_get_string(3, 0);
    if (difficulty_option == help_menu) {
        window_logo_show(MESSAGE_MISSING_PATCH);
    } else {
        window_logo_show(MESSAGE_NONE);
    }

    return 1;
}

int reload_language(int is_editor, int reload_images)
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

static void game_time_init(int year)
{
    time_data.tick = 0;
    time_data.day = 0;
    time_data.month = 0;
    time_data.total_days = 0;
    time_data.year = year;
}

static void game_animation_init(void)
{
    for (int i = 0; i < MAX_ANIM_TIMERS; i++) {
        timers[i].last_update = 0;
        timers[i].should_update = 0;
    }
}

int game_init_editor(void)
{
    if (!reload_language(1, 0)) {
        return 0;
    }
    city_victory_reset();
    building_construction_clear_type();
    city_data_init();
    city_data_init_scenario();
    city_message_init_scenario();
    game_state_init();
    game_animation_init();
    initialize_city_sounds();
    building_clear_all();
    building_storage_clear_all();
    figure_init_scenario();
    figure_name_init();
    reset_all_formations();
    figure_route_clear_all();
    traders_clear();
    game_time_init(2098);
    game_file_editor_create_scenario(2);
    if (city_view_is_sidebar_collapsed()) {
        city_view_toggle_sidebar();
    }
    editor_set_active(1);
    show_editor_map();
    return 1;
}

static void clear_buildings(void)
{
    undo_data.num_buildings = 0;
    memset(undo_data.buildings, 0, MAX_UNDO_BUILDINGS * sizeof(struct building_t));
}

static void decay(unsigned char *value)
{
    if (*value > 0) {
        *value = *value - 1;
    } else {
        *value = 0;
    }
}

static int check_evolve_desirability(struct building_t *house)
{
    int level = house->subtype.house_level;
    int evolve_des = house_properties[level].evolve_desirability;
    if (level >= HOUSE_LUXURY_PALACE) {
        evolve_des = 1000;
    }
    int current_des = house->desirability;
    int status;
    if (current_des <= house_properties[level].devolve_desirability) {
        status = DEVOLVE;
    } else if (current_des >= evolve_des) {
        status = EVOLVE;
    } else {
        status = NONE;
    }
    house->data.house.evolve_text_id = status; // BUG? -1 in an unsigned char?
    return status;
}

static int has_required_goods_and_services(struct building_t *house, int for_upgrade, struct house_demands_t *demands)
{
    int level = house->subtype.house_level;
    if (for_upgrade) {
        ++level;
    }
    // water
    if (!house->has_water_access) {
        if (house_properties[level].water >= 2) {
            ++demands->missing.fountain;
            return 0;
        }
        if (house_properties[level].water == 1 && !house->has_well_access) {
            ++demands->missing.well;
            return 0;
        }
    }
    // entertainment
    if (house->data.house.entertainment < house_properties[level].entertainment) {
        if (house->data.house.entertainment) {
            ++demands->missing.more_entertainment;
        } else {
            ++demands->missing.entertainment;
        }
        return 0;
    }
    // education
    if (house->data.house.education < house_properties[level].education) {
        if (house->data.house.education) {
            ++demands->missing.more_education;
        } else {
            ++demands->missing.education;
        }
        return 0;
    }
    if (house_properties[level].education == 2) {
        ++demands->requiring.school;
        ++demands->requiring.library;
    } else if (house_properties[level].education == 1) {
        ++demands->requiring.school;
    }
    // religion
    if (house->data.house.num_gods < house_properties[level].religion) {
        if (house_properties[level].religion == 1) {
            ++demands->missing.religion;
            return 0;
        } else if (house_properties[level].religion == 2) {
            ++demands->missing.second_religion;
            return 0;
        } else if (house_properties[level].religion == 3) {
            ++demands->missing.third_religion;
            return 0;
        }
    } else if (house_properties[level].religion > 0) {
        ++demands->requiring.religion;
    }
    // barber
    if (house->data.house.barber < house_properties[level].barber) {
        ++demands->missing.barber;
        return 0;
    }
    if (house_properties[level].barber == 1) {
        ++demands->requiring.barber;
    }
    // bathhouse
    if (house->data.house.bathhouse < house_properties[level].bathhouse) {
        ++demands->missing.bathhouse;
        return 0;
    }
    if (house_properties[level].bathhouse == 1) {
        ++demands->requiring.bathhouse;
    }
    // health
    if (house->data.house.health < house_properties[level].health) {
        if (house_properties[level].health < 2) {
            ++demands->missing.clinic;
        } else {
            ++demands->missing.hospital;
        }
        return 0;
    }
    if (house_properties[level].health >= 1) {
        ++demands->requiring.clinic;
    }
    // food types
    int foodtypes_available = 0;
    for (int i = INVENTORY_WHEAT; i <= INVENTORY_MEAT; i++) {
        if (house->data.house.inventory[i]) {
            foodtypes_available++;
        }
    }
    if (foodtypes_available < house_properties[level].food_types) {
        ++demands->missing.food;
        return 0;
    }
    // goods
    if (house->data.house.inventory[INVENTORY_POTTERY] < house_properties[level].pottery) {
        return 0;
    }
    if (house->data.house.inventory[INVENTORY_OIL] < house_properties[level].oil) {
        return 0;
    }
    if (house->data.house.inventory[INVENTORY_FURNITURE] < house_properties[level].furniture) {
        return 0;
    }
    int wine_required = house_properties[level].wine;
    if (wine_required && !house->data.house.inventory[INVENTORY_WINE]) {
        return 0;
    }
    if (wine_required > 1 && !city_resource_multiple_wine_available()) {
        ++demands->missing.second_wine;
        return 0;
    }
    return 1;
}

static int check_requirements(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_evolve_desirability(house);
    if (!has_required_goods_and_services(house, 0, demands)) {
        status = DEVOLVE;
    } else if (status == EVOLVE) {
        status = has_required_goods_and_services(house, 1, demands);
    }
    return status;
}

static void prepare_for_merge(int building_id, int num_tiles)
{
    for (int i = 0; i < INVENTORY_MAX; i++) {
        merge_data.inventory[i] = 0;
    }
    merge_data.population = 0;
    int grid_offset = map_grid_offset(merge_data.x, merge_data.y);
    for (int i = 0; i < num_tiles; i++) {
        int house_offset = grid_offset + HOUSE_TILE_OFFSETS[i];
        if (map_terrain_is(house_offset, TERRAIN_BUILDING)) {
            struct building_t *house = &all_buildings[map_building_at(house_offset)];
            if (house->id != building_id && house->house_size) {
                merge_data.population += house->house_population;
                for (int inv = 0; inv < INVENTORY_MAX; inv++) {
                    merge_data.inventory[inv] += house->data.house.inventory[inv];
                    house->house_population = 0;
                    house->state = BUILDING_STATE_DELETED_BY_GAME;
                }
            }
        }
    }
}

static void building_house_merge(struct building_t *house)
{
    if (house->house_is_merged) {
        return;
    }
    if ((map_random_get(house->grid_offset) & 7) >= 5) {
        return;
    }
    int num_house_tiles = 0;
    for (int i = 0; i < 4; i++) {
        int tile_offset = house->grid_offset + HOUSE_TILE_OFFSETS[i];
        if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
            struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
            if (other_house->id == house->id) {
                num_house_tiles++;
            } else if (other_house->state == BUILDING_STATE_IN_USE && other_house->house_size &&
                    other_house->subtype.house_level == house->subtype.house_level &&
                    !other_house->house_is_merged) {
                num_house_tiles++;
            }
        }
    }
    if (num_house_tiles == 4) {
        game_undo_disable();
        merge_data.x = house->x + EXPAND_DIRECTION_DELTA[0].x;
        merge_data.y = house->y + EXPAND_DIRECTION_DELTA[0].y;
        prepare_for_merge(house->id, 4);
        house->size = house->house_size = 2;
        house->house_population += merge_data.population;
        for (int i = 0; i < INVENTORY_MAX; i++) {
            house->data.house.inventory[i] += merge_data.inventory[i];
        }
        int image_id = image_group(HOUSE_IMAGE[house->subtype.house_level].group) + 4;
        if (HOUSE_IMAGE[house->subtype.house_level].offset) {
            image_id += 1;
        }
        map_building_tiles_remove(house->id, house->x, house->y);
        house->x = merge_data.x;
        house->y = merge_data.y;
        house->grid_offset = map_grid_offset(house->x, house->y);
        house->house_is_merged = 1;
        map_building_tiles_add(house->id, house->x, house->y, 2, image_id, TERRAIN_BUILDING);
    }
}

static int evolve_small_tent(struct building_t *house, struct house_demands_t *demands)
{
    if (house->house_population > 0) {
        building_house_merge(house);
        int status = check_requirements(house, demands);
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_TENT);
        }
    }
    return 0;
}

static int has_devolve_delay(struct building_t *house, int status)
{
    if (status == DEVOLVE && house->data.house.devolve_delay < 2) {
        house->data.house.devolve_delay++;
        return 1;
    } else {
        house->data.house.devolve_delay = 0;
        return 0;
    }
}

static int evolve_large_tent(struct building_t *house, struct house_demands_t *demands)
{
    if (house->house_population > 0) {
        building_house_merge(house);
        int status = check_requirements(house, demands);
        if (!has_devolve_delay(house, status)) {
            if (status == EVOLVE) {
                building_house_change_to(house, BUILDING_HOUSE_SMALL_SHACK);
            } else if (status == DEVOLVE) {
                building_house_change_to(house, BUILDING_HOUSE_SMALL_TENT);
            }
        }
    }
    return 0;
}

static int evolve_small_shack(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_SHACK);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_TENT);
        }
    }
    return 0;
}

static int evolve_large_shack(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_HOVEL);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_SHACK);
        }
    }
    return 0;
}

static int evolve_small_hovel(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_HOVEL);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_SHACK);
        }
    }
    return 0;
}

static int evolve_large_hovel(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_CASA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_HOVEL);
        }
    }
    return 0;
}

static int evolve_small_casa(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_CASA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_HOVEL);
        }
    }
    return 0;
}

static int evolve_large_casa(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_INSULA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_CASA);
        }
    }
    return 0;
}

static int evolve_small_insula(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_MEDIUM_INSULA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_CASA);
        }
    }
    return 0;
}

static int house_image_group(int level)
{
    return image_group(HOUSE_IMAGE[level].group) + HOUSE_IMAGE[level].offset;
}

static void create_house_tile(int type, int x, int y, int image_id, int population, const int *inventory)
{
    struct building_t *house = building_create(type, x, y);
    house->house_population = population;
    for (int i = 0; i < INVENTORY_MAX; i++) {
        house->data.house.inventory[i] = inventory[i];
    }
    map_building_tiles_add(house->id, house->x, house->y, 1, image_id + (map_random_get(house->grid_offset) & 1), TERRAIN_BUILDING);
}

static void split_size2(struct building_t *house, int new_type)
{
    int inventory_per_tile[INVENTORY_MAX];
    int inventory_remainder[INVENTORY_MAX];
    for (int i = 0; i < INVENTORY_MAX; i++) {
        inventory_per_tile[i] = house->data.house.inventory[i] / 4;
        inventory_remainder[i] = house->data.house.inventory[i] % 4;
    }
    int population_per_tile = house->house_population / 4;
    int population_remainder = house->house_population % 4;

    map_building_tiles_remove(house->id, house->x, house->y);

    // main tile
    house->type = new_type;
    house->subtype.house_level = house->type - BUILDING_HOUSE_SMALL_TENT;
    house->size = house->house_size = 1;
    house->house_is_merged = 0;
    house->house_population = population_per_tile + population_remainder;
    for (int i = 0; i < INVENTORY_MAX; i++) {
        house->data.house.inventory[i] = inventory_per_tile[i] + inventory_remainder[i];
    }

    int image_id = house_image_group(house->subtype.house_level);
    map_building_tiles_add(house->id, house->x, house->y, house->size,
                           image_id + (map_random_get(house->grid_offset) & 1), TERRAIN_BUILDING);

    // the other tiles (new buildings)
    create_house_tile(house->type, house->x + 1, house->y, image_id, population_per_tile, inventory_per_tile);
    create_house_tile(house->type, house->x, house->y + 1, image_id, population_per_tile, inventory_per_tile);
    create_house_tile(house->type, house->x + 1, house->y + 1, image_id, population_per_tile, inventory_per_tile);
}

static void split(struct building_t *house, int num_tiles)
{
    int grid_offset = map_grid_offset(merge_data.x, merge_data.y);
    for (int i = 0; i < num_tiles; i++) {
        int tile_offset = grid_offset + HOUSE_TILE_OFFSETS[i];
        if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
            struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
            if (other_house->id != house->id && other_house->house_size) {
                if (other_house->house_is_merged == 1) {
                    split_size2(other_house, other_house->type);
                } else if (other_house->house_size == 2) {
                    split_size2(other_house, BUILDING_HOUSE_MEDIUM_INSULA);
                } else if (other_house->house_size == 3) {
                    int inventory_per_tile[INVENTORY_MAX];
                    int inventory_remainder[INVENTORY_MAX];
                    for (int ii = 0; ii < INVENTORY_MAX; ii++) {
                        inventory_per_tile[ii] = other_house->data.house.inventory[ii] / 9;
                        inventory_remainder[ii] = other_house->data.house.inventory[ii] % 9;
                    }
                    int population_per_tile = other_house->house_population / 9;
                    int population_remainder = other_house->house_population % 9;
                    map_building_tiles_remove(other_house->id, other_house->x, other_house->y);
                    // main tile
                    other_house->type = BUILDING_HOUSE_MEDIUM_INSULA;
                    other_house->subtype.house_level = other_house->type - BUILDING_HOUSE_SMALL_TENT;
                    other_house->size = other_house->house_size = 1;
                    other_house->house_is_merged = 0;
                    other_house->house_population = population_per_tile + population_remainder;
                    for (int ii = 0; ii < INVENTORY_MAX; ii++) {
                        other_house->data.house.inventory[ii] = inventory_per_tile[ii] + inventory_remainder[ii];
                    }

                    int image_id = house_image_group(other_house->subtype.house_level);
                    map_building_tiles_add(other_house->id, other_house->x, other_house->y, other_house->size,
                                           image_id + (map_random_get(other_house->grid_offset) & 1), TERRAIN_BUILDING);
                    // the other tiles (new buildings)
                    create_house_tile(other_house->type, other_house->x, other_house->y + 1, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x + 1, other_house->y + 1, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x + 2, other_house->y + 1, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x, other_house->y + 2, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x + 1, other_house->y + 2, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x + 2, other_house->y + 2, image_id, population_per_tile, inventory_per_tile);
                }
            }
        }
    }
}

static int building_house_can_expand(struct building_t *house, int num_tiles)
{
    // merge with other houses
    for (int dir = 0; dir < MAX_DIR; dir++) {
        int base_offset = EXPAND_DIRECTION_DELTA[dir].offset + house->grid_offset;
        int ok_tiles = 0;
        for (int i = 0; i < num_tiles; i++) {
            int tile_offset = base_offset + HOUSE_TILE_OFFSETS[i];
            if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
                struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
                if (other_house->id == house->id) {
                    ok_tiles++;
                } else if (other_house->state == BUILDING_STATE_IN_USE && other_house->house_size) {
                    if (other_house->subtype.house_level <= house->subtype.house_level) {
                        ok_tiles++;
                    }
                }
            }
        }
        if (ok_tiles == num_tiles) {
            merge_data.x = house->x + EXPAND_DIRECTION_DELTA[dir].x;
            merge_data.y = house->y + EXPAND_DIRECTION_DELTA[dir].y;
            return 1;
        }
    }
    // merge with houses and empty terrain
    for (int dir = 0; dir < MAX_DIR; dir++) {
        int base_offset = EXPAND_DIRECTION_DELTA[dir].offset + house->grid_offset;
        int ok_tiles = 0;
        for (int i = 0; i < num_tiles; i++) {
            int tile_offset = base_offset + HOUSE_TILE_OFFSETS[i];
            if (!map_terrain_is(tile_offset, TERRAIN_NOT_CLEAR)) {
                ok_tiles++;
            } else if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
                struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
                if (other_house->id == house->id) {
                    ok_tiles++;
                } else if (other_house->state == BUILDING_STATE_IN_USE && other_house->house_size) {
                    if (other_house->subtype.house_level <= house->subtype.house_level) {
                        ok_tiles++;
                    }
                }
            }
        }
        if (ok_tiles == num_tiles) {
            merge_data.x = house->x + EXPAND_DIRECTION_DELTA[dir].x;
            merge_data.y = house->y + EXPAND_DIRECTION_DELTA[dir].y;
            return 1;
        }
    }
    // merge with houses, empty terrain and gardens
    for (int dir = 0; dir < MAX_DIR; dir++) {
        int base_offset = EXPAND_DIRECTION_DELTA[dir].offset + house->grid_offset;
        int ok_tiles = 0;
        for (int i = 0; i < num_tiles; i++) {
            int tile_offset = base_offset + HOUSE_TILE_OFFSETS[i];
            if (!map_terrain_is(tile_offset, TERRAIN_NOT_CLEAR)) {
                ok_tiles++;
            } else if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
                struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
                if (other_house->id == house->id) {
                    ok_tiles++;
                } else if (other_house->state == BUILDING_STATE_IN_USE && other_house->house_size) {
                    if (other_house->subtype.house_level <= house->subtype.house_level) {
                        ok_tiles++;
                    }
                }
            } else if (map_terrain_is(tile_offset, TERRAIN_GARDEN)) {
                ok_tiles++;
            }
        }
        if (ok_tiles == num_tiles) {
            merge_data.x = house->x + EXPAND_DIRECTION_DELTA[dir].x;
            merge_data.y = house->y + EXPAND_DIRECTION_DELTA[dir].y;
            return 1;
        }
    }
    house->data.house.no_space_to_expand = 1;
    return 0;
}

static int evolve_medium_insula(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            if (building_house_can_expand(house, 4)) {
                game_undo_disable();
                house->house_is_merged = 0;
                split(house, 4);
                prepare_for_merge(house->id, 4);

                house->type = BUILDING_HOUSE_LARGE_INSULA;
                house->subtype.house_level = HOUSE_LARGE_INSULA;
                house->size = house->house_size = 2;
                house->house_population += merge_data.population;
                for (int i = 0; i < INVENTORY_MAX; i++) {
                    house->data.house.inventory[i] += merge_data.inventory[i];
                }
                int image_id = house_image_group(house->subtype.house_level) + (map_random_get(house->grid_offset) & 1);
                map_building_tiles_remove(house->id, house->x, house->y);
                house->x = merge_data.x;
                house->y = merge_data.y;
                house->grid_offset = map_grid_offset(house->x, house->y);
                map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);
                map_tiles_update_all_gardens();
                return 1;
            }
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_INSULA);
        }
    }
    return 0;
}

static int evolve_large_insula(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_INSULA);
        } else if (status == DEVOLVE) {
            game_undo_disable();
            split_size2(house, BUILDING_HOUSE_MEDIUM_INSULA);
        }
    }
    return 0;
}

static int evolve_grand_insula(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_VILLA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_INSULA);
        }
    }
    return 0;
}

static int evolve_small_villa(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_MEDIUM_VILLA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_INSULA);
        }
    }
    return 0;
}

static int evolve_medium_villa(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            if (building_house_can_expand(house, 9)) {
                game_undo_disable();
                split(house, 9);
                prepare_for_merge(house->id, 9);

                house->type = BUILDING_HOUSE_LARGE_VILLA;
                house->subtype.house_level = HOUSE_LARGE_VILLA;
                house->size = house->house_size = 3;
                house->house_population += merge_data.population;
                for (int i = 0; i < INVENTORY_MAX; i++) {
                    house->data.house.inventory[i] += merge_data.inventory[i];
                }
                int image_id = house_image_group(house->subtype.house_level);
                map_building_tiles_remove(house->id, house->x, house->y);
                house->x = merge_data.x;
                house->y = merge_data.y;
                house->grid_offset = map_grid_offset(house->x, house->y);
                map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);
                map_tiles_update_all_gardens();
                return 1;
            }
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_VILLA);
        }
    }
    return 0;
}

static int evolve_large_villa(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_VILLA);
        } else if (status == DEVOLVE) {
            game_undo_disable();
            int inventory_per_tile[INVENTORY_MAX];
            int inventory_remainder[INVENTORY_MAX];
            for (int i = 0; i < INVENTORY_MAX; i++) {
                inventory_per_tile[i] = house->data.house.inventory[i] / 6;
                inventory_remainder[i] = house->data.house.inventory[i] % 6;
            }
            int population_per_tile = house->house_population / 6;
            int population_remainder = house->house_population % 6;

            map_building_tiles_remove(house->id, house->x, house->y);

            // main tile
            house->type = BUILDING_HOUSE_MEDIUM_VILLA;
            house->subtype.house_level = house->type - BUILDING_HOUSE_SMALL_TENT;
            house->size = house->house_size = 2;
            house->house_is_merged = 0;
            house->house_population = population_per_tile + population_remainder;
            for (int i = 0; i < INVENTORY_MAX; i++) {
                house->data.house.inventory[i] = inventory_per_tile[i] + inventory_remainder[i];
            }

            int image_id = house_image_group(house->subtype.house_level);
            map_building_tiles_add(house->id, house->x, house->y, house->size, image_id + (map_random_get(house->grid_offset) & 1), TERRAIN_BUILDING);

            // the other tiles (new buildings)
            image_id = house_image_group(HOUSE_MEDIUM_INSULA);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 2, house->y, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 2, house->y + 1, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x, house->y + 2, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 1, house->y + 2, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 2, house->y + 2, image_id, population_per_tile, inventory_per_tile);
        }
    }
    return 0;
}

static int evolve_grand_villa(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_PALACE);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_VILLA);
        }
    }
    return 0;
}

static int evolve_small_palace(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_MEDIUM_PALACE);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_VILLA);
        }
    }
    return 0;
}

static int evolve_medium_palace(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            if (building_house_can_expand(house, 16)) {
                game_undo_disable();
                split(house, 16);
                prepare_for_merge(house->id, 16);

                house->type = BUILDING_HOUSE_LARGE_PALACE;
                house->subtype.house_level = HOUSE_LARGE_PALACE;
                house->size = house->house_size = 4;
                house->house_population += merge_data.population;
                for (int i = 0; i < INVENTORY_MAX; i++) {
                    house->data.house.inventory[i] += merge_data.inventory[i];
                }
                int image_id = house_image_group(house->subtype.house_level);
                map_building_tiles_remove(house->id, house->x, house->y);
                house->x = merge_data.x;
                house->y = merge_data.y;
                house->grid_offset = map_grid_offset(house->x, house->y);
                map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);
                map_tiles_update_all_gardens();
                return 1;
            }
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_PALACE);
        }
    }
    return 0;
}

static int evolve_large_palace(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LUXURY_PALACE);
        } else if (status == DEVOLVE) {
            game_undo_disable();
            int inventory_per_tile[INVENTORY_MAX];
            int inventory_remainder[INVENTORY_MAX];
            for (int i = 0; i < INVENTORY_MAX; i++) {
                inventory_per_tile[i] = house->data.house.inventory[i] / 8;
                inventory_remainder[i] = house->data.house.inventory[i] % 8;
            }
            int population_per_tile = house->house_population / 8;
            int population_remainder = house->house_population % 8;

            map_building_tiles_remove(house->id, house->x, house->y);

            // main tile
            house->type = BUILDING_HOUSE_MEDIUM_PALACE;
            house->subtype.house_level = house->type - BUILDING_HOUSE_SMALL_TENT;
            house->size = house->house_size = 3;
            house->house_is_merged = 0;
            house->house_population = population_per_tile + population_remainder;
            for (int i = 0; i < INVENTORY_MAX; i++) {
                house->data.house.inventory[i] = inventory_per_tile[i] + inventory_remainder[i];
            }

            int image_id = house_image_group(house->subtype.house_level);
            map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);

            // the other tiles (new buildings)
            image_id = house_image_group(HOUSE_MEDIUM_INSULA);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 3, house->y, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 3, house->y + 1, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 3, house->y + 2, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x, house->y + 3, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 1, house->y + 3, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 2, house->y + 3, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 3, house->y + 3, image_id, population_per_tile, inventory_per_tile);
        }
    }
    return 0;
}

static int evolve_luxury_palace(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_evolve_desirability(house);
    if (!has_required_goods_and_services(house, 0, demands)) {
        status = DEVOLVE;
    }
    if (!has_devolve_delay(house, status) && status == DEVOLVE) {
        building_house_change_to(house, BUILDING_HOUSE_LARGE_PALACE);
    }
    return 0;
}

static int (*evolve_callback[])(struct building_t *, struct house_demands_t *) = {
    evolve_small_tent, evolve_large_tent, evolve_small_shack, evolve_large_shack,
    evolve_small_hovel, evolve_large_hovel, evolve_small_casa, evolve_large_casa,
    evolve_small_insula, evolve_medium_insula, evolve_large_insula, evolve_grand_insula,
    evolve_small_villa, evolve_medium_villa, evolve_large_villa, evolve_grand_villa,
    evolve_small_palace, evolve_medium_palace, evolve_large_palace, evolve_luxury_palace
};

static void consume_resource(struct building_t *b, int inventory, int amount)
{
    if (amount > 0) {
        if (amount > b->data.house.inventory[inventory]) {
            b->data.house.inventory[inventory] = 0;
        } else {
            b->data.house.inventory[inventory] -= amount;
        }
    }
}

static int map_water_get_wharf_for_new_fishing_boat(struct figure_t *boat, struct map_point_t *tile)
{
    struct building_t *wharf = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_WHARF) {
            int wharf_boat_id = b->data.industry.fishing_boat_id;
            if (!wharf_boat_id || wharf_boat_id == boat->id) {
                wharf = b;
                break;
            }
        }
    }
    if (!wharf) {
        return 0;
    }
    int dx, dy;
    switch (wharf->data.industry.orientation) {
        case 0: dx = 1; dy = -1; break;
        case 1: dx = 2; dy = 1; break;
        case 2: dx = 1; dy = 2; break;
        default: dx = -1; dy = 1; break;
    }
    tile->x = wharf->x + dx;
    tile->y = wharf->y + dy;
    return wharf->id;
}

static int building_dock_get_queue_destination(struct map_point_t *tile)
{
    if (!city_data.building.working_docks) {
        return 0;
    }
    // first queue position
    for (int i = 0; i < 10; i++) {
        int dock_id = city_data.building.working_dock_ids[i];
        if (!dock_id) continue;
        struct building_t *dock = &all_buildings[dock_id];
        int dx, dy;
        switch (dock->data.dock.orientation) {
            case 0: dx = 2; dy = -2; break;
            case 1: dx = 4; dy = 2; break;
            case 2: dx = 2; dy = 4; break;
            default: dx = -2; dy = 2; break;
        }
        tile->x = dock->x + dx;
        tile->y = dock->y + dy;
        if (!map_has_figure_at(map_grid_offset(tile->x, tile->y))) {
            return dock_id;
        }
    }
    // second queue position
    for (int i = 0; i < 10; i++) {
        int dock_id = city_data.building.working_dock_ids[i];
        if (!dock_id) continue;
        struct building_t *dock = &all_buildings[dock_id];
        int dx, dy;
        switch (dock->data.dock.orientation) {
            case 0: dx = 2; dy = -3; break;
            case 1: dx = 5; dy = 2; break;
            case 2: dx = 2; dy = 5; break;
            default: dx = -3; dy = 2; break;
        }
        tile->x = dock->x + dx;
        tile->y = dock->y + dy;
        if (!map_has_figure_at(map_grid_offset(tile->x, tile->y))) {
            return dock_id;
        }
    }
    return 0;
}

static int building_dock_get_free_destination(int ship_id, struct map_point_t *tile)
{
    if (!city_data.building.working_docks) {
        return 0;
    }
    int dock_id = 0;
    for (int i = 0; i < 10; i++) {
        dock_id = city_data.building.working_dock_ids[i];
        if (!dock_id) continue;
        struct building_t *dock = &all_buildings[dock_id];
        if (!dock->data.dock.trade_ship_id || dock->data.dock.trade_ship_id == ship_id) {
            break;
        }
    }
    // BUG: when 10 docks in city, always takes last one... regardless of whether it is free
    if (dock_id <= 0) {
        return 0;
    }
    struct building_t *dock = &all_buildings[dock_id];
    int dx, dy;
    switch (dock->data.dock.orientation) {
        case 0: dx = 1; dy = -1; break;
        case 1: dx = 3; dy = 1; break;
        case 2: dx = 1; dy = 3; break;
        default: dx = -1; dy = 1; break;
    }
    tile->x = dock->x + dx;
    tile->y = dock->y + dy;
    dock->data.dock.trade_ship_id = ship_id;
    return dock_id;
}

static int get_closest_warehouse(const struct figure_t *f, int x, int y, int city_id, struct map_point_t *warehouse)
{
    int exportable[RESOURCE_TYPES_MAX];
    int importable[RESOURCE_TYPES_MAX];
    exportable[RESOURCE_NONE] = 0;
    importable[RESOURCE_NONE] = 0;
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        exportable[r] = can_export_resource_to_trade_city(city_id, r);
        if (f->trader_amount_bought >= 8) {
            exportable[r] = 0;
        }
        if (city_id) {
            importable[r] = can_import_resource_from_trade_city(city_id, r);
        } else { // Don't import goods from native traders
            importable[r] = 0;
        }
        if (f->loads_sold_or_carrying >= 8) {
            importable[r] = 0;
        }
    }
    int num_importable = 0;
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        if (importable[r]) {
            num_importable++;
        }
    }
    int min_distance = 10000;
    struct building_t *min_building = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_WAREHOUSE) {
            continue;
        }
        if (!b->has_road_access) {
            continue;
        }
        struct building_storage_t *s = building_storage_get(b->storage_id);
        int num_imports_for_warehouse = 0;
        for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
            if (s->resource_state[r] != BUILDING_STORAGE_STATE_NOT_ACCEPTING
                && can_import_resource_from_trade_city(city_id, r)) {
                num_imports_for_warehouse++;
            }
        }
        int distance_penalty = 32;
        struct building_t *space = b;
        for (int space_cnt = 0; space_cnt < 8; space_cnt++) {
            space = &all_buildings[space->next_part_building_id];
            if (space->id && exportable[space->subtype.warehouse_resource_id]) {
                distance_penalty -= 4;
            }
            if (num_importable && num_imports_for_warehouse && !s->empty_all) {
                for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                    int import_resource = city_trade_next_caravan_import_resource();
                    if (s->resource_state[import_resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
                        break;
                    }
                }
                int resource = city_data.trade.caravan_import_resource;
                if (s->resource_state[resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
                    if (space->subtype.warehouse_resource_id == RESOURCE_NONE) {
                        distance_penalty -= 16;
                    }
                    if (space->id && importable[space->subtype.warehouse_resource_id] && space->loads_stored < 4 &&
                        space->subtype.warehouse_resource_id == resource) {
                        distance_penalty -= 8;
                    }
                }
            }
        }
        if (distance_penalty < 32) {
            int distance = calc_maximum_distance(b->x, b->y, x, y);
            distance += distance_penalty;
            if (distance < min_distance) {
                min_distance = distance;
                min_building = b;
            }
        }
    }
    if (!min_building) {
        return 0;
    }
    if (min_building->has_road_access == 1) {
        warehouse->x = min_building->x;
        warehouse->y = min_building->y;
    } else if (!map_has_road_access(min_building->x, min_building->y, 3, warehouse)) {
        return 0;
    }
    return min_building->id;
}

static void go_to_next_warehouse(struct figure_t *f, int x_src, int y_src)
{
    struct map_point_t dst;
    int warehouse_id = get_closest_warehouse(f, x_src, y_src, f->empire_city_id, &dst);
    if (warehouse_id) {
        f->destination_building_id = warehouse_id;
        f->action_state = FIGURE_ACTION_TRADE_CARAVAN_ARRIVING;
        f->destination_x = dst.x;
        f->destination_y = dst.y;
    } else {
        f->action_state = FIGURE_ACTION_TRADE_CARAVAN_LEAVING;
        f->destination_x = scenario.exit_point.x;
        f->destination_y = scenario.exit_point.y;
    }
}

static int trader_get_buy_resource(int warehouse_id, int city_id)
{
    struct building_t *warehouse = &all_buildings[warehouse_id];
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return RESOURCE_NONE;
    }
    struct building_t *space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = &all_buildings[space->next_part_building_id];
        if (space->id <= 0) {
            continue;
        }
        int resource = space->subtype.warehouse_resource_id;
        if (space->loads_stored > 0 && can_export_resource_to_trade_city(city_id, resource)) {
            // update stocks
            city_resource_remove_from_warehouse(resource, 1);
            space->loads_stored--;
            if (space->loads_stored <= 0) {
                space->subtype.warehouse_resource_id = RESOURCE_NONE;
            }
            // update finances
            city_finance_process_export(trade_prices[resource].sell);

            // update graphics
            building_warehouse_space_set_image(space, resource);
            return resource;
        }
    }
    return 0;
}

static void roamer_action(struct figure_t *f, int num_ticks)
{
    switch (f->action_state) {
        case FIGURE_ACTION_ROAMING:
            f->is_invisible = 0;
            f->roam_length++;
            if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                int x, y;
                struct building_t *b = &all_buildings[f->building_id];
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x, &y)) {
                    f->action_state = FIGURE_ACTION_ROAMER_RETURNING;
                    f->destination_x = x;
                    f->destination_y = y;
                    figure_route_remove(f);
                    f->roam_length = 0;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            figure_movement_roam_ticks(f, num_ticks);
            break;
        case FIGURE_ACTION_ROAMER_RETURNING:
            figure_movement_move_ticks(f, num_ticks);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
    }
}

static void culture_action(struct figure_t *f, int group)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        figure_delete(f);
        return;
    }
    figure_image_increase_offset(f, 12);
    roamer_action(f, 1);
    f->image_id = image_group(group) + figure_image_direction(f) + 8 * f->image_offset;
}

static void missile_hit_target(struct figure_t *projectile, struct figure_t *target)
{
    struct figure_t *shooter = &figures[projectile->building_id];
    int damage_inflicted = (figure_properties[shooter->type].missile_attack_value + figure_properties[projectile->type].missile_attack_value) - figure_properties[target->type].missile_defense_value;
    if (damage_inflicted < 0) {
        damage_inflicted = 0;
    }
    if (projectile->type != FIGURE_BOLT
        && ((target->type == FIGURE_FORT_LEGIONARY && legion_formations[target->formation_id].layout == FORMATION_TORTOISE)
            || (target->type == FIGURE_ENEMY_CAESAR_LEGIONARY && enemy_formations[target->formation_id].layout == FORMATION_TORTOISE))
            && target->figure_is_halted) {
        damage_inflicted = 1;
    }
    int target_damage = damage_inflicted + target->damage;
    if (target_damage <= figure_properties[target->type].max_damage) {
        target->damage = target_damage;
    } else { // kill target
        target->damage = figure_properties[target->type].max_damage + 1;
        target->is_corpse = 1;
        target->is_targetable = 0;
        target->wait_ticks = 0;
        figure_play_die_sound(target);
        if (figure_properties[target->type].is_player_legion_unit) {
            update_formation_morale_after_death(&legion_formations[target->formation_id]);
        } else {
            update_formation_morale_after_death(&enemy_formations[target->formation_id]);
        }
        clear_targeting_on_unit_death(target);
    }
    if (figure_properties[target->type].is_player_legion_unit) {
        legion_formations[target->formation_id].missile_attack_timeout = 6;
    } else if (figure_properties[target->type].is_herd_animal) {
        herd_formations[target->formation_id].missile_attack_timeout = 6;
    } else if (figure_properties[target->type].is_enemy_unit || figure_properties[target->type].is_caesar_legion_unit) {
        enemy_formations[target->formation_id].missile_attack_timeout = 6;
    }
    // clear targeting
    shooter->target_figure_id = 0;
    figure__remove_ranged_targeter_from_list(target, shooter);
    figure_delete(projectile);
}

static int get_target_on_tile(struct figure_t *projectile)
{
    struct figure_t *shooter = &figures[projectile->building_id];
    if (map_figures.items[projectile->grid_offset] > 0) {
        int figure_id = map_figures.items[projectile->grid_offset];
        while (figure_id) {
            struct figure_t *target = &figures[figure_id];
            if (figure_is_alive(target) && target->is_targetable) {
                if (figure_properties[shooter->type].is_friendly_armed_unit || figure_properties[shooter->type].is_player_legion_unit) {
                    if (is_valid_target_for_player_unit(target)) {
                        return target->id;
                    }
                } else if (figure_properties[shooter->type].is_enemy_unit) {
                    if (is_valid_target_for_enemy_unit(target)) {
                        return target->id;
                    }
                }
            }
            figure_id = target->next_figure_id_on_same_tile;
        }
    }
    return 0;
}

static int closest_house_with_room(int x, int y)
{
    int min_dist = 1000;
    int min_building_id = 0;
    int max_id = building_get_highest_id();
    for (int i = 1; i <= max_id; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size
            && b->house_population_room > 0) {
            if (!b->immigrant_figure_id) {
                int dist = calc_maximum_distance(x, y, b->x, b->y);
                if (dist < min_dist) {
                    min_dist = dist;
                    min_building_id = i;
                }
            }
        }
    }
    return min_building_id;
}

static void update_migrant_dir_and_image(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    f->image_id = image_group(GROUP_FIGURE_MIGRANT) + figure_image_direction(f) + 8 * f->image_offset;
    if (f->action_state == FIGURE_ACTION_IMMIGRANT_ARRIVING || f->action_state == FIGURE_ACTION_EMIGRANT_LEAVING) {
        int dir = figure_image_direction(f);
        f->cart_image_id = image_group(GROUP_FIGURE_MIGRANT_CART) + dir;
        figure_image_set_cart_offset(f, (dir + 4) % 8);
    }
}

static int create_delivery_boy(int leader_id, struct figure_t *f)
{
    struct figure_t *boy = figure_create(FIGURE_DELIVERY_BOY, f->x, f->y, 0);
    boy->is_targetable = 1;
    boy->terrain_usage = TERRAIN_USAGE_ROADS;
    boy->leading_figure_id = leader_id;
    boy->collecting_item_id = f->collecting_item_id;
    boy->building_id = f->building_id;
    return boy->id;
}

static int fight_fire(struct figure_t *f)
{
    if (building_list_burning_size() <= 0) {
        return 0;
    }
    switch (f->action_state) {
        case FIGURE_ACTION_PREFECT_CREATED:
        case FIGURE_ACTION_PREFECT_ENTERING_EXITING:
        case FIGURE_ACTION_PREFECT_GOING_TO_FIRE:
        case FIGURE_ACTION_PREFECT_AT_FIRE:
            return 0;
    }
    f->wait_ticks_missile++;
    if (f->wait_ticks_missile < 20) {
        return 0;
    }
    int distance;
    int ruin_id = 0;
    int min_occupied_building_id = 0;
    int min_occupied_dist = distance = 10000;
    const int *burning = building_list_burning_items();
    int burning_size = building_list_burning_size();
    for (int i = 0; i < burning_size; i++) {
        int building_id = burning[i];
        struct building_t *b = &all_buildings[building_id];
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_BURNING_RUIN && !b->ruin_has_plague) {
            int dist = calc_maximum_distance(f->x, f->y, b->x, b->y);
            if (b->figure_id4) {
                if (dist < min_occupied_dist) {
                    min_occupied_dist = dist;
                    min_occupied_building_id = building_id;
                }
            } else if (dist < distance) {
                distance = dist;
                ruin_id = building_id;
            }
        }
    }
    if (!ruin_id && min_occupied_dist <= 2) {
        ruin_id = min_occupied_building_id;
        distance = 2;
    }
    if (ruin_id > 0 && distance <= 25) {
        struct building_t *ruin = &all_buildings[ruin_id];
        f->wait_ticks_missile = 0;
        f->action_state = FIGURE_ACTION_PREFECT_GOING_TO_FIRE;
        f->destination_x = ruin->road_access_x;
        f->destination_y = ruin->road_access_y;
        f->destination_building_id = ruin_id;
        figure_route_remove(f);
        ruin->figure_id4 = f->id;
        return 1;
    }
    return 0;
}

static int determine_destination(int x, int y, int type1, int type2)
{
    int road_network = map_road_network_get(map_grid_offset(x, y));
    building_list_small_clear();

    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        if (b->type != type1 && b->type != type2) {
            continue;
        }
        if (b->road_network_id == road_network) {
            if (b->type == BUILDING_HIPPODROME && b->prev_part_building_id) {
                continue;
            }
            building_list_small_add(i);
        }
    }
    int total_venues = building_list_small_size();
    if (total_venues <= 0) {
        return 0;
    }
    const int *venues = building_list_small_items();
    int min_building_id = 0;
    int min_distance = 10000;
    for (int i = 0; i < total_venues; i++) {
        struct building_t *b = &all_buildings[venues[i]];
        int days_left;
        if (b->type == type1) {
            days_left = b->data.entertainment.days1;
        } else if (b->type == type2) {
            days_left = b->data.entertainment.days2;
        } else {
            days_left = 0;
        }
        int dist = days_left + calc_maximum_distance(x, y, b->x, b->y);
        if (dist < min_distance) {
            min_distance = dist;
            min_building_id = venues[i];
        }
    }
    return min_building_id;
}

static void shoot_enemy_missile(struct figure_t *f, struct map_point_t *tile)
{
    f->is_shooting = 1;
    f->attack_image_offset = 1;
    figure_create_missile(f, tile, figure_properties[f->type].missile_type);
    if (figure_properties[f->type].missile_type == FIGURE_ARROW) {
        play_sound_effect(SOUND_EFFECT_ARROW);
    }
    f->wait_ticks_missile = 0;
    // clear targeting
    figure__remove_ranged_targeter_from_list(&figures[f->target_figure_id], f);
    f->target_figure_id = 0;
}

static void spawn_enemy(struct figure_t *f, struct formation_t *m)
{
    if (f->wait_ticks) {
        f->wait_ticks--;
        if (!f->wait_ticks) {
            if (f->index_in_formation % 4 < 1) {
                if (m->layout == FORMATION_ENEMY_MOB) {
                    play_speech_file("wavs/drums.wav");
                } else {
                    play_speech_file("wavs/horn1.wav");
                }
            }
            f->is_invisible = 0;
            f->action_state = FIGURE_ACTION_ENEMY_ADVANCING;
        }
    }
}

static void ranged_enemy_action(struct figure_t *f)
{
    struct formation_t *m = &enemy_formations[f->formation_id];
    struct map_point_t tile = { -1, -1 };
    if (f->is_shooting) {
        f->attack_image_offset++;
        if (f->attack_image_offset > 100) {
            f->attack_image_offset = 0;
            f->is_shooting = 0;
        }
    } else {
        f->wait_ticks_missile++;
        if (f->wait_ticks_missile > 250) {
            f->wait_ticks_missile = 250;
        }
    }
    switch (f->action_state) {
        case FIGURE_ACTION_ENEMY_SPAWNING:
            spawn_enemy(f, m);
            break;
        case FIGURE_ACTION_ENEMY_REGROUPING:
            map_figure_update(f);
            f->image_offset = 0;
            if (f->wait_ticks_missile > figure_properties[f->type].missile_delay && set_missile_target(f, &tile)) {
                f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                shoot_enemy_missile(f, &tile);
            }
            break;
        case FIGURE_ACTION_ENEMY_ADVANCING:
            f->destination_x = m->destination_x + formation_layout_position_x(m->layout, f->index_in_formation);
            f->destination_y = m->destination_y + formation_layout_position_y(m->layout, f->index_in_formation);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if ((f->type == FIGURE_ENEMY_HUN_MOUNTED_ARCHER || f->type == FIGURE_ENEMY_GOTH_MOUNTED_ARCHER || f->type == FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER)
            && f->wait_ticks_missile > figure_properties[f->type].missile_delay
            && set_missile_target(f, &tile)) {
                shoot_enemy_missile(f, &tile);
            }
            if (f->direction == DIR_FIGURE_AT_DESTINATION
                || f->direction == DIR_FIGURE_REROUTE
                || f->direction == DIR_FIGURE_LOST) {
                figure_route_remove(f);
                f->action_state = FIGURE_ACTION_ENEMY_REGROUPING;
            }
            break;
        case FIGURE_ACTION_ENEMY_ENGAGED:
            if (f->target_figure_id && calc_maximum_distance(f->x, f->y, f->destination_x, f->destination_y) < figure_properties[f->type].max_range) {
                figure_route_remove(f);
                f->destination_x = f->x;
                f->destination_y = f->y;
                f->image_offset = 0;
                if (f->wait_ticks_missile > figure_properties[f->type].missile_delay && set_missile_target(f, &tile)) {
                    f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                    shoot_enemy_missile(f, &tile);
                    break;
                }
            } else {
                figure_movement_move_ticks(f, f->speed_multiplier);
                if (f->direction == DIR_FIGURE_AT_DESTINATION
                    || f->direction == DIR_FIGURE_REROUTE
                    || f->direction == DIR_FIGURE_LOST) {
                    figure_route_remove(f);
                    f->action_state = FIGURE_ACTION_ENEMY_REGROUPING;
                }
            }
            break;
    }
}

static void melee_enemy_action(struct figure_t *f)
{
    struct formation_t *m = &enemy_formations[f->formation_id];
    switch (f->action_state) {
        case FIGURE_ACTION_ENEMY_SPAWNING:
            spawn_enemy(f, m);
            break;
        case FIGURE_ACTION_ENEMY_REGROUPING:
            map_figure_update(f);
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_ENEMY_ADVANCING:
            f->destination_x = m->destination_x + formation_layout_position_x(m->layout, f->index_in_formation);
            f->destination_y = m->destination_y + formation_layout_position_y(m->layout, f->index_in_formation);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION
                || f->direction == DIR_FIGURE_REROUTE
                || f->direction == DIR_FIGURE_LOST) {
                figure_route_remove(f);
                f->action_state = FIGURE_ACTION_ENEMY_REGROUPING;
            }
            break;
        case FIGURE_ACTION_ENEMY_ENGAGED:
        {
            struct figure_t *target_unit = melee_unit__set_closest_target(f);
            if (target_unit) {
                f->destination_x = target_unit->x;
                f->destination_y = target_unit->y;
                figure_movement_move_ticks(f, f->speed_multiplier);
            } else {
                figure_movement_move_ticks(f, f->speed_multiplier);
            }
            if (f->direction == DIR_FIGURE_AT_DESTINATION
                || f->direction == DIR_FIGURE_REROUTE
                || f->direction == DIR_FIGURE_LOST) {
                figure_route_remove(f);
                f->action_state = FIGURE_ACTION_ENEMY_REGROUPING;
            }
            break;
        }
    }
}

static void figure_editor_flag_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 16);
    f->image_id = MAP_FLAG_IMG_ID + f->image_offset / 2;
    map_figure_delete(f);

    struct map_point_t point = { 0, 0 };
    int image_base = image_group(GROUP_FIGURE_MAP_FLAG_ICONS);
    if (f->resource_id == MAP_FLAG_ENTRY) {
        point.x = scenario.entry_point.x;
        point.y = scenario.entry_point.y;
        f->cart_image_id = image_base + 2;
    } else if (f->resource_id == MAP_FLAG_EXIT) {
        point.x = scenario.exit_point.x;
        point.y = scenario.exit_point.y;
        f->cart_image_id = image_base + 3;
    } else if (f->resource_id == MAP_FLAG_RIVER_ENTRY) {
        point.x = scenario.river_entry_point.x;
        point.y = scenario.river_entry_point.y;
        f->cart_image_id = image_base + 4;
    } else if (f->resource_id == MAP_FLAG_RIVER_EXIT) {
        point.x = scenario.river_exit_point.x;
        point.y = scenario.river_exit_point.y;
        f->cart_image_id = image_base + 5;
    } else if (f->resource_id >= MAP_FLAG_EARTHQUAKE_MIN && f->resource_id <= MAP_FLAG_EARTHQUAKE_MAX) {
        point = scenario.earthquake_points[f->resource_id - MAP_FLAG_EARTHQUAKE_MIN];
        f->cart_image_id = image_base;
    } else if (f->resource_id >= MAP_FLAG_INVASION_MIN && f->resource_id <= MAP_FLAG_INVASION_MAX) {
        point = scenario.invasion_points[f->resource_id - MAP_FLAG_INVASION_MIN];
        f->cart_image_id = image_base + 1;
    } else if (f->resource_id >= MAP_FLAG_FISHING_MIN && f->resource_id <= MAP_FLAG_FISHING_MAX) {
        point = scenario.fishing_points[f->resource_id - MAP_FLAG_FISHING_MIN];
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_ICONS) + 3;
    } else if (f->resource_id >= MAP_FLAG_HERD_MIN && f->resource_id <= MAP_FLAG_HERD_MAX) {
        point = scenario.herd_points[f->resource_id - MAP_FLAG_HERD_MIN];
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_ICONS) + 4;
    }
    f->x = point.x;
    f->y = point.y;

    f->grid_offset = map_grid_offset(f->x, f->y);
    f->cross_country_x = 15 * f->x + 7;
    f->cross_country_y = 15 * f->y + 7;
    map_figure_add(f);
}

static void get_trade_center_location(const struct figure_t *f, int *x, int *y)
{
    if (city_data.building.trade_center_building_id) {
        struct building_t *trade_center = &all_buildings[city_data.building.trade_center_building_id];
        *x = trade_center->x;
        *y = trade_center->y;
    } else {
        *x = f->x;
        *y = f->y;
    }
}

static int fetch_export_resource(struct figure_t *f, struct building_t *dock)
{
    int ship_id = dock->data.dock.trade_ship_id;
    if (!ship_id) {
        return 0;
    }
    struct figure_t *ship = &figures[ship_id];
    if (ship->action_state != FIGURE_ACTION_TRADE_SHIP_MOORED || ship->trader_amount_bought >= 12) {
        return 0;
    }
    int x, y;
    get_trade_center_location(f, &x, &y);
    struct map_point_t tile;
    int exportable[RESOURCE_TYPES_MAX];
    exportable[RESOURCE_NONE] = 0;
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        exportable[r] = can_export_resource_to_trade_city(ship->empire_city_id, r);
    }
    int resource = city_trade_next_docker_export_resource();
    for (int i = RESOURCE_WHEAT; i < RESOURCE_TYPES_MAX && !exportable[resource]; i++) {
        resource = city_trade_next_docker_export_resource();
    }
    int min_building_id = 0;
    if (exportable[resource]) {
        int min_distance = 10000;
        for (int i = 1; i < MAX_BUILDINGS; i++) {
            struct building_t *b = &all_buildings[i];
            if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_WAREHOUSE) {
                continue;
            }
            if (!b->has_road_access) {
                continue;
            }
            if (b->road_network_id != dock->road_network_id) {
                continue;
            }
            int distance_penalty = 32;
            struct building_t *space = b;
            for (int s = 0; s < 8; s++) {
                space = &all_buildings[space->next_part_building_id];
                if (space->id && space->subtype.warehouse_resource_id == resource && space->loads_stored > 0) {
                    distance_penalty--;
                }
            }
            if (distance_penalty < 32) {
                int distance = calc_maximum_distance(b->x, b->y, x, y);
                // prefer fuller warehouse
                distance += distance_penalty;
                if (distance < min_distance) {
                    min_distance = distance;
                    min_building_id = i;
                }
            }
        }
        if (min_building_id) {
            struct building_t *min = &all_buildings[min_building_id];
            if (map_has_road_access(min->x, min->y, 3, &tile) && min->has_road_access == 1) {
                tile.x = min->x;
                tile.y = min->y;
            }
        }
    }
    if (!min_building_id) {
        return 0;
    }
    ship->trader_amount_bought++;
    f->destination_building_id = min_building_id;
    f->action_state = FIGURE_ACTION_DOCKER_EXPORT_GOING_TO_WAREHOUSE;
    f->wait_ticks = 0;
    f->destination_x = tile.x;
    f->destination_y = tile.y;
    f->resource_id = resource;
    return 1;
}

static void generate_protestor(struct building_t *b)
{
    city_data.sentiment.protesters++;
    if (b->house_criminal_active < 1) {
        b->house_criminal_active = 1;
        int x_road, y_road;
        if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
            struct figure_t *f = figure_create(FIGURE_PROTESTER, x_road, y_road, DIR_4_BOTTOM);
            f->is_targetable = 1;
            f->terrain_usage = TERRAIN_USAGE_ROADS;
            f->wait_ticks = 10 + (b->house_figure_generation_delay & 0xf);
            city_data.ratings.peace_num_criminals++;
        }
    }
}

static void generate_mugger(struct building_t *b)
{
    city_data.sentiment.criminals++;
    if (b->house_criminal_active < 2) {
        b->house_criminal_active = 2;
        int x_road, y_road;
        if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
            struct figure_t *f = figure_create(FIGURE_CRIMINAL, x_road, y_road, DIR_4_BOTTOM);
            f->is_targetable = 1;
            f->terrain_usage = TERRAIN_USAGE_ROADS;
            f->wait_ticks = 10 + (b->house_figure_generation_delay & 0xf);
            city_data.ratings.peace_num_criminals++;
            if (city_data.finance.this_year.income.taxes > 20) {
                int money_stolen = city_data.finance.this_year.income.taxes / 4;
                if (money_stolen > 400) {
                    money_stolen = 400 - random_byte() / 2;
                }
                city_message_post(1, MESSAGE_THEFT, money_stolen, f->grid_offset);
                city_data.finance.stolen_this_year += money_stolen;
                city_finance_process_misc(money_stolen);
            }
        }
    }
}

static void remove_resource_from_warehouse(struct figure_t *f)
{
    if (figure_is_alive(f)) {
        int err = building_warehouse_remove_resource(&all_buildings[f->building_id], f->resource_id, 1);
        if (err) {
            figure_delete(f);
        }
    }
}

static void building_workshop_add_raw_material(struct building_t *b)
{
    if (b->id > 0 && building_is_workshop(b->type)) {
        b->loads_stored++; // BUG: any raw material accepted
    }
}

static void reroute_cartpusher(struct figure_t *f)
{
    figure_route_remove(f);
    if (terrain_land_citizen.items[f->grid_offset] != CITIZEN_2_PASSABLE_TERRAIN) {
        f->action_state = FIGURE_ACTION_CARTPUSHER_INITIAL;
    }
    f->wait_ticks = 0;
}

static void update_image_cartpusher(struct figure_t *f)
{
    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);

    f->image_id = image_group(GROUP_FIGURE_CARTPUSHER) + dir + 8 * f->image_offset;
    if (f->cart_image_id) {
        f->cart_image_id += dir;
        figure_image_set_cart_offset(f, dir);
        if (f->loads_sold_or_carrying >= 8) {
            f->y_offset_cart -= 40;
        }
    }
}

static int building_granary_for_storing(int x, int y, int resource, int road_network_id, int force_on_stockpile, int *understaffed, struct map_point_t *dst)
{
    if (scenario.rome_supplies_wheat) {
        return 0;
    }
    if (!resource_is_food(resource)) {
        return 0;
    }
    if (city_data.resource.stockpiled[resource] && !force_on_stockpile) {
        return 0;
    }
    int min_dist = INFINITE;
    int min_building_id = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_GRANARY) {
            continue;
        }
        if (!b->has_road_access || b->road_network_id != road_network_id) {
            continue;
        }
        if (calc_percentage(b->num_workers, building_properties[b->type].n_laborers) < 100) {
            if (understaffed) {
                *understaffed += 1;
            }
            continue;
        }
        struct building_storage_t *s = building_storage_get(b->storage_id);
        if (s->resource_state[resource] == BUILDING_STORAGE_STATE_NOT_ACCEPTING || s->empty_all) {
            continue;
        }
        if (b->data.granary.resource_stored[RESOURCE_NONE] >= ONE_LOAD) {
            // there is room
            int dist = calc_maximum_distance(b->x + 1, b->y + 1, x, y);
            if (dist < min_dist) {
                min_dist = dist;
                min_building_id = i;
            }
        }
    }
    // deliver to center of granary
    struct building_t *min = &all_buildings[min_building_id];
    dst->x = min->x + 1;
    dst->y = min->y + 1;
    return min_building_id;
}

static int building_get_workshop_for_raw_material_with_room(int x, int y, int resource, int road_network_id, struct map_point_t *dst)
{
    if (city_data.resource.stockpiled[resource]) {
        return 0;
    }
    int output_type = resource_to_workshop_type(resource);
    if (output_type == WORKSHOP_NONE) {
        return 0;
    }
    int min_dist = INFINITE;
    struct building_t *min_building = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !building_is_workshop(b->type)) {
            continue;
        }
        if (!b->has_road_access) {
            continue;
        }
        if (b->subtype.workshop_type == output_type && b->road_network_id == road_network_id && b->loads_stored < 2) {
            int dist = calc_maximum_distance(b->x, b->y, x, y);
            if (b->loads_stored > 0) {
                dist += 20;
            }
            if (dist < min_dist) {
                min_dist = dist;
                min_building = b;
            }
        }
    }
    if (min_building) {
        dst->x = min_building->road_access_x;
        dst->y = min_building->road_access_y;
        return min_building->id;
    }
    return 0;
}

static void set_destination(struct figure_t *f, int action, int building_id, int x_dst, int y_dst)
{
    f->destination_building_id = building_id;
    f->action_state = action;
    f->wait_ticks = 0;
    f->destination_x = x_dst;
    f->destination_y = y_dst;
}

static void set_cart_graphic(struct figure_t *f)
{
    f->cart_image_id = EMPTY_CART_IMG_ID + resource_images[f->resource_id].cart_img_id + resource_image_offset(f->resource_id, RESOURCE_IMAGE_CART);
}

void game_run(void)
{
    uint32_t now_millis = time_get_millis();
    for (int i = 0; i < MAX_ANIM_TIMERS; i++) {
        timers[i].should_update = 0;
    }
    unsigned int delay_millis = 0;
    for (int i = 0; i < MAX_ANIM_TIMERS; i++) {
        if (now_millis - timers[i].last_update >= delay_millis) {
            timers[i].should_update = 1;
            timers[i].last_update = now_millis;
        }
        delay_millis += 20;
    }
    int last_check_was_valid = speed_data.last_check_was_valid;
    speed_data.last_check_was_valid = 0;
    if (game_state_is_paused()) {
        return;
    }
    int millis_per_tick = 1;
    switch (window_get_id()) {
        default:
            return;
        case WINDOW_CITY:
        case WINDOW_CITY_MILITARY:
        case WINDOW_SLIDING_SIDEBAR:
        case WINDOW_OVERLAY_MENU:
        case WINDOW_MILITARY_MENU:
        case WINDOW_BUILD_MENU:
        {
            int speed = setting_game_speed();
            if (speed < 10) {
                return;
            } else if (speed <= 100) {
                millis_per_tick = MILLIS_PER_TICK_PER_SPEED[speed / 10];
            } else {
                if (speed > 500) {
                    speed = 500;
                }
                millis_per_tick = MILLIS_PER_HYPER_SPEED[speed / 100];
            }
            break;
        }
        case WINDOW_EDITOR_MAP:
            millis_per_tick = MILLIS_PER_TICK_PER_SPEED[7]; // 70%, nice speed for flag animations
            break;
    }
    uint32_t now = time_get_millis();
    uint32_t diff = now - speed_data.last_update;
    speed_data.last_check_was_valid = 1;
    int num_ticks;
    if (last_check_was_valid) {
        int ticks = diff / millis_per_tick;
        if (!ticks) {
            return;
        } else if (ticks <= MAX_TICKS_PER_FRAME) {
            speed_data.last_update = now - (diff % millis_per_tick); // account for left-over millis in this frame
            num_ticks = ticks;
        } else {
            speed_data.last_update = now;
            num_ticks = MAX_TICKS_PER_FRAME;
        }
    } else {
        // returning to map from another window or pause: always force a tick
        speed_data.last_update = now;
        num_ticks = 1;
    }
    for (int i = 0; i < num_ticks; i++) {
        if (editor_is_active()) {
            random_generate_next(); // update random to randomize native huts
            for (int j = 1; j < MAX_FIGURES; j++) {
                struct figure_t *f = &figures[j];
                if (f->in_use && f->type == FIGURE_MAP_FLAG) {
                    figure_editor_flag_action(f);
                }
            }
            return;
        }
        random_generate_next();
        if (game_can_undo()) {
            int earthquake_in_progress = 0;
            for (int j = 0; j < MAX_EARTHQUAKES; j++) {
                if (scenario.earthquakes[j].state == EVENT_IN_PROGRESS) {
                    earthquake_in_progress = 1;
                    break;
                }
            }
            if (undo_data.timeout_ticks <= 0 || earthquake_in_progress) {
                undo_data.available = 0;
                clear_buildings();
                window_invalidate();
            } else {
                undo_data.timeout_ticks--;
                if (undo_data.type != BUILDING_CLEAR_LAND
                && undo_data.type != BUILDING_AQUEDUCT
                && undo_data.type != BUILDING_ROAD
                && undo_data.type != BUILDING_WALL
                && undo_data.type != BUILDING_LOW_BRIDGE
                && undo_data.type != BUILDING_SHIP_BRIDGE
                && undo_data.type != BUILDING_PLAZA
                && undo_data.type != BUILDING_GARDENS) {
                    if (undo_data.num_buildings <= 0) {
                        undo_data.available = 0;
                        window_invalidate();
                    } else {
                        int cont = 1;
                        if (undo_data.type == BUILDING_HOUSE_VACANT_LOT) {
                            for (int j = 0; j < undo_data.num_buildings; j++) {
                                if (undo_data.buildings[j].id && all_buildings[undo_data.buildings[j].id].house_population) {
                                    // no undo on a new house where people moved in
                                    undo_data.available = 0;
                                    window_invalidate();
                                    cont = 0;
                                    break;
                                }
                            }
                        }
                        if (cont) {
                            for (int j = 0; j < undo_data.num_buildings; j++) {
                                if (undo_data.buildings[j].id) {
                                    struct building_t *b = &all_buildings[undo_data.buildings[j].id];
                                    if (b->state == BUILDING_STATE_UNDO ||
                                        b->state == BUILDING_STATE_RUBBLE ||
                                        b->state == BUILDING_STATE_DELETED_BY_GAME) {
                                        undo_data.available = 0;
                                        window_invalidate();
                                        break;
                                    } else {
                                        if (b->type != undo_data.buildings[j].type || b->grid_offset != undo_data.buildings[j].grid_offset) {
                                            undo_data.available = 0;
                                            window_invalidate();
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        // NB: these ticks are noop:
        // 0, 9, 11, 13, 14, 15, 26, 29, 41, 42, 47
        int total_houses;
        const int *houses;
        int recalculate_terrain = 0;
        switch (game_time_tick()) {
            case 1: city_gods_calculate_moods(1); break;
            case 2: update_music(0); break;
            case 3: widget_minimap_invalidate(); break;
            case 4:
                // update debt state
                if (city_data.finance.treasury >= 0) {
                    city_data.emperor.months_in_debt = -1;
                } else {
                    if (city_data.emperor.debt_state == 0) {
                        // provide bailout
                        int rescue_loan = scenario.rescue_loan;
                        city_finance_process_donation(rescue_loan);
                        city_finance_calculate_totals();

                        city_data.emperor.debt_state = 1;
                        city_data.emperor.months_in_debt = 0;
                        city_message_post(1, MESSAGE_CITY_IN_DEBT, 0, 0);
                        if (city_data.ratings.prosperity >= 3) {
                            city_data.ratings.prosperity -= 3;
                        }
                        city_data.ratings.prosperity_explanation = 8;
                    } else if (city_data.emperor.debt_state == 1) {
                        city_data.emperor.debt_state = 2;
                        city_data.emperor.months_in_debt = 0;
                        city_message_post(1, MESSAGE_CITY_IN_DEBT_AGAIN, 0, 0);
                        city_data.ratings.favor = calc_bound(city_data.ratings.favor - 5, 0, 100);
                    } else if (city_data.emperor.debt_state == 2) {
                        if (city_data.emperor.months_in_debt == -1) {
                            city_message_post(1, MESSAGE_CITY_IN_DEBT_AGAIN, 0, 0);
                            city_data.emperor.months_in_debt = 0;
                        }
                        if (game_time_day() == 0) {
                            city_data.emperor.months_in_debt++;
                        }
                        if (city_data.emperor.months_in_debt >= 12) {
                            city_data.emperor.debt_state = 3;
                            city_data.emperor.months_in_debt = 0;
                            if (!city_data.figure.imperial_soldiers) {
                                city_message_post(1, MESSAGE_CITY_STILL_IN_DEBT, 0, 0);
                                city_data.ratings.favor = calc_bound(city_data.ratings.favor - 10, 0, 100);
                            }
                        }
                    } else if (city_data.emperor.debt_state == 3) {
                        if (city_data.emperor.months_in_debt == -1) {
                            city_message_post(1, MESSAGE_CITY_STILL_IN_DEBT, 0, 0);
                            city_data.emperor.months_in_debt = 0;
                        }
                        if (game_time_day() == 0) {
                            city_data.emperor.months_in_debt++;
                        }
                        if (city_data.emperor.months_in_debt >= 12) {
                            city_data.emperor.debt_state = 4;
                            city_data.emperor.months_in_debt = 0;
                            if (!city_data.figure.imperial_soldiers) {
                                if (city_data.ratings.favor > 10) {
                                    city_data.ratings.favor = 10;
                                }
                            }
                        }
                    }
                }
                // process caesar invasion
                if (city_data.figure.imperial_soldiers && !city_data.emperor.invasion.from_editor) {
                    // caesar invasion in progress
                    city_data.emperor.invasion.duration_day_countdown--;
                    if (city_data.ratings.favor >= 35 && city_data.emperor.invasion.duration_day_countdown < 176) {
                        // pause legions
                        for (int j = 0; j < MAX_ENEMY_FORMATIONS; j++) {
                            if (enemy_formations[j].in_use == 1 && enemy_formations[j].figure_type == FIGURE_ENEMY_CAESAR_LEGIONARY) {
                                enemy_formations[j].wait_ticks_movement = 0;
                            }
                        }
                    } else if (city_data.ratings.favor >= 22) {
                        if (city_data.emperor.invasion.duration_day_countdown > 0) {
                            // retreat
                            for (int j = 0; j < MAX_ENEMY_FORMATIONS; j++) {
                                if (enemy_formations[j].in_use && enemy_formations[j].figure_type == FIGURE_ENEMY_CAESAR_LEGIONARY) {
                                    enemy_formations[j].morale = 0;
                                }
                            }
                            if (!city_data.emperor.invasion.retreat_message_shown) {
                                city_data.emperor.invasion.retreat_message_shown = 1;
                                city_message_post(1, MESSAGE_CAESAR_ARMY_RETREAT, 0, 0);
                            }
                        } else if (city_data.emperor.invasion.duration_day_countdown == 0) {
                            // a year has passed (11 months), siege goes on
                            city_message_post(1, MESSAGE_CAESAR_ARMY_CONTINUE, 0, 0);
                        }
                    }
                } else if (city_data.emperor.invasion.soldiers_killed && city_data.emperor.invasion.soldiers_killed >= city_data.emperor.invasion.size) {
                    // player defeated caesar army
                    city_data.emperor.invasion.size = 0;
                    city_data.emperor.invasion.soldiers_killed = 0;
                    if (!city_data.emperor.invasion.from_editor) {
                        if (city_data.ratings.favor < 35) {
                            city_data.ratings.favor = calc_bound(city_data.ratings.favor + 10, 0, 100);
                            if (city_data.emperor.invasion.count < 2) {
                                city_message_post(1, MESSAGE_CAESAR_RESPECT_1, 0, 0);
                            } else if (city_data.emperor.invasion.count < 3) {
                                city_message_post(1, MESSAGE_CAESAR_RESPECT_2, 0, 0);
                            } else {
                                city_message_post(1, MESSAGE_CAESAR_RESPECT_3, 0, 0);
                            }
                        }
                    }
                    if (city_data.emperor.invasion.from_editor) {
                        city_data.emperor.invasion.from_editor = 0;
                    }
                } else if (city_data.emperor.invasion.days_until_invasion <= 0) {
                    if (city_data.ratings.favor <= 10) {
                        // warn player that caesar is angry and will invade in a year
                        city_data.emperor.invasion.warnings_given++;
                        city_data.emperor.invasion.days_until_invasion = 192;
                        if (city_data.emperor.invasion.warnings_given <= 1) {
                            city_message_post(1, MESSAGE_CAESAR_WRATH, 0, 0);
                        }
                    }
                } else {
                    city_data.emperor.invasion.days_until_invasion--;
                    if (city_data.emperor.invasion.days_until_invasion == 0) {
                        // invade!
                        int size;
                        if (city_data.emperor.invasion.count == 0) {
                            size = 32;
                        } else if (city_data.emperor.invasion.count == 1) {
                            size = 64;
                        } else if (city_data.emperor.invasion.count == 2) {
                            size = 96;
                        } else {
                            size = 160;
                        }
                        if (start_invasion_by_caesar(size)) {
                            city_data.emperor.invasion.count++;
                            city_data.emperor.invasion.duration_day_countdown = 192;
                            city_data.emperor.invasion.retreat_message_shown = 0;
                            city_data.emperor.invasion.size = size;
                            city_data.emperor.invasion.soldiers_killed = 0;
                        }
                    }
                }
                break;
            case 5: formation_update_all(); break;
            case 6: map_natives_check_land(); break;
            case 7: map_road_network_update(); break;
            case 8: building_granaries_calculate_stocks(); break;
            case 10: building_update_highest_id(); break;
            case 12: // decay houses services coverage
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_UNUSED && b->type != BUILDING_TOWER) {
                        if (b->houses_covered <= 1) {
                            b->houses_covered = 0;
                        } else {
                            b->houses_covered--;
                        }
                    }
                }
                break;
            case 16: city_resource_calculate_warehouse_stocks(); break;
            case 17: city_resource_calculate_food_stocks_and_supply_wheat(); break;
            case 18: city_resource_calculate_workshop_stocks(); break;
            case 19: // update dock open water access
                map_routing_calculate_distances_water_boat(scenario.river_entry_point.x, scenario.river_entry_point.y);
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state == BUILDING_STATE_IN_USE && !b->house_size && b->type == BUILDING_DOCK) {
                        if (map_terrain_is_adjacent_to_open_water(b->x, b->y, 3)) {
                            b->has_water_access = 1;
                        } else {
                            b->has_water_access = 0;
                        }
                    }
                }
                break;
            case 20: // update industry production
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_IN_USE || !b->output_resource_id) {
                        continue;
                    }
                    b->data.industry.has_raw_materials = 0;
                    if (b->houses_covered <= 0 || b->num_workers <= 0) {
                        continue;
                    }
                    if (b->subtype.workshop_type && !b->loads_stored) {
                        continue;
                    }
                    if (b->data.industry.curse_days_left) {
                        b->data.industry.curse_days_left--;
                    } else {
                        if (b->data.industry.blessing_days_left) {
                            b->data.industry.blessing_days_left--;
                        }
                        if (b->type == BUILDING_MARBLE_QUARRY) {
                            b->data.industry.progress += b->num_workers / 2;
                        } else {
                            b->data.industry.progress += b->num_workers;
                        }
                        if (b->data.industry.blessing_days_left && building_is_farm(b->type)) {
                            b->data.industry.progress += b->num_workers;
                        }
                        int max = b->subtype.workshop_type ? MAX_PROGRESS_WORKSHOP : MAX_PROGRESS_RAW;
                        if (b->data.industry.progress > max) {
                            b->data.industry.progress = max;
                        }
                        if (building_is_farm(b->type)) {
                            update_farm_image(b);
                        }
                    }
                }
                break;
            case 22: // update house room
                city_data.population.total_capacity = 0;
                city_data.population.room_in_houses = 0;

                building_list_large_clear(0);
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
                        building_list_large_add(j);
                    }
                }
                total_houses = building_list_large_size();
                houses = building_list_large_items();
                for (int j = 0; j < total_houses; j++) {
                    struct building_t *b = &all_buildings[houses[j]];
                    b->house_population_room = 0;
                    int max_pop = house_properties[b->subtype.house_level].max_people;
                    if (b->house_is_merged) {
                        max_pop *= 4;
                    }
                    city_data.population.total_capacity += max_pop;
                    city_data.population.room_in_houses += max_pop - b->house_population;
                    b->house_population_room = max_pop - b->house_population;
                    if (b->house_population > b->house_highest_population) {
                        b->house_highest_population = b->house_population;
                    }
                }
                break;
            case 23: // update migration
                city_migration_update();
                city_population_yearly_update();
                int num_plebs = 0;
                int num_patricians = 0;
                total_houses = building_list_large_size();
                houses = building_list_large_items();
                for (int j = 0; j < total_houses; j++) {
                    struct building_t *b = &all_buildings[houses[j]];
                    if (b->house_population > 0) {
                        if (b->subtype.house_level >= HOUSE_SMALL_VILLA) {
                            num_patricians += b->house_population;
                        } else {
                            num_plebs += b->house_population;
                        }
                    }
                }
                city_labor_calculate_workers(num_plebs, num_patricians);
                // population messages
                if (city_data.population.population >= 500 && city_message_mark_population_shown(500)) {
                    city_message_post(1, MESSAGE_POPULATION_500, 0, 0);
                }
                if (city_data.population.population >= 1000 && city_message_mark_population_shown(1000)) {
                    city_message_post(1, MESSAGE_POPULATION_1000, 0, 0);
                }
                if (city_data.population.population >= 2000 && city_message_mark_population_shown(2000)) {
                    city_message_post(1, MESSAGE_POPULATION_2000, 0, 0);
                }
                if (city_data.population.population >= 3000 && city_message_mark_population_shown(3000)) {
                    city_message_post(1, MESSAGE_POPULATION_3000, 0, 0);
                }
                if (city_data.population.population >= 5000 && city_message_mark_population_shown(5000)) {
                    city_message_post(1, MESSAGE_POPULATION_5000, 0, 0);
                }
                if (city_data.population.population >= 10000 && city_message_mark_population_shown(10000)) {
                    city_message_post(1, MESSAGE_POPULATION_10000, 0, 0);
                }
                if (city_data.population.population >= 15000 && city_message_mark_population_shown(15000)) {
                    city_message_post(1, MESSAGE_POPULATION_15000, 0, 0);
                }
                if (city_data.population.population >= 20000 && city_message_mark_population_shown(20000)) {
                    city_message_post(1, MESSAGE_POPULATION_20000, 0, 0);
                }
                if (city_data.population.population >= 25000 && city_message_mark_population_shown(25000)) {
                    city_message_post(1, MESSAGE_POPULATION_25000, 0, 0);
                }
                break;
            case 24: // evict overcrowded
            {
                int size = building_list_large_size();
                const int *items = building_list_large_items();
                for (int j = 0; j < size; j++) {
                    struct building_t *b = &all_buildings[items[j]];
                    if (b->house_population_room < 0) {
                        int num_people_to_evict = -b->house_population_room;
                        figure_create_homeless(b->x, b->y, num_people_to_evict);
                        if (num_people_to_evict < b->house_population) {
                            b->house_population -= num_people_to_evict;
                        } else {
                            // house has been removed
                            b->state = BUILDING_STATE_UNDO;
                        }
                    }
                }
                break;
            }
            case 25: city_labor_update(); break;
            case 27: map_water_supply_update_reservoir_fountain(); break;
            case 28: // update houses water supply
                building_list_small_clear();
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_IN_USE) {
                        continue;
                    }
                    if (b->type == BUILDING_WELL) {
                        building_list_small_add(j);
                    } else if (b->house_size) {
                        b->has_water_access = 0;
                        b->has_well_access = 0;
                        if (map_terrain_exists_tile_in_area_with_type(
                            b->x, b->y, b->size, TERRAIN_FOUNTAIN_RANGE)) {
                            b->has_water_access = 1;
                        }
                    }
                }
                int total_wells = building_list_small_size();
                const int *wells = building_list_small_items();
                for (int j = 0; j < total_wells; j++) {
                    struct building_t *well = &all_buildings[wells[j]];
                    int x_min, y_min, x_max, y_max;
                    map_grid_get_area(well->x, well->y, 1, 2, &x_min, &y_min, &x_max, &y_max);

                    for (int yy = y_min; yy <= y_max; yy++) {
                        for (int xx = x_min; xx <= x_max; xx++) {
                            int building_id = map_building_at(map_grid_offset(xx, yy));
                            if (building_id) {
                                all_buildings[building_id].has_well_access = 1;
                            }
                        }
                    }
                }
                break;
            case 30: widget_minimap_invalidate(); break;
            case 31: building_figure_generate(); break;
            case 32: city_trade_update(); break;
            case 33: building_count_update(); city_culture_update_coverage(); break;
            case 34: // distribute treasury
                int units = 5 * building_count_active(BUILDING_SENATE) + building_count_active(BUILDING_FORUM);
                int amount_per_unit;
                int remainder;
                if (city_data.finance.treasury > 0 && units > 0) {
                    amount_per_unit = city_data.finance.treasury / units;
                    remainder = city_data.finance.treasury - units * amount_per_unit;
                } else {
                    amount_per_unit = 0;
                    remainder = 0;
                }

                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_IN_USE || b->house_size) {
                        continue;
                    }
                    b->tax_income_or_storage = 0;
                    if (b->num_workers <= 0) {
                        continue;
                    }
                    switch (b->type) {
                        // ordered based on importance: most important gets the remainder
                        case BUILDING_SENATE:
                            b->tax_income_or_storage = 5 * amount_per_unit + remainder;
                            remainder = 0;
                            break;
                        case BUILDING_FORUM:
                            if (remainder && !building_count_active(BUILDING_SENATE)) {
                                b->tax_income_or_storage = amount_per_unit + remainder;
                                remainder = 0;
                            } else {
                                b->tax_income_or_storage = amount_per_unit;
                            }
                            break;
                    }
                }
                break;
            case 35: // decay culture 
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_IN_USE || !b->house_size) {
                        continue;
                    }
                    decay(&b->data.house.theater);
                    decay(&b->data.house.amphitheater_actor);
                    decay(&b->data.house.amphitheater_gladiator);
                    decay(&b->data.house.colosseum_gladiator);
                    decay(&b->data.house.colosseum_lion);
                    decay(&b->data.house.hippodrome);
                    decay(&b->data.house.school);
                    decay(&b->data.house.library);
                    decay(&b->data.house.academy);
                    decay(&b->data.house.barber);
                    decay(&b->data.house.clinic);
                    decay(&b->data.house.bathhouse);
                    decay(&b->data.house.hospital);
                    decay(&b->data.house.temple_ceres);
                    decay(&b->data.house.temple_neptune);
                    decay(&b->data.house.temple_mercury);
                    decay(&b->data.house.temple_mars);
                    decay(&b->data.house.temple_venus);
                }
                break;
            case 36: // calculate culture aggregates
            {
                int base_entertainment = city_culture_coverage_average_entertainment() / 5;
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_IN_USE || !b->house_size) {
                        continue;
                    }
                    // entertainment
                    b->data.house.entertainment = base_entertainment;
                    if (b->data.house.theater) {
                        b->data.house.entertainment += 10;
                    }
                    if (b->data.house.amphitheater_actor) {
                        if (b->data.house.amphitheater_gladiator) {
                            b->data.house.entertainment += 15;
                        } else {
                            b->data.house.entertainment += 10;
                        }
                    }
                    if (b->data.house.colosseum_gladiator) {
                        if (b->data.house.colosseum_lion) {
                            b->data.house.entertainment += 25;
                        } else {
                            b->data.house.entertainment += 15;
                        }
                    }
                    if (b->data.house.hippodrome) {
                        b->data.house.entertainment += 30;
                    }
                    // education
                    b->data.house.education = 0;
                    // release build mingw doesn't like school || library for some reason
                    if (b->data.house.school) {
                        b->data.house.education = 1;
                    }
                    if (b->data.house.library) {
                        b->data.house.education = 1;
                    }
                    if (b->data.house.school && b->data.house.library) {
                        b->data.house.education = 2;
                        if (b->data.house.academy) {
                            b->data.house.education = 3;
                        }
                    }
                    // religion
                    b->data.house.num_gods = 0;
                    if (b->data.house.temple_ceres) {
                        ++b->data.house.num_gods;
                    }
                    if (b->data.house.temple_neptune) {
                        ++b->data.house.num_gods;
                    }
                    if (b->data.house.temple_mercury) {
                        ++b->data.house.num_gods;
                    }
                    if (b->data.house.temple_mars) {
                        ++b->data.house.num_gods;
                    }
                    if (b->data.house.temple_venus) {
                        ++b->data.house.num_gods;
                    }
                    // health
                    b->data.house.health = 0;
                    if (b->data.house.clinic) {
                        ++b->data.house.health;
                    }
                    if (b->data.house.hospital) {
                        ++b->data.house.health;
                    }
                }
                break;
            }
            case 37: map_desirability_update(); break;
            case 38: // update desirability
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_IN_USE) {
                        continue;
                    }
                    b->desirability = map_desirability_get_max(b->x, b->y, b->size);
                }
                break;
            case 39: // evolve/devolve houses, consume goods
                city_data.houses.missing.fountain = 0;
                city_data.houses.missing.well = 0;
                city_data.houses.missing.entertainment = 0;
                city_data.houses.missing.more_entertainment = 0;
                city_data.houses.missing.education = 0;
                city_data.houses.missing.more_education = 0;
                city_data.houses.missing.religion = 0;
                city_data.houses.missing.second_religion = 0;
                city_data.houses.missing.third_religion = 0;
                city_data.houses.missing.barber = 0;
                city_data.houses.missing.bathhouse = 0;
                city_data.houses.missing.clinic = 0;
                city_data.houses.missing.hospital = 0;
                city_data.houses.missing.food = 0;
                // NB: second_wine purposely not cleared
                city_data.houses.requiring.school = 0;
                city_data.houses.requiring.library = 0;
                city_data.houses.requiring.barber = 0;
                city_data.houses.requiring.bathhouse = 0;
                city_data.houses.requiring.clinic = 0;
                city_data.houses.requiring.religion = 0;
                int has_expanded = 0;
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state == BUILDING_STATE_IN_USE && building_is_house(b->type) && b->type != BUILDING_HOUSE_VACANT_LOT) {
                        int calc_grid_offset = map_grid_offset(b->x, b->y);
                        b->data.house.no_space_to_expand = 0;
                        if (b->grid_offset != calc_grid_offset || map_building_at(b->grid_offset) != b->id) {
                            int map_width, map_height;
                            map_grid_size(&map_width, &map_height);
                            for (int y = 0; y < map_height; y++) {
                                for (int x = 0; x < map_width; x++) {
                                    int grid_offset = map_grid_offset(x, y);
                                    if (map_building_at(grid_offset) == b->id) {
                                        b->grid_offset = grid_offset;
                                        b->x = map_grid_offset_to_x(grid_offset);
                                        b->y = map_grid_offset_to_y(grid_offset);
                                        building_totals_add_corrupted_house(0);
                                        return;
                                    }
                                }
                            }
                            building_totals_add_corrupted_house(1);
                            b->state = BUILDING_STATE_RUBBLE;
                        }
                        has_expanded |= evolve_callback[b->type - BUILDING_HOUSE_SMALL_TENT](b, &city_data.houses);
                        if (game_time_day() == 0 || game_time_day() == 7) {
                            consume_resource(b, INVENTORY_POTTERY, house_properties[b->subtype.house_level].pottery);
                            consume_resource(b, INVENTORY_FURNITURE, house_properties[b->subtype.house_level].furniture);
                            consume_resource(b, INVENTORY_OIL, house_properties[b->subtype.house_level].oil);
                            consume_resource(b, INVENTORY_WINE, house_properties[b->subtype.house_level].wine);
                        }
                    }
                }
                if (has_expanded) {
                    map_routing_update_land();
                }
                break;
            case 40: building_update_state(); break;
            case 43: // update burning ruins
                building_list_burning_clear();
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_BURNING_RUIN) {
                        continue;
                    }
                    if (b->fire_duration < 0) {
                        b->fire_duration = 0;
                    }
                    b->fire_duration++;
                    if (b->fire_duration > 32) {
                        game_undo_disable();
                        b->state = BUILDING_STATE_RUBBLE;
                        map_building_tiles_set_rubble(j, b->x, b->y, b->size);
                        recalculate_terrain = 1;
                        continue;
                    }
                    if (b->ruin_has_plague) {
                        continue;
                    }
                    building_list_burning_add(j);
                    if (scenario.climate == CLIMATE_DESERT) {
                        if (b->fire_duration & 3) { // check spread every 4 ticks
                            continue;
                        }
                    } else {
                        if (b->fire_duration & 7) { // check spread every 8 ticks
                            continue;
                        }
                    }
                    if ((b->house_figure_generation_delay & 3) != (random_byte() & 3)) {
                        continue;
                    }
                    int dir1 = fire_spread_direction - 1;
                    if (dir1 < 0) dir1 = 7;
                    int dir2 = fire_spread_direction + 1;
                    if (dir2 > 7) dir2 = 0;

                    int grid_offset = b->grid_offset;
                    int next_building_id = map_building_at(grid_offset + map_grid_direction_delta(fire_spread_direction));
                    if (next_building_id && !all_buildings[next_building_id].fire_proof) {
                        building_destroy_by_fire(&all_buildings[next_building_id]);
                        play_sound_effect(SOUND_EFFECT_EXPLOSION);
                        recalculate_terrain = 1;
                    } else {
                        next_building_id = map_building_at(grid_offset + map_grid_direction_delta(dir1));
                        if (next_building_id && !all_buildings[next_building_id].fire_proof) {
                            building_destroy_by_fire(&all_buildings[next_building_id]);
                            play_sound_effect(SOUND_EFFECT_EXPLOSION);
                            recalculate_terrain = 1;
                        } else {
                            next_building_id = map_building_at(grid_offset + map_grid_direction_delta(dir2));
                            if (next_building_id && !all_buildings[next_building_id].fire_proof) {
                                building_destroy_by_fire(&all_buildings[next_building_id]);
                                play_sound_effect(SOUND_EFFECT_EXPLOSION);
                                recalculate_terrain = 1;
                            }
                        }
                    }
                }
                if (recalculate_terrain) {
                    map_routing_update_land();
                }
                break;
            case 44: // check fire/collapse
                city_data.sentiment.protesters = 0;
                city_data.sentiment.criminals = 0;
                int random_global = random_byte() & 7;
                int max_id = building_get_highest_id();
                for (int j = 1; j <= max_id; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_IN_USE || b->fire_proof) {
                        continue;
                    }
                    if (b->type == BUILDING_HIPPODROME && b->prev_part_building_id) {
                        continue;
                    }
                    int random_building = (j + map_random_get(b->grid_offset)) & 7;
                    // damage
                    b->damage_risk += random_building == random_global ? 3 : 1;
                    if (b->house_size && b->subtype.house_level <= HOUSE_LARGE_TENT) {
                        b->damage_risk = 0;
                    }
                    if (b->damage_risk > 200) {
                        city_message_apply_sound_interval(MESSAGE_CAT_COLLAPSE);
                        city_message_post_with_popup_delay(MESSAGE_CAT_COLLAPSE, MESSAGE_COLLAPSED_BUILDING, b->type, b->grid_offset);
                        game_undo_disable();
                        building_destroy_by_collapse(b);
                        recalculate_terrain = 1;
                        continue;
                    }
                    // fire
                    if (random_building == random_global) {
                        if (!b->house_size) {
                            b->fire_risk += 5;
                        } else if (b->house_population <= 0) {
                            b->fire_risk = 0;
                        } else if (b->subtype.house_level <= HOUSE_LARGE_SHACK) {
                            b->fire_risk += 10;
                        } else if (b->subtype.house_level <= HOUSE_GRAND_INSULA) {
                            b->fire_risk += 5;
                        } else {
                            b->fire_risk += 2;
                        }
                        if (scenario.climate == CLIMATE_NORTHERN) {
                            b->fire_risk = 0;
                        } else if (scenario.climate == CLIMATE_DESERT) {
                            b->fire_risk += 3;
                        }
                    }
                    if (b->fire_risk > 100) {
                        city_message_apply_sound_interval(MESSAGE_CAT_FIRE);
                        city_message_post_with_popup_delay(MESSAGE_CAT_FIRE, MESSAGE_FIRE, b->type, b->grid_offset);
                        building_destroy_by_fire(b);
                        play_sound_effect(SOUND_EFFECT_EXPLOSION);
                        recalculate_terrain = 1;
                    }
                }
                if (recalculate_terrain) {
                    map_routing_update_land();
                }
                break;
            case 45:
            {
                struct building_t *min_building = 0;
                int min_happiness = 50;
                max_id = building_get_highest_id();
                for (int k = 1; k <= max_id; k++) {
                    struct building_t *b = &all_buildings[k];
                    if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
                        if (b->sentiment.house_happiness >= 50) {
                            b->house_criminal_active = 0;
                        } else if (b->sentiment.house_happiness < min_happiness) {
                            min_happiness = b->sentiment.house_happiness;
                            min_building = b;
                        }
                    }
                }
                if (min_building) {
                    int sentiment = city_data.sentiment.value;
                    if (sentiment < 30) {
                        if (random_byte() >= sentiment + 50) {
                            if (min_happiness <= 10) {
                                int x_road, y_road;
                                if (map_closest_road_within_radius(min_building->x, min_building->y, min_building->size, 4, &x_road, &y_road)) {
                                    city_data.sentiment.criminals++;
                                    int people_in_mob;
                                    if (city_data.population.population <= 150) {
                                        people_in_mob = 1;
                                    } else if (city_data.population.population <= 300) {
                                        people_in_mob = 2;
                                    } else if (city_data.population.population <= 800) {
                                        people_in_mob = 3;
                                    } else if (city_data.population.population <= 1200) {
                                        people_in_mob = 4;
                                    } else if (city_data.population.population <= 2000) {
                                        people_in_mob = 5;
                                    } else {
                                        people_in_mob = 6;
                                    }
                                    int x_target, y_target;
                                    int target_building_id = formation_rioter_get_target_building(&x_target, &y_target);
                                    for (int k = 0; k < people_in_mob; k++) {
                                        struct figure_t *f = figure_create(FIGURE_RIOTER, x_road, y_road, DIR_4_BOTTOM);
                                        f->is_targetable = 1;
                                        f->action_state = FIGURE_ACTION_RIOTER_CREATED;
                                        f->terrain_usage = TERRAIN_USAGE_ENEMY;
                                        f->roam_length = 0;
                                        f->wait_ticks = 10 + 4 * k;
                                        if (target_building_id) {
                                            f->destination_x = x_target;
                                            f->destination_y = y_target;
                                            f->destination_building_id = target_building_id;
                                        }
                                        city_data.figure.rioters++;
                                    }
                                    destroy_on_fire(min_building, 0);
                                    city_data.ratings.peace_num_rioters++;
                                    city_data.ratings.peace_riot_cause = city_data.sentiment.low_mood_cause;
                                    city_sentiment_change_happiness(20);
                                    city_message_apply_sound_interval(MESSAGE_CAT_RIOT);
                                    city_message_post_with_popup_delay(MESSAGE_CAT_RIOT, MESSAGE_RIOT, min_building->type, map_grid_offset(x_road, y_road));
                                }
                            } else if (min_happiness < 30) {
                                generate_mugger(min_building);
                            } else if (min_happiness < 50) {
                                generate_protestor(min_building);
                            }
                        }
                    } else if (sentiment < 60) {
                        if (random_byte() >= sentiment + 40) {
                            if (min_happiness < 30) {
                                generate_mugger(min_building);
                            } else if (min_happiness < 50) {
                                generate_protestor(min_building);
                            }
                        }
                    } else {
                        if (random_byte() >= sentiment + 20) {
                            if (min_happiness < 50) {
                                generate_protestor(min_building);
                            }
                        }
                    }
                }
            }
            break;
            case 46: // update wheat production
                if (scenario.climate != CLIMATE_NORTHERN) {
                    for (int j = 1; j < MAX_BUILDINGS; j++) {
                        struct building_t *b = &all_buildings[j];
                        if (b->state != BUILDING_STATE_IN_USE || !b->output_resource_id) {
                            continue;
                        }
                        if (b->houses_covered <= 0 || b->num_workers <= 0) {
                            continue;
                        }
                        if (b->type == BUILDING_WHEAT_FARM && !b->data.industry.curse_days_left) {
                            b->data.industry.progress += b->num_workers;
                            if (b->data.industry.blessing_days_left) {
                                b->data.industry.progress += b->num_workers;
                            }
                            if (b->data.industry.progress > MAX_PROGRESS_RAW) {
                                b->data.industry.progress = MAX_PROGRESS_RAW;
                            }
                            update_farm_image(b);
                        }
                    }
                }
                break;
            case 48: // decay tax collector
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state == BUILDING_STATE_IN_USE && b->house_tax_coverage) {
                        b->house_tax_coverage--;
                    }
                }
                break;
            case 49: city_culture_calculate(); break;
        }
        time_data.tick++;
        if (time_data.tick >= 50) {
            time_data.tick = 0;
            time_data.total_days++;
            time_data.day++;
            if (time_data.day >= 16) { // advance month
                time_data.day = 0;
                city_data.migration.newcomers = 0;
                city_health_update();
                process_random_event();
                // collect monthly taxes
                city_data.taxes.taxed_plebs = 0;
                city_data.taxes.taxed_patricians = 0;
                city_data.taxes.untaxed_plebs = 0;
                city_data.taxes.untaxed_patricians = 0;
                city_data.taxes.monthly.uncollected_plebs = 0;
                city_data.taxes.monthly.collected_plebs = 0;
                city_data.taxes.monthly.uncollected_patricians = 0;
                city_data.taxes.monthly.collected_patricians = 0;
                for (int j = 0; j < MAX_HOUSE_LEVELS; j++) {
                    city_data.population.at_level[j] = 0;
                }
                for (int j = 1; j < MAX_BUILDINGS; j++) {
                    struct building_t *b = &all_buildings[j];
                    if (b->state != BUILDING_STATE_IN_USE || !b->house_size) {
                        continue;
                    }
                    int is_patrician = b->subtype.house_level >= HOUSE_SMALL_VILLA;
                    int population = b->house_population;
                    city_data.population.at_level[b->subtype.house_level] += population;
                    int tax = population * house_properties[b->subtype.house_level].tax_multiplier;
                    if (b->house_tax_coverage) {
                        if (is_patrician) {
                            city_data.taxes.taxed_patricians += population;
                            city_data.taxes.monthly.collected_patricians += tax;
                        } else {
                            city_data.taxes.taxed_plebs += population;
                            city_data.taxes.monthly.collected_plebs += tax;
                        }
                        b->tax_income_or_storage += tax;
                    } else {
                        if (is_patrician) {
                            city_data.taxes.untaxed_patricians += population;
                            city_data.taxes.monthly.uncollected_patricians += tax;
                        } else {
                            city_data.taxes.untaxed_plebs += population;
                            city_data.taxes.monthly.uncollected_plebs += tax;
                        }
                    }
                }
                int collected_patricians = calc_adjust_with_percentage(
                    city_data.taxes.monthly.collected_patricians / 2,
                    city_data.finance.tax_percentage);
                int collected_plebs = calc_adjust_with_percentage(
                    city_data.taxes.monthly.collected_plebs / 2,
                    city_data.finance.tax_percentage);
                int collected_total = collected_patricians + collected_plebs;
                city_data.taxes.yearly.collected_patricians += collected_patricians;
                city_data.taxes.yearly.collected_plebs += collected_plebs;
                city_data.taxes.yearly.uncollected_patricians += calc_adjust_with_percentage(
                    city_data.taxes.monthly.uncollected_patricians / 2,
                    city_data.finance.tax_percentage);
                city_data.taxes.yearly.uncollected_plebs += calc_adjust_with_percentage(
                    city_data.taxes.monthly.uncollected_plebs / 2,
                    city_data.finance.tax_percentage);

                city_data.finance.treasury += collected_total;
                int total_patricians = city_data.taxes.taxed_patricians + city_data.taxes.untaxed_patricians;
                int total_plebs = city_data.taxes.taxed_plebs + city_data.taxes.untaxed_plebs;
                city_data.taxes.percentage_taxed_patricians = calc_percentage(city_data.taxes.taxed_patricians, total_patricians);
                city_data.taxes.percentage_taxed_plebs = calc_percentage(city_data.taxes.taxed_plebs, total_plebs);
                city_data.taxes.percentage_taxed_people = calc_percentage(
                    city_data.taxes.taxed_patricians + city_data.taxes.taxed_plebs,
                    total_patricians + total_plebs);
                // pay monthly wages
                int wages = city_data.labor.wages * city_data.labor.workers_employed / 10 / 12;
                city_data.finance.treasury -= wages;
                city_data.finance.wages_so_far += wages;
                city_data.finance.wage_rate_paid_this_year += city_data.labor.wages;
                // pay monthly interest
                if (city_data.finance.treasury < 0) {
                    int interest = calc_adjust_with_percentage(-city_data.finance.treasury, 10) / 12;
                    city_data.finance.treasury -= interest;
                    city_data.finance.interest_so_far += interest;
                }
                // pay monthly salary
                if (city_finance_can_afford(city_data.emperor.salary_amount)) {
                    city_data.finance.salary_so_far += city_data.emperor.salary_amount;
                    city_data.emperor.personal_savings += city_data.emperor.salary_amount;
                    city_data.finance.treasury -= city_data.emperor.salary_amount;
                }
                city_resource_consume_food();
                city_victory_update_months_to_govern();
                update_legion_morale_monthly();
                city_message_decrease_delays();

                map_tiles_update_all_roads();
                map_tiles_update_all_water();
                map_routing_update_land_citizen();
                city_message_sort_and_compact();

                time_data.month++;
                if (time_data.month >= 12) { // advance year
                    time_data.month = 0;
                    game_undo_disable();
                    time_data.year++;
                    process_empire_expansion();
                    city_population_request_yearly_update();
                    // reset taxes
                    city_data.finance.last_year.income.taxes = city_data.taxes.yearly.collected_plebs + city_data.taxes.yearly.collected_patricians;
                    city_data.taxes.yearly.collected_plebs = 0;
                    city_data.taxes.yearly.collected_patricians = 0;
                    city_data.taxes.yearly.uncollected_plebs = 0;
                    city_data.taxes.yearly.uncollected_patricians = 0;
                    // reset tax income in building list
                    for (int j = 1; j < MAX_BUILDINGS; j++) {
                        struct building_t *b = &all_buildings[j];
                        if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
                            b->tax_income_or_storage = 0;
                        }
                    }
                    // copy amounts to last year
                    struct finance_overview_t *last_year = &city_data.finance.last_year;
                    struct finance_overview_t *this_year = &city_data.finance.this_year;
                    // wages
                    last_year->expenses.wages = city_data.finance.wages_so_far;
                    city_data.finance.wages_so_far = 0;
                    city_data.finance.wage_rate_paid_last_year = city_data.finance.wage_rate_paid_this_year;
                    city_data.finance.wage_rate_paid_this_year = 0;
                    // import/export
                    last_year->income.exports = this_year->income.exports;
                    this_year->income.exports = 0;
                    last_year->expenses.imports = this_year->expenses.imports;
                    this_year->expenses.imports = 0;
                    // construction
                    last_year->expenses.construction = this_year->expenses.construction;
                    this_year->expenses.construction = 0;
                    // interest
                    last_year->expenses.interest = city_data.finance.interest_so_far;
                    city_data.finance.interest_so_far = 0;
                    // salary
                    city_data.finance.last_year.expenses.salary = city_data.finance.salary_so_far;
                    city_data.finance.salary_so_far = 0;
                    // sundries
                    last_year->expenses.sundries = this_year->expenses.sundries;
                    this_year->expenses.sundries = 0;
                    city_data.finance.stolen_last_year = city_data.finance.stolen_this_year;
                    city_data.finance.stolen_this_year = 0;
                    // donations
                    last_year->income.donated = this_year->income.donated;
                    this_year->income.donated = 0;
                    // pay tribute
                    int income =
                        last_year->income.donated +
                        last_year->income.taxes +
                        last_year->income.exports;
                    int expenses =
                        last_year->expenses.sundries +
                        last_year->expenses.salary +
                        last_year->expenses.interest +
                        last_year->expenses.construction +
                        last_year->expenses.wages +
                        last_year->expenses.imports;
                    city_data.finance.tribute_not_paid_last_year = 0;
                    if (city_data.finance.treasury <= 0) {
                        // city is in debt
                        city_data.finance.tribute_not_paid_last_year = 1;
                        city_data.finance.tribute_not_paid_total_years++;
                        last_year->expenses.tribute = 0;
                    } else if (income <= expenses) {
                        // city made a loss: fixed tribute based on population
                        city_data.finance.tribute_not_paid_total_years = 0;
                        if (city_data.population.population > 2000) {
                            last_year->expenses.tribute = 200;
                        } else if (city_data.population.population > 1000) {
                            last_year->expenses.tribute = 100;
                        } else {
                            last_year->expenses.tribute = 0;
                        }
                    } else {
                        // city made a profit: tribute is max of: 25% of profit, fixed tribute based on population
                        city_data.finance.tribute_not_paid_total_years = 0;
                        if (city_data.population.population > 5000) {
                            last_year->expenses.tribute = 500;
                        } else if (city_data.population.population > 3000) {
                            last_year->expenses.tribute = 400;
                        } else if (city_data.population.population > 2000) {
                            last_year->expenses.tribute = 300;
                        } else if (city_data.population.population > 1000) {
                            last_year->expenses.tribute = 225;
                        } else if (city_data.population.population > 500) {
                            last_year->expenses.tribute = 150;
                        } else {
                            last_year->expenses.tribute = 50;
                        }
                        int pct_profit = calc_adjust_with_percentage(income - expenses, 25);
                        if (pct_profit > last_year->expenses.tribute) {
                            last_year->expenses.tribute = pct_profit;
                        }
                    }
                    city_data.finance.treasury -= last_year->expenses.tribute;
                    city_data.finance.this_year.expenses.tribute = 0;
                    last_year->balance = city_data.finance.treasury;
                    last_year->income.total = income;
                    last_year->expenses.total = last_year->expenses.tribute + expenses;
                    // reset yearly trade amounts
                    for (int j = 0; j < MAX_EMPIRE_OBJECTS; j++) {
                        if (empire_objects[j].in_use && empire_objects[j].trade_route_open) {
                            for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                                empire_objects[j].resource_bought[r] = 0;
                                empire_objects[j].resource_sold[r] = 0;
                            }
                        }
                    }
                    fire_spread_direction = random_byte() & 7;
                    city_ratings_update(1);
                    city_data.religion.neptune_double_trade_active = 0;
                } else {
                    city_ratings_update(0);
                }
                process_custom_messages();
                process_gladiator_revolt();
                process_imperial_requests();
                process_price_changes();
                process_demand_changes();
                process_invasions();
                process_distant_battles();
                // record monthly population
                city_data.population.monthly.values[city_data.population.monthly.next_index++] = city_data.population.population;
                if (city_data.population.monthly.next_index >= 2400) {
                    city_data.population.monthly.next_index = 0;
                }
                ++city_data.population.monthly.count;
                city_data.festival.months_since_festival++;
                if (city_data.festival.first_festival_effect_months) {
                    --city_data.festival.first_festival_effect_months;
                }
                if (city_data.festival.second_festival_effect_months) {
                    --city_data.festival.second_festival_effect_months;
                }
                if (city_data.festival.size) {
                    city_data.festival.months_to_go--;
                    if (city_data.festival.months_to_go <= 0) {
                        if (city_data.festival.first_festival_effect_months <= 0) {
                            city_data.festival.first_festival_effect_months = 12;
                            switch (city_data.festival.size) {
                                case FESTIVAL_SMALL: city_sentiment_change_happiness(7); break;
                                case FESTIVAL_LARGE: city_sentiment_change_happiness(9); break;
                                case FESTIVAL_GRAND: city_sentiment_change_happiness(12); break;
                            }
                        } else if (city_data.festival.second_festival_effect_months <= 0) {
                            city_data.festival.second_festival_effect_months = 12;
                            switch (city_data.festival.size) {
                                case FESTIVAL_SMALL: city_sentiment_change_happiness(2); break;
                                case FESTIVAL_LARGE: city_sentiment_change_happiness(3); break;
                                case FESTIVAL_GRAND: city_sentiment_change_happiness(5); break;
                            }
                        }
                        city_data.festival.months_since_festival = 1;
                        city_data.religion.gods[city_data.festival.god].months_since_festival = 0;
                        switch (city_data.festival.size) {
                            case FESTIVAL_SMALL: city_message_post(1, MESSAGE_SMALL_FESTIVAL, 0, 0); break;
                            case FESTIVAL_LARGE: city_message_post(1, MESSAGE_LARGE_FESTIVAL, 0, 0); break;
                            case FESTIVAL_GRAND: city_message_post(1, MESSAGE_GRAND_FESTIVAL, 0, 0); break;
                        }
                        city_data.festival.size = FESTIVAL_NONE;
                        city_data.festival.months_to_go = 0;
                    }
                }
                if (setting_monthly_autosave()) {
                    game_file_io_write_saved_game(SAVES_DIR_PATH, "autosave.sav");
                }
            }
            if (game_time_day() == 0 || game_time_day() == 8) {
                city_sentiment_update();
            }
        }
        process_earthquake();
        city_victory_check();
        city_data.entertainment.hippodrome_has_race = 0;
        for (int j = 1; j < MAX_FIGURES; j++) {
            struct figure_t *f = &figures[j];
            if (f->is_corpse) {
                figure_handle_corpse(f);
                continue;
            } else if (f->engaged_in_combat) {
                figure_combat_handle_attack(f);
                continue;
            } else if (f->is_fleeing) {
                rout_unit(f);
                continue;
            }
            if (f->in_use) {
                switch (f->type) {
                    case FIGURE_IMMIGRANT:
                    {
                        struct building_t *b = &all_buildings[f->immigrant_building_id];
                        f->cart_image_id = 0;
                        if (b->state != BUILDING_STATE_IN_USE || b->immigrant_figure_id != f->id || !b->house_size) {
                            figure_delete(f);
                            return;
                        }
                        switch (f->action_state) {
                            case FIGURE_ACTION_IMMIGRANT_CREATED:
                                f->is_invisible = 1;
                                f->wait_ticks--;
                                if (f->wait_ticks <= 0) {
                                    int x_road, y_road;
                                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                        f->action_state = FIGURE_ACTION_IMMIGRANT_ARRIVING;
                                        f->destination_x = x_road;
                                        f->destination_y = y_road;
                                        f->roam_length = 0;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_IMMIGRANT_ARRIVING:
                                f->is_invisible = 0;
                                figure_movement_move_ticks(f, 1);
                                switch (f->direction) {
                                    case DIR_FIGURE_AT_DESTINATION:
                                        f->action_state = FIGURE_ACTION_IMMIGRANT_ENTERING_HOUSE;
                                        figure_movement_set_cross_country_destination(f, b->x, b->y);
                                        f->roam_length = 0;
                                        break;
                                    case DIR_FIGURE_REROUTE:
                                        figure_route_remove(f);
                                        break;
                                    case DIR_FIGURE_LOST:
                                        b->immigrant_figure_id = 0;
                                        figure_delete(f);
                                        return;
                                }
                                break;
                            case FIGURE_ACTION_IMMIGRANT_ENTERING_HOUSE:
                                f->use_cross_country = 1;
                                f->is_invisible = 1;
                                if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                                    int max_people = house_properties[b->subtype.house_level].max_people;
                                    if (b->house_is_merged) {
                                        max_people *= 4;
                                    }
                                    int room = max_people - b->house_population;
                                    if (room < 0) {
                                        room = 0;
                                    }
                                    if (room < f->migrant_num_people) {
                                        f->migrant_num_people = room;
                                    }
                                    if (!b->house_population) {
                                        building_house_change_to(b, BUILDING_HOUSE_SMALL_TENT);
                                    }
                                    b->house_population += f->migrant_num_people;
                                    b->house_population_room = max_people - b->house_population;
                                    city_population_add(f->migrant_num_people);
                                    b->immigrant_figure_id = 0;
                                    figure_delete(f);
                                    return;
                                }
                                f->is_invisible = f->in_building_wait_ticks ? 1 : 0;
                                break;
                        }
                        update_migrant_dir_and_image(f);
                    }
                    break;
                    case FIGURE_EMIGRANT:
                        f->cart_image_id = 0;
                        switch (f->action_state) {
                            case FIGURE_ACTION_EMIGRANT_CREATED:
                                f->is_invisible = 1;
                                f->wait_ticks++;
                                if (f->wait_ticks >= 5) {
                                    int x_road, y_road;
                                    if (!map_closest_road_within_radius(f->x, f->y, 1, 5, &x_road, &y_road)) {
                                        figure_delete(f);
                                        return;
                                    }
                                    f->action_state = FIGURE_ACTION_EMIGRANT_EXITING_HOUSE;
                                    figure_movement_set_cross_country_destination(f, x_road, y_road);
                                    f->roam_length = 0;
                                }
                                break;
                            case FIGURE_ACTION_EMIGRANT_EXITING_HOUSE:
                                f->use_cross_country = 1;
                                f->is_invisible = 1;
                                if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                                    f->action_state = FIGURE_ACTION_EMIGRANT_LEAVING;
                                    f->destination_x = scenario.entry_point.x;
                                    f->destination_y = scenario.entry_point.y;
                                    f->roam_length = 0;
                                    f->progress_on_tile = 15;
                                }
                                f->is_invisible = f->in_building_wait_ticks ? 1 : 0;
                                break;
                            case FIGURE_ACTION_EMIGRANT_LEAVING:
                                f->use_cross_country = 0;
                                f->is_invisible = 0;
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                                    f->direction == DIR_FIGURE_REROUTE ||
                                    f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                        }
                        update_migrant_dir_and_image(f);
                        break;
                    case FIGURE_HOMELESS:
                        switch (f->action_state) {
                            case FIGURE_ACTION_HOMELESS_CREATED:
                                f->image_offset = 0;
                                f->wait_ticks++;
                                if (f->wait_ticks > 51) {
                                    int building_id = closest_house_with_room(f->x, f->y);
                                    if (building_id) {
                                        struct building_t *b = &all_buildings[building_id];
                                        int x_road, y_road;
                                        if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                            b->immigrant_figure_id = f->id;
                                            f->immigrant_building_id = building_id;
                                            f->action_state = FIGURE_ACTION_HOMELESS_GOING_TO_HOUSE;
                                            f->destination_x = x_road;
                                            f->destination_y = y_road;
                                            f->roam_length = 0;
                                        } else {
                                            figure_delete(f);
                                            return;
                                        }
                                    } else {
                                        f->action_state = FIGURE_ACTION_HOMELESS_LEAVING;
                                        f->destination_x = scenario.exit_point.x;
                                        f->destination_y = scenario.exit_point.y;
                                        f->roam_length = 0;
                                        f->wait_ticks = 0;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_HOMELESS_GOING_TO_HOUSE:
                                f->is_invisible = 0;
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    all_buildings[f->immigrant_building_id].immigrant_figure_id = 0;
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    struct building_t *b = &all_buildings[f->immigrant_building_id];
                                    f->action_state = FIGURE_ACTION_HOMELESS_ENTERING_HOUSE;
                                    figure_movement_set_cross_country_destination(f, b->x, b->y);
                                    f->roam_length = 0;
                                }
                                break;
                            case FIGURE_ACTION_HOMELESS_ENTERING_HOUSE:
                                f->use_cross_country = 1;
                                f->is_invisible = 1;
                                if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                                    struct building_t *b = &all_buildings[f->immigrant_building_id];
                                    if (f->immigrant_building_id && building_is_house(b->type)) {
                                        int max_people = house_properties[b->subtype.house_level].max_people;
                                        if (b->house_is_merged) {
                                            max_people *= 4;
                                        }
                                        int room = max_people - b->house_population;
                                        if (room < 0) {
                                            room = 0;
                                        }
                                        if (room < f->migrant_num_people) {
                                            f->migrant_num_people = room;
                                        }
                                        if (!b->house_population) {
                                            building_house_change_to(b, BUILDING_HOUSE_SMALL_TENT);
                                        }
                                        b->house_population += f->migrant_num_people;
                                        b->house_population_room = max_people - b->house_population;
                                        city_population_add_homeless(f->migrant_num_people);
                                        b->immigrant_figure_id = 0;
                                    }
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_HOMELESS_LEAVING:
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                }
                                f->wait_ticks++;
                                if (f->wait_ticks > 30) {
                                    f->wait_ticks = 0;
                                    int building_id = closest_house_with_room(f->x, f->y);
                                    if (building_id > 0) {
                                        struct building_t *b = &all_buildings[building_id];
                                        int x_road, y_road;
                                        if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                            b->immigrant_figure_id = f->id;
                                            f->immigrant_building_id = building_id;
                                            f->action_state = FIGURE_ACTION_HOMELESS_GOING_TO_HOUSE;
                                            f->destination_x = x_road;
                                            f->destination_y = y_road;
                                            f->roam_length = 0;
                                            figure_route_remove(f);
                                        }
                                    }
                                }
                                break;
                        }
                        figure_image_increase_offset(f, 12);
                        f->image_id = image_group(GROUP_FIGURE_HOMELESS) + figure_image_direction(f) + 8 * f->image_offset;
                        break;
                    case FIGURE_PATRICIAN:
                        if (all_buildings[f->building_id].state != BUILDING_STATE_IN_USE) {
                            figure_delete(f);
                            return;
                        }
                        roamer_action(f, 1);
                        figure_image_increase_offset(f, 12);
                        f->image_id = image_group(GROUP_FIGURE_PATRICIAN) + figure_image_direction(f) + 8 * f->image_offset;
                        break;
                    case FIGURE_CART_PUSHER:
                    {
                        f->cart_image_id = 0;
                        int road_network_id = map_road_network_get(f->grid_offset);
                        struct building_t *b = &all_buildings[f->building_id];

                        switch (f->action_state) {
                            case FIGURE_ACTION_CARTPUSHER_INITIAL:
                                set_cart_graphic(f);
                                if (!map_routing_citizen_is_passable(f->grid_offset)) {
                                    figure_delete(f);
                                    return;
                                }
                                if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
                                    figure_delete(f);
                                    return;
                                }
                                f->wait_ticks++;
                                if (f->wait_ticks > 30) {
                                    struct map_point_t dst;
                                    int understaffed_storages = 0;
                                    // priority 1: warehouse if resource is on stockpile
                                    int dst_building_id = building_warehouse_for_storing(0, f->x, f->y,
                                        b->output_resource_id, road_network_id,
                                        &understaffed_storages, &dst);
                                    if (!city_data.resource.stockpiled[b->output_resource_id]) {
                                        dst_building_id = 0;
                                    }
                                    if (dst_building_id) {
                                        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE, dst_building_id, dst.x, dst.y);
                                    } else {
                                        // priority 2: accepting granary for food
                                        dst_building_id = building_granary_for_storing(f->x, f->y,
                                            b->output_resource_id, road_network_id, 0,
                                            &understaffed_storages, &dst);
                                        if (dst_building_id) {
                                            set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY, dst_building_id, dst.x, dst.y);
                                        } else {
                                            // priority 3: workshop for raw material
                                            dst_building_id = building_get_workshop_for_raw_material_with_room(f->x, f->y, b->output_resource_id, road_network_id, &dst);
                                            if (dst_building_id) {
                                                set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WORKSHOP, dst_building_id, dst.x, dst.y);
                                            } else {
                                                // priority 4: warehouse
                                                dst_building_id = building_warehouse_for_storing(0, f->x, f->y,
                                                    b->output_resource_id, road_network_id,
                                                    &understaffed_storages, &dst);
                                                if (dst_building_id) {
                                                    set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE, dst_building_id, dst.x, dst.y);
                                                } else {
                                                    // priority 5: granary forced when on stockpile
                                                    dst_building_id = building_granary_for_storing(f->x, f->y,
                                                        b->output_resource_id, road_network_id, 1,
                                                        &understaffed_storages, &dst);
                                                    if (dst_building_id) {
                                                        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY, dst_building_id, dst.x, dst.y);
                                                    } else {
                                                        // no one will accept
                                                        f->wait_ticks = 0;
                                                        // set cartpusher text
                                                        f->min_max_seen = understaffed_storages ? 2 : 1;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE:
                                set_cart_graphic(f);
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_CARTPUSHER_AT_WAREHOUSE;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    reroute_cartpusher(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY:
                                set_cart_graphic(f);
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_CARTPUSHER_AT_GRANARY;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    reroute_cartpusher(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    f->action_state = FIGURE_ACTION_CARTPUSHER_INITIAL;
                                    f->wait_ticks = 0;
                                }
                                if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WORKSHOP:
                                set_cart_graphic(f);
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_CARTPUSHER_AT_WORKSHOP;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    reroute_cartpusher(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_CARTPUSHER_AT_WAREHOUSE:
                                f->wait_ticks++;
                                if (f->wait_ticks > 10) {
                                    if (building_warehouse_add_resource(&all_buildings[f->destination_building_id], f->resource_id)) {
                                        f->action_state = FIGURE_ACTION_CARTPUSHER_RETURNING;
                                        f->wait_ticks = 0;
                                        f->destination_x = f->source_x;
                                        f->destination_y = f->source_y;
                                    } else {
                                        figure_route_remove(f);
                                        f->action_state = FIGURE_ACTION_CARTPUSHER_INITIAL;
                                        f->wait_ticks = 0;
                                    }
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_CARTPUSHER_AT_GRANARY:
                                f->wait_ticks++;
                                if (f->wait_ticks > 5) {
                                    if (building_granary_add_resource(&all_buildings[f->destination_building_id], f->resource_id, 1)) {
                                        f->action_state = FIGURE_ACTION_CARTPUSHER_RETURNING;
                                        f->wait_ticks = 0;
                                        f->destination_x = f->source_x;
                                        f->destination_y = f->source_y;
                                    } else {
                                        b = &all_buildings[f->building_id];
                                        struct map_point_t dst;
                                        // priority 1: accepting granary for food
                                        int dst_building_id = building_granary_for_storing(f->x, f->y,
                                            b->output_resource_id, road_network_id, 0,
                                            0, &dst);
                                        if (dst_building_id) {
                                            set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY, dst_building_id, dst.x, dst.y);
                                        } else {
                                            // priority 2: warehouse
                                            dst_building_id = building_warehouse_for_storing(0, f->x, f->y,
                                                b->output_resource_id, road_network_id,
                                                0, &dst);
                                            if (dst_building_id) {
                                                set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE, dst_building_id, dst.x, dst.y);
                                            } else {
                                                // priority 3: granary
                                                dst_building_id = building_granary_for_storing(f->x, f->y,
                                                    b->output_resource_id, road_network_id, 1,
                                                    0, &dst);
                                                if (dst_building_id) {
                                                    set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY, dst_building_id, dst.x, dst.y);
                                                } else {
                                                    // no one will accept, stand idle
                                                    f->wait_ticks = 0;
                                                }
                                            }
                                        }
                                    }
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_CARTPUSHER_AT_WORKSHOP:
                                f->wait_ticks++;
                                if (f->wait_ticks > 5) {
                                    building_workshop_add_raw_material(&all_buildings[f->destination_building_id]);
                                    f->action_state = FIGURE_ACTION_CARTPUSHER_RETURNING;
                                    f->wait_ticks = 0;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_CARTPUSHER_RETURNING:
                                f->cart_image_id = EMPTY_CART_IMG_ID;
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                        }
                        figure_image_increase_offset(f, 12);
                        update_image_cartpusher(f);
                    }
                    break;
                    case FIGURE_LABOR_SEEKER:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        if (b->state != BUILDING_STATE_IN_USE || b->figure_id2 != f->id) {
                            figure_delete(f);
                            return;
                        }
                        roamer_action(f, 1);
                        figure_image_increase_offset(f, 12);
                        f->image_id = image_group(GROUP_FIGURE_LABOR_SEEKER) + figure_image_direction(f) + 8 * f->image_offset;
                    }
                    break;
                    case FIGURE_BARBER:
                        culture_action(f, GROUP_FIGURE_BARBER);
                        break;
                    case FIGURE_BATHHOUSE_WORKER:
                        culture_action(f, GROUP_FIGURE_BATHHOUSE_WORKER);
                        break;
                    case FIGURE_DOCTOR:
                    case FIGURE_SURGEON:
                        culture_action(f, GROUP_FIGURE_DOCTOR_SURGEON);
                        break;
                    case FIGURE_PRIEST:
                        culture_action(f, GROUP_FIGURE_PRIEST);
                        break;
                    case FIGURE_SCHOOL_CHILD:\
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_SCHOOL) {
                            figure_delete(f);
                            return;
                        }
                        figure_image_increase_offset(f, 12);
                        switch (f->action_state) {
                            case FIGURE_ACTION_ROAMING:
                                f->is_invisible = 0;
                                f->roam_length++;
                                if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                                    figure_delete(f);
                                    return;
                                }
                                figure_movement_roam_ticks(f, 2);
                                break;
                        }
                        f->image_id = image_group(GROUP_FIGURE_SCHOOL_CHILD) + figure_image_direction(f) + 8 * f->image_offset;
                        break;
                    }
                    case FIGURE_TEACHER:
                        culture_action(f, GROUP_FIGURE_TEACHER_LIBRARIAN);
                        break;
                    case FIGURE_LIBRARIAN:
                        culture_action(f, GROUP_FIGURE_TEACHER_LIBRARIAN);
                        break;
                    case FIGURE_MISSIONARY:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
                            figure_delete(f);
                            return;
                        }
                        roamer_action(f, 1);
                        figure_image_increase_offset(f, 12);
                        f->image_id = image_group(GROUP_FIGURE_MISSIONARY) + figure_image_direction(f) + 8 * f->image_offset;
                        break;
                    }
                    case FIGURE_ACTOR:
                    case FIGURE_GLADIATOR:
                    case FIGURE_LION_TAMER:
                    case FIGURE_CHARIOTEER:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        f->cart_image_id = EMPTY_CART_IMG_ID;
                        f->use_cross_country = 0;
                        figure_image_increase_offset(f, 12);
                        f->wait_ticks_missile++;
                        if (f->wait_ticks_missile >= 120) {
                            f->wait_ticks_missile = 0;
                        }
                        if (scenario.gladiator_revolt.state == EVENT_IN_PROGRESS && f->type == FIGURE_GLADIATOR) {
                            if (f->action_state == FIGURE_ACTION_ENTERTAINER_GOING_TO_VENUE ||
                                f->action_state == FIGURE_ACTION_ENTERTAINER_ROAMING ||
                                f->action_state == FIGURE_ACTION_ENTERTAINER_RETURNING) {
                                f->type = FIGURE_ENEMY_GLADIATOR;
                                figure_route_remove(f);
                                f->roam_length = 0;
                                f->action_state = FIGURE_ACTION_NATIVE_CREATED;
                                f->is_targetable = 1;
                                f->terrain_usage = TERRAIN_USAGE_ANY;
                                return;
                            }
                        }
                        int speed_factor = f->type == FIGURE_CHARIOTEER ? 2 : 1;
                        switch (f->action_state) {
                            case FIGURE_ACTION_ENTERTAINER_AT_SCHOOL_CREATED:
                                f->is_invisible = 1;
                                f->image_offset = 0;
                                f->wait_ticks_missile = 0;
                                f->wait_ticks--;
                                if (f->wait_ticks <= 0) {
                                    int x_road, y_road;
                                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                        f->action_state = FIGURE_ACTION_ENTERTAINER_EXITING_SCHOOL;
                                        figure_movement_set_cross_country_destination(f, x_road, y_road);
                                        f->roam_length = 0;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_ENTERTAINER_EXITING_SCHOOL:
                                f->use_cross_country = 1;
                                f->is_invisible = 1;
                                if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                                    int dst_building_id = 0;
                                    switch (f->type) {
                                        case FIGURE_ACTOR:
                                            dst_building_id = determine_destination(f->x, f->y, BUILDING_THEATER, BUILDING_AMPHITHEATER);
                                            break;
                                        case FIGURE_GLADIATOR:
                                            dst_building_id = determine_destination(f->x, f->y, BUILDING_AMPHITHEATER, BUILDING_COLOSSEUM);
                                            break;
                                        case FIGURE_LION_TAMER:
                                            dst_building_id = determine_destination(f->x, f->y, BUILDING_COLOSSEUM, 0);
                                            break;
                                        case FIGURE_CHARIOTEER:
                                            dst_building_id = determine_destination(f->x, f->y, BUILDING_HIPPODROME, 0);
                                            break;
                                    }
                                    if (dst_building_id) {
                                        struct building_t *b_dst = &all_buildings[dst_building_id];
                                        int x_road, y_road;
                                        if (map_closest_road_within_radius(b_dst->x, b_dst->y, b_dst->size, 2, &x_road, &y_road)) {
                                            f->destination_building_id = dst_building_id;
                                            f->action_state = FIGURE_ACTION_ENTERTAINER_GOING_TO_VENUE;
                                            f->destination_x = x_road;
                                            f->destination_y = y_road;
                                            f->roam_length = 0;
                                        } else {
                                            figure_delete(f);
                                            return;
                                        }
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                f->is_invisible = 1;
                                break;
                            case FIGURE_ACTION_ENTERTAINER_GOING_TO_VENUE:
                            {
                                f->is_invisible = 0;
                                f->roam_length++;
                                if (f->roam_length >= 3200) {
                                    figure_delete(f);
                                    return;
                                }
                                figure_movement_move_ticks(f, speed_factor);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    b = &all_buildings[f->destination_building_id];
                                    if (b->type >= BUILDING_AMPHITHEATER && b->type <= BUILDING_COLOSSEUM) {
                                        switch (f->type) {
                                            case FIGURE_ACTOR:
                                                b->data.entertainment.play++;
                                                if (b->data.entertainment.play >= 5) {
                                                    b->data.entertainment.play = 0;
                                                }
                                                if (b->type == BUILDING_THEATER) {
                                                    b->data.entertainment.days1 = 32;
                                                } else {
                                                    b->data.entertainment.days2 = 32;
                                                }
                                                break;
                                            case FIGURE_GLADIATOR:
                                                if (b->type == BUILDING_AMPHITHEATER) {
                                                    b->data.entertainment.days1 = 32;
                                                } else {
                                                    b->data.entertainment.days2 = 32;
                                                }
                                                break;
                                            case FIGURE_LION_TAMER:
                                            case FIGURE_CHARIOTEER:
                                                b->data.entertainment.days1 = 32;
                                                break;
                                        }
                                    }
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            }
                            case FIGURE_ACTION_ENTERTAINER_ROAMING:
                                f->is_invisible = 0;
                                f->roam_length++;
                                if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                                    int x_road, y_road;
                                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                        f->action_state = FIGURE_ACTION_ENTERTAINER_RETURNING;
                                        f->destination_x = x_road;
                                        f->destination_y = y_road;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                figure_movement_roam_ticks(f, speed_factor);
                                break;
                            case FIGURE_ACTION_ENTERTAINER_RETURNING:
                                figure_movement_move_ticks(f, speed_factor);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                                    f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                        }
                        int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
                        if (f->type == FIGURE_CHARIOTEER) {
                            f->cart_image_id = 0;
                            f->image_id = image_group(GROUP_FIGURE_CHARIOTEER) + dir + 8 * f->image_offset;
                            return;
                        }
                        int image_id;
                        if (f->type == FIGURE_ACTOR) {
                            image_id = image_group(GROUP_FIGURE_ACTOR);
                        } else if (f->type == FIGURE_GLADIATOR) {
                            image_id = image_group(GROUP_FIGURE_GLADIATOR);
                        } else if (f->type == FIGURE_LION_TAMER) {
                            image_id = image_group(GROUP_FIGURE_LION_TAMER);
                            if (f->wait_ticks_missile >= 96) {
                                image_id = image_group(GROUP_FIGURE_LION_TAMER_WHIP);
                            }
                            f->cart_image_id = image_group(GROUP_FIGURE_LION);
                        } else {
                            return;
                        }
                        f->image_id = image_id + dir + 8 * f->image_offset;
                        if (f->cart_image_id) {
                            f->cart_image_id += dir + 8 * f->image_offset;
                            figure_image_set_cart_offset(f, dir);
                        }
                        break;
                    }
                    case FIGURE_HIPPODROME_HORSES:
                        figure_hippodrome_horse_action(f);
                        break;
                    case FIGURE_TAX_COLLECTOR:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        f->use_cross_country = 0;
                        if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
                            figure_delete(f);
                            return;
                        }
                        switch (f->action_state) {
                            case FIGURE_ACTION_TAX_COLLECTOR_CREATED:
                                f->is_invisible = 1;
                                f->image_offset = 0;
                                f->wait_ticks--;
                                if (f->wait_ticks <= 0) {
                                    int x_road, y_road;
                                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                        f->action_state = FIGURE_ACTION_TAX_COLLECTOR_ENTERING_EXITING;
                                        figure_movement_set_cross_country_destination(f, x_road, y_road);
                                        f->roam_length = 0;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_TAX_COLLECTOR_ENTERING_EXITING:
                                f->use_cross_country = 1;
                                f->is_invisible = 1;
                                if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                                    if (map_building_at(f->grid_offset) == f->building_id) {
                                        // returned to own building
                                        figure_delete(f);
                                        return;
                                    } else {
                                        f->action_state = FIGURE_ACTION_TAX_COLLECTOR_ROAMING;
                                        figure_movement_init_roaming(f);
                                        f->roam_length = 0;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_TAX_COLLECTOR_ROAMING:
                                f->is_invisible = 0;
                                f->roam_length++;
                                if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                                    int x_road, y_road;
                                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                        f->action_state = FIGURE_ACTION_TAX_COLLECTOR_RETURNING;
                                        f->destination_x = x_road;
                                        f->destination_y = y_road;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                figure_movement_roam_ticks(f, 1);
                                break;
                            case FIGURE_ACTION_TAX_COLLECTOR_RETURNING:
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_TAX_COLLECTOR_ENTERING_EXITING;
                                    figure_movement_set_cross_country_destination(f, b->x, b->y);
                                    f->roam_length = 0;
                                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                        }
                        figure_image_increase_offset(f, 12);
                        f->image_id = image_group(GROUP_FIGURE_TAX_COLLECTOR) + figure_image_direction(f) + 8 * f->image_offset;
                        break;
                    }
                    case FIGURE_ENGINEER:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        f->use_cross_country = 0;
                        if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
                            figure_delete(f);
                            return;
                        }
                        figure_image_increase_offset(f, 12);
                        switch (f->action_state) {
                            case FIGURE_ACTION_ENGINEER_CREATED:
                                f->is_invisible = 1;
                                f->image_offset = 0;
                                f->wait_ticks--;
                                if (f->wait_ticks <= 0) {
                                    int x_road, y_road;
                                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                        f->action_state = FIGURE_ACTION_ENGINEER_ENTERING_EXITING;
                                        figure_movement_set_cross_country_destination(f, x_road, y_road);
                                        f->roam_length = 0;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_ENGINEER_ENTERING_EXITING:
                                f->use_cross_country = 1;
                                f->is_invisible = 1;
                                if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                                    if (map_building_at(f->grid_offset) == f->building_id) {
                                        // returned to own building
                                        figure_delete(f);
                                        return;
                                    } else {
                                        f->action_state = FIGURE_ACTION_ENGINEER_ROAMING;
                                        figure_movement_init_roaming(f);
                                        f->roam_length = 0;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_ENGINEER_ROAMING:
                                f->is_invisible = 0;
                                f->roam_length++;
                                if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                                    int x_road, y_road;
                                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                        f->action_state = FIGURE_ACTION_ENGINEER_RETURNING;
                                        f->destination_x = x_road;
                                        f->destination_y = y_road;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                figure_movement_roam_ticks(f, 1);
                                break;
                            case FIGURE_ACTION_ENGINEER_RETURNING:
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_ENGINEER_ENTERING_EXITING;
                                    figure_movement_set_cross_country_destination(f, b->x, b->y);
                                    f->roam_length = 0;
                                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                        }
                        f->image_id = image_group(GROUP_FIGURE_ENGINEER) + figure_image_direction(f) + 8 * f->image_offset;
                    }
                    break;
                    case FIGURE_FISHING_BOAT:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        if (b->state != BUILDING_STATE_IN_USE) {
                            figure_delete(f);
                            return;
                        }
                        if (f->action_state != FIGURE_ACTION_FISHING_BOAT_CREATED && b->data.industry.fishing_boat_id != f->id) {
                            struct map_point_t tile;
                            b = &all_buildings[map_water_get_wharf_for_new_fishing_boat(f, &tile)];
                            if (b->id) {
                                f->building_id = b->id;
                                b->data.industry.fishing_boat_id = f->id;
                                f->action_state = FIGURE_ACTION_FISHING_BOAT_GOING_TO_WHARF;
                                f->destination_x = tile.x;
                                f->destination_y = tile.y;
                                f->source_x = tile.x;
                                f->source_y = tile.y;
                                figure_route_remove(f);
                            } else {
                                figure_delete(f);
                                return;
                            }
                        }
                        figure_image_increase_offset(f, 12);
                        f->cart_image_id = 0;
                        switch (f->action_state) {
                            case FIGURE_ACTION_FISHING_BOAT_CREATED:
                                f->wait_ticks++;
                                if (f->wait_ticks >= 50) {
                                    f->wait_ticks = 0;
                                    struct map_point_t tile;
                                    int wharf_id = map_water_get_wharf_for_new_fishing_boat(f, &tile);
                                    if (wharf_id) {
                                        b->figure_id = 0; // remove from original building
                                        f->building_id = wharf_id;
                                        all_buildings[wharf_id].data.industry.fishing_boat_id = f->id;
                                        f->action_state = FIGURE_ACTION_FISHING_BOAT_GOING_TO_WHARF;
                                        f->destination_x = tile.x;
                                        f->destination_y = tile.y;
                                        f->source_x = tile.x;
                                        f->source_y = tile.y;
                                        figure_route_remove(f);
                                    }
                                }
                                break;
                            case FIGURE_ACTION_FISHING_BOAT_GOING_TO_FISH:
                                figure_movement_move_ticks(f, 1);
                                f->height_adjusted_ticks = 0;
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    struct map_point_t tile = { 0 };

                                    if (map_figure_at(f->grid_offset) != f->id) {
                                        for (int radius = 1; radius <= 5; radius++) {
                                            int x_min, y_min, x_max, y_max;
                                            map_grid_get_area(f->x, f->y, 1, radius, &x_min, &y_min, &x_max, &y_max);
                                            for (int yy = y_min; yy <= y_max; yy++) {
                                                for (int xx = x_min; xx <= x_max; xx++) {
                                                    int grid_offset = map_grid_offset(xx, yy);
                                                    if (!map_has_figure_at(grid_offset) && map_terrain_is(grid_offset, TERRAIN_WATER)) {
                                                        tile.x = xx;
                                                        tile.y = yy;
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (tile.x) {
                                        figure_route_remove(f);
                                        f->destination_x = tile.x;
                                        f->destination_y = tile.y;
                                        f->direction = f->previous_tile_direction;
                                    } else {
                                        f->action_state = FIGURE_ACTION_FISHING_BOAT_FISHING;
                                        f->wait_ticks = 0;
                                    }
                                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    f->action_state = FIGURE_ACTION_FISHING_BOAT_AT_WHARF;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                }
                                break;
                            case FIGURE_ACTION_FISHING_BOAT_FISHING:
                                f->wait_ticks++;
                                if (f->wait_ticks >= 200) {
                                    f->wait_ticks = 0;
                                    f->action_state = FIGURE_ACTION_FISHING_BOAT_RETURNING_WITH_FISH;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                    figure_route_remove(f);
                                }
                                break;
                            case FIGURE_ACTION_FISHING_BOAT_GOING_TO_WHARF:
                                figure_movement_move_ticks(f, 1);
                                f->height_adjusted_ticks = 0;
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_FISHING_BOAT_AT_WHARF;
                                    f->wait_ticks = 0;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    // cannot reach grounds
                                    city_message_post_with_message_delay(MESSAGE_CAT_FISHING_BLOCKED, 1, MESSAGE_FISHING_BOAT_BLOCKED, 12);
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_FISHING_BOAT_AT_WHARF:
                            {
                                int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                                int max_wait_ticks = 5 * (102 - pct_workers);
                                if (b->data.industry.has_fish > 0) {
                                    pct_workers = 0;
                                }
                                if (pct_workers > 0) {
                                    f->wait_ticks++;
                                    if (f->wait_ticks >= max_wait_ticks) {
                                        f->wait_ticks = 0;
                                        struct map_point_t tile;
                                        int num_fishing_spots = 0;
                                        for (int k = 0; k < MAX_FISH_POINTS; k++) {
                                            if (scenario.fishing_points[k].x > 0) {
                                                num_fishing_spots++;
                                            }
                                        }
                                        if (num_fishing_spots) {
                                            int min_dist = 10000;
                                            int min_fish_id = 0;
                                            for (int k = 0; k < MAX_FISH_POINTS; k++) {
                                                if (scenario.fishing_points[k].x > 0) {
                                                    int dist = calc_maximum_distance(f->x, f->y, scenario.fishing_points[k].x, scenario.fishing_points[k].y);
                                                    if (dist < min_dist) {
                                                        min_dist = dist;
                                                        min_fish_id = k;
                                                    }
                                                }
                                            }
                                            if (min_dist < 10000) {
                                                tile.x = scenario.fishing_points[min_fish_id].x;
                                                tile.y = scenario.fishing_points[min_fish_id].y;
                                                f->action_state = FIGURE_ACTION_FISHING_BOAT_GOING_TO_FISH;
                                                f->destination_x = tile.x;
                                                f->destination_y = tile.y;
                                                figure_route_remove(f);
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                            case FIGURE_ACTION_FISHING_BOAT_RETURNING_WITH_FISH:
                                figure_movement_move_ticks(f, 1);
                                f->height_adjusted_ticks = 0;
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_FISHING_BOAT_AT_WHARF;
                                    f->wait_ticks = 0;
                                    b->figure_spawn_delay = 1;
                                    b->data.industry.has_fish++;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                        }
                        int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
                        if (f->action_state == FIGURE_ACTION_FISHING_BOAT_FISHING) {
                            f->image_id = image_group(GROUP_FIGURE_SHIP) + dir + 16;
                        } else {
                            f->image_id = image_group(GROUP_FIGURE_SHIP) + dir + 8;
                        }
                        break;
                    }
                    case FIGURE_FISH_GULLS:
                    {
                        f->use_cross_country = 1;
                        if (!(f->image_offset & 3) && figure_movement_move_ticks_cross_country(f, 1)) {
                            f->progress_on_tile++;
                            if (f->progress_on_tile > 8) {
                                f->progress_on_tile = 0;
                            }
                            figure_movement_set_cross_country_destination(f,
                                f->source_x + SEAGULL_OFFSETS[f->progress_on_tile].x,
                                f->source_y + SEAGULL_OFFSETS[f->progress_on_tile].y);
                        }
                        if (f->id & 1) {
                            figure_image_increase_offset(f, 54);
                            f->image_id = image_group(GROUP_FIGURE_SEAGULLS) + f->image_offset / 3;
                        } else {
                            figure_image_increase_offset(f, 72);
                            f->image_id = image_group(GROUP_FIGURE_SEAGULLS) + 18 + f->image_offset / 3;
                        }
                    }
                    break;
                    case FIGURE_SHIPWRECK:
                        figure_image_increase_offset(f, 128);
                        if (f->wait_ticks < 1000) {
                            map_figure_delete(f);
                            struct map_point_t tile = { 0 };
                            if (!(map_terrain_is(f->grid_offset, TERRAIN_WATER) && map_figure_at(f->grid_offset) == f->id)) {
                                for (int radius = 1; radius <= 5; radius++) {
                                    int x_min, y_min, x_max, y_max;
                                    map_grid_get_area(f->x, f->y, 1, radius, &x_min, &y_min, &x_max, &y_max);

                                    for (int yy = y_min; yy <= y_max; yy++) {
                                        for (int xx = x_min; xx <= x_max; xx++) {
                                            int grid_offset = map_grid_offset(xx, yy);
                                            if (!map_has_figure_at(grid_offset) || map_figure_at(grid_offset) == f->id) {
                                                if (map_terrain_is(grid_offset, TERRAIN_WATER) &&
                                                    map_terrain_is(map_grid_offset(xx, yy - 2), TERRAIN_WATER) &&
                                                    map_terrain_is(map_grid_offset(xx, yy + 2), TERRAIN_WATER) &&
                                                    map_terrain_is(map_grid_offset(xx - 2, yy), TERRAIN_WATER) &&
                                                    map_terrain_is(map_grid_offset(xx + 2, yy), TERRAIN_WATER)) {
                                                    f->x = tile.x;
                                                    f->y = tile.y;
                                                    f->grid_offset = map_grid_offset(f->x, f->y);
                                                    f->cross_country_x = 15 * f->x + 7;
                                                    f->cross_country_y = 15 * f->y + 7;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            map_figure_add(f);
                            f->wait_ticks = 1000;
                        }
                        f->wait_ticks++;
                        if (f->wait_ticks > 2000) {
                            figure_delete(f);
                            return;
                        }
                        f->image_id = image_group(GROUP_FIGURE_SHIPWRECK) + f->image_offset / 16;
                        break;
                    case FIGURE_DOCKER:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        figure_image_increase_offset(f, 12);
                        f->cart_image_id = 0;
                        if (b->state != BUILDING_STATE_IN_USE) {
                            figure_delete(f);
                            return;
                        }
                        if (b->type != BUILDING_DOCK && b->type != BUILDING_WHARF) {
                            figure_delete(f);
                            return;
                        }
                        if (b->data.dock.num_ships) {
                            b->data.dock.num_ships--;
                        }
                        if (b->data.dock.trade_ship_id) {
                            struct figure_t *ship = &figures[b->data.dock.trade_ship_id];
                            if (!figure_is_alive(ship) || ship->type != FIGURE_TRADE_SHIP) {
                                b->data.dock.trade_ship_id = 0;
                            } else if (trader_has_traded_max(ship->trader_id)) {
                                b->data.dock.trade_ship_id = 0;
                            } else if (ship->action_state == FIGURE_ACTION_TRADE_SHIP_LEAVING) {
                                b->data.dock.trade_ship_id = 0;
                            }
                        }
                        switch (f->action_state) {
                            case FIGURE_ACTION_DOCKER_IDLING:
                                f->resource_id = 0;
                                f->cart_image_id = 0;
                                int ship_id = b->data.dock.trade_ship_id;
                                struct figure_t *ship = &figures[ship_id];
                                int x, y;
                                get_trade_center_location(f, &x, &y);
                                struct map_point_t tile;
                                int importable[16];
                                importable[RESOURCE_NONE] = 0;
                                for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                                    importable[r] = can_import_resource_from_trade_city(ship->empire_city_id, r);
                                }
                                int resource = city_trade_next_docker_import_resource();
                                for (int k = RESOURCE_WHEAT; k < RESOURCE_TYPES_MAX && !importable[resource]; k++) {
                                    resource = city_trade_next_docker_import_resource();
                                }
                                int min_building_id = 0;
                                if (importable[resource]) {
                                    int min_distance = 10000;
                                    for (int l = 1; l < MAX_BUILDINGS; l++) {
                                        b = &all_buildings[l];
                                        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_WAREHOUSE) {
                                            continue;
                                        }
                                        if (!b->has_road_access) {
                                            continue;
                                        }
                                        struct building_storage_t *storage = building_storage_get(b->storage_id);
                                        if (storage->resource_state[resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING && !storage->empty_all) {
                                            int distance_penalty = 32;
                                            struct building_t *space = b;
                                            for (int s = 0; s < 8; s++) {
                                                space = &all_buildings[space->next_part_building_id];
                                                if (space->id && space->subtype.warehouse_resource_id == RESOURCE_NONE) {
                                                    distance_penalty -= 8;
                                                }
                                                if (space->id && space->subtype.warehouse_resource_id == resource && space->loads_stored < 4) {
                                                    distance_penalty -= 4;
                                                }
                                            }
                                            if (distance_penalty < 32) {
                                                int distance = calc_maximum_distance(
                                                    b->x, b->y, x, y);
                                                // prefer emptier warehouse
                                                distance += distance_penalty;
                                                if (distance < min_distance) {
                                                    min_distance = distance;
                                                    min_building_id = l;
                                                }
                                            }
                                        }
                                    }
                                    if (min_building_id) {
                                        struct building_t *min = &all_buildings[min_building_id];
                                        if (map_has_road_access(min->x, min->y, 3, &tile) && min->has_road_access == 1) {
                                            tile.x = min->x;
                                            tile.y = min->y;
                                        }
                                    }
                                }

                                if (!ship_id || ship->action_state != FIGURE_ACTION_TRADE_SHIP_MOORED || !ship->loads_sold_or_carrying || !min_building_id) {
                                    fetch_export_resource(f, b);
                                } else {
                                    ship->loads_sold_or_carrying--;
                                    f->destination_building_id = min_building_id;
                                    f->wait_ticks = 0;
                                    f->action_state = FIGURE_ACTION_DOCKER_IMPORT_QUEUE;
                                    f->destination_x = tile.x;
                                    f->destination_y = tile.y;
                                    f->resource_id = resource;
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_DOCKER_IMPORT_QUEUE:
                                f->cart_image_id = 0;
                                f->image_offset = 0;
                                if (b->data.dock.queued_docker_id <= 0) {
                                    b->data.dock.queued_docker_id = f->id;
                                    f->wait_ticks = 0;
                                }
                                if (b->data.dock.queued_docker_id == f->id) {
                                    b->data.dock.num_ships = 120;
                                    f->wait_ticks++;
                                    if (f->wait_ticks >= 80) {
                                        f->action_state = FIGURE_ACTION_DOCKER_IMPORT_GOING_TO_WAREHOUSE;
                                        f->wait_ticks = 0;
                                        set_cart_graphic(f);
                                        b->data.dock.queued_docker_id = 0;
                                    }
                                } else {
                                    int has_queued_docker = 0;
                                    for (int k = 0; k < 3; k++) {
                                        if (b->data.dock.docker_ids[k]) {
                                            struct figure_t *docker = &figures[b->data.dock.docker_ids[k]];
                                            if (docker->id == b->data.dock.queued_docker_id && figure_is_alive(docker)) {
                                                if (docker->action_state == FIGURE_ACTION_DOCKER_IMPORT_QUEUE ||
                                                    docker->action_state == FIGURE_ACTION_DOCKER_EXPORT_QUEUE) {
                                                    has_queued_docker = 1;
                                                }
                                            }
                                        }
                                    }
                                    if (!has_queued_docker) {
                                        b->data.dock.queued_docker_id = 0;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_DOCKER_EXPORT_QUEUE:
                                set_cart_graphic(f);
                                if (b->data.dock.queued_docker_id <= 0) {
                                    b->data.dock.queued_docker_id = f->id;
                                    f->wait_ticks = 0;
                                }
                                if (b->data.dock.queued_docker_id == f->id) {
                                    b->data.dock.num_ships = 120;
                                    f->wait_ticks++;
                                    if (f->wait_ticks >= 80) {
                                        f->action_state = FIGURE_ACTION_DOCKER_IDLING;
                                        f->wait_ticks = 0;
                                        f->image_id = 0;
                                        f->cart_image_id = 0;
                                        b->data.dock.queued_docker_id = 0;
                                    }
                                }
                                f->wait_ticks++;
                                if (f->wait_ticks >= 20) {
                                    f->action_state = FIGURE_ACTION_DOCKER_IDLING;
                                    f->wait_ticks = 0;
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_DOCKER_IMPORT_GOING_TO_WAREHOUSE:
                                set_cart_graphic(f);
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_DOCKER_IMPORT_AT_WAREHOUSE;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_DOCKER_EXPORT_GOING_TO_WAREHOUSE:
                                f->cart_image_id = EMPTY_CART_IMG_ID;
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_DOCKER_EXPORT_AT_WAREHOUSE;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_DOCKER_EXPORT_RETURNING:
                                set_cart_graphic(f);
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_DOCKER_EXPORT_QUEUE;
                                    f->wait_ticks = 0;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_DOCKER_IMPORT_RETURNING:
                                set_cart_graphic(f);
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_DOCKER_IDLING;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_DOCKER_IMPORT_AT_WAREHOUSE:
                                set_cart_graphic(f);
                                f->wait_ticks++;
                                if (f->wait_ticks > 10) {
                                    int trade_city_id;
                                    if (b->data.dock.trade_ship_id) {
                                        trade_city_id = figures[b->data.dock.trade_ship_id].empire_city_id;
                                    } else {
                                        trade_city_id = 0;
                                    }

                                    int try_import_resource = 0;
                                    struct building_t *warehouse = &all_buildings[f->destination_building_id];
                                    if (warehouse->type == BUILDING_WAREHOUSE) {
                                        // try existing storage bay with the same resource
                                        struct building_t *space = warehouse;
                                        for (int k = 0; k < 8; k++) {
                                            space = &all_buildings[space->next_part_building_id];
                                            if (space->id > 0) {
                                                if (space->loads_stored && space->loads_stored < 4 && space->subtype.warehouse_resource_id == f->resource_id) {
                                                    empire_objects[trade_city_id].resource_sold[f->resource_id]++;
                                                    building_warehouse_space_add_import(space, f->resource_id);
                                                    try_import_resource = 1;
                                                    break;
                                                }
                                            }
                                        }
                                        if (!try_import_resource) {
                                            // try unused storage bay
                                            space = warehouse;
                                            for (int k = 0; k < 8; k++) {
                                                space = &all_buildings[space->next_part_building_id];
                                                if (space->id > 0) {
                                                    if (space->subtype.warehouse_resource_id == RESOURCE_NONE) {
                                                        empire_objects[trade_city_id].resource_sold[f->resource_id]++;
                                                        building_warehouse_space_add_import(space, f->resource_id);
                                                        try_import_resource = 1;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (try_import_resource) {
                                        int trader_id = figures[b->data.dock.trade_ship_id].trader_id;
                                        trader_record_sold_resource(trader_id, f->resource_id);
                                        f->action_state = FIGURE_ACTION_DOCKER_IMPORT_RETURNING;
                                        f->wait_ticks = 0;
                                        f->destination_x = f->source_x;
                                        f->destination_y = f->source_y;
                                        f->resource_id = 0;
                                        fetch_export_resource(f, b);
                                    } else {
                                        f->action_state = FIGURE_ACTION_DOCKER_IMPORT_RETURNING;
                                        f->destination_x = f->source_x;
                                        f->destination_y = f->source_y;
                                    }
                                    f->wait_ticks = 0;
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_DOCKER_EXPORT_AT_WAREHOUSE:
                                f->cart_image_id = EMPTY_CART_IMG_ID;
                                f->wait_ticks++;
                                if (f->wait_ticks > 10) {
                                    int trade_city_id;
                                    if (b->data.dock.trade_ship_id) {
                                        trade_city_id = figures[b->data.dock.trade_ship_id].empire_city_id;
                                    } else {
                                        trade_city_id = 0;
                                    }
                                    f->action_state = FIGURE_ACTION_DOCKER_IMPORT_RETURNING;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                    f->wait_ticks = 0;
                                    struct building_t *warehouse = &all_buildings[f->destination_building_id];
                                    int try_export_resource = 0;
                                    if (warehouse->type == BUILDING_WAREHOUSE) {
                                        struct building_t *space = warehouse;
                                        for (int k = 0; k < 8; k++) {
                                            space = &all_buildings[space->next_part_building_id];
                                            if (space->id > 0) {
                                                if (space->loads_stored && space->subtype.warehouse_resource_id == f->resource_id) {
                                                    empire_objects[trade_city_id].resource_bought[f->resource_id]++;
                                                    city_resource_remove_from_warehouse(f->resource_id, 1);
                                                    space->loads_stored--;
                                                    if (space->loads_stored <= 0) {
                                                        space->subtype.warehouse_resource_id = RESOURCE_NONE;
                                                    }
                                                    city_finance_process_export(trade_prices[f->resource_id].sell);
                                                    building_warehouse_space_set_image(space, f->resource_id);
                                                    try_export_resource = 1;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    if (try_export_resource) {
                                        int trader_id = figures[b->data.dock.trade_ship_id].trader_id;
                                        trader_record_bought_resource(trader_id, f->resource_id);
                                        f->action_state = FIGURE_ACTION_DOCKER_EXPORT_RETURNING;
                                    } else {
                                        fetch_export_resource(f, b);
                                    }
                                }
                                f->image_offset = 0;
                                break;
                        }
                        int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
                        f->image_id = image_group(GROUP_FIGURE_CARTPUSHER) + dir + 8 * f->image_offset;
                        if (f->cart_image_id) {
                            f->cart_image_id += dir;
                            figure_image_set_cart_offset(f, dir);
                        } else {
                            f->image_id = 0;
                        }
                        break;
                    }
                    case FIGURE_FLOTSAM:
                        if (scenario.river_exit_point.x != -1 && scenario.river_exit_point.y != -1) {
                            f->is_invisible = 0;
                            switch (f->action_state) {
                                case FIGURE_ACTION_FLOTSAM_CREATED:
                                    f->is_invisible = 1;
                                    f->wait_ticks--;
                                    if (f->wait_ticks <= 0) {
                                        f->action_state = FIGURE_ACTION_FLOTSAM_FLOATING;
                                        f->wait_ticks = 0;
                                        int shipwreck_flotsam_created = 0;
                                        if (city_data.religion.neptune_sank_ships) {
                                            city_data.religion.neptune_sank_ships = 0;
                                            shipwreck_flotsam_created = 1;
                                        }
                                        if (!f->resource_id && shipwreck_flotsam_created) {
                                            f->min_max_seen = 1;
                                        }
                                        f->destination_x = scenario.river_exit_point.x;
                                        f->destination_y = scenario.river_exit_point.y;
                                    }
                                    break;
                                case FIGURE_ACTION_FLOTSAM_FLOATING:
                                    if (f->flotsam_visible) {
                                        f->flotsam_visible = 0;
                                    } else {
                                        f->flotsam_visible = 1;
                                        f->wait_ticks++;
                                        figure_movement_move_ticks(f, 1);
                                        f->is_invisible = 0;
                                        f->height_adjusted_ticks = 0;
                                        if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                                            f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                            f->action_state = FIGURE_ACTION_FLOTSAM_OFF_MAP;
                                        }
                                    }
                                    break;
                                case FIGURE_ACTION_FLOTSAM_OFF_MAP:
                                    f->is_invisible = 1;
                                    f->min_max_seen = 0;
                                    f->action_state = FIGURE_ACTION_FLOTSAM_CREATED;
                                    if (f->wait_ticks >= 400) {
                                        f->wait_ticks = random_byte() & 7;
                                    } else if (f->wait_ticks >= 200) {
                                        f->wait_ticks = 50 + (random_byte() & 0xf);
                                    } else if (f->wait_ticks >= 100) {
                                        f->wait_ticks = 100 + (random_byte() & 0x1f);
                                    } else if (f->wait_ticks >= 50) {
                                        f->wait_ticks = 200 + (random_byte() & 0x3f);
                                    } else {
                                        f->wait_ticks = 300 + random_byte();
                                    }
                                    map_figure_delete(f);
                                    f->x = scenario.river_entry_point.x;
                                    f->y = scenario.river_entry_point.y;
                                    f->grid_offset = map_grid_offset(f->x, f->y);
                                    f->cross_country_x = 15 * f->x;
                                    f->cross_country_y = 15 * f->y;
                                    break;
                            }
                            if (f->resource_id == 0) {
                                figure_image_increase_offset(f, 12);
                                if (f->min_max_seen) {
                                    f->image_id = image_group(GROUP_FIGURE_FLOTSAM_SHEEP) + FLOTSAM_TYPE_0[f->image_offset];
                                } else {
                                    f->image_id = image_group(GROUP_FIGURE_FLOTSAM_0) + FLOTSAM_TYPE_0[f->image_offset];
                                }
                            } else if (f->resource_id == 1) {
                                figure_image_increase_offset(f, 24);
                                f->image_id = image_group(GROUP_FIGURE_FLOTSAM_1) + FLOTSAM_TYPE_12[f->image_offset];
                            } else if (f->resource_id == 2) {
                                figure_image_increase_offset(f, 24);
                                f->image_id = image_group(GROUP_FIGURE_FLOTSAM_2) + FLOTSAM_TYPE_12[f->image_offset];
                            } else if (f->resource_id == 3) {
                                figure_image_increase_offset(f, 24);
                                if (FLOTSAM_TYPE_3[f->image_offset] == -1) {
                                    f->image_id = 0;
                                } else {
                                    f->image_id = image_group(GROUP_FIGURE_FLOTSAM_3) + FLOTSAM_TYPE_3[f->image_offset];
                                }
                            }
                        }
                        break;
                    case FIGURE_BALLISTA:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        f->is_invisible = 1;
                        f->height_adjusted_ticks = 10;
                        f->current_height = 45;
                        if (b->state != BUILDING_STATE_IN_USE || b->figure_id4 != f->id) {
                            figure_delete(f);
                            return;
                        }
                        if (b->num_workers <= 0 || b->figure_id <= 0) {
                            figure_delete(f);
                            return;
                        }
                        map_figure_delete(f);
                        switch (city_view_orientation()) {
                            case DIR_0_TOP: f->x = b->x; f->y = b->y; break;
                            case DIR_2_RIGHT: f->x = b->x + 1; f->y = b->y; break;
                            case DIR_4_BOTTOM: f->x = b->x + 1; f->y = b->y + 1; break;
                            case DIR_6_LEFT: f->x = b->x; f->y = b->y + 1; break;
                        }
                        f->grid_offset = map_grid_offset(f->x, f->y);
                        map_figure_add(f);
                        switch (f->action_state) {
                            case FIGURE_ACTION_BALLISTA_READY:
                            {
                                struct map_point_t tile = { -1, -1 };
                                if (f->is_shooting) {
                                    f->attack_image_offset++;
                                    if (f->attack_image_offset > 100) {
                                        f->attack_image_offset = 0;
                                        f->is_shooting = 0;
                                    }
                                } else {
                                    f->wait_ticks_missile++;
                                    if (f->wait_ticks_missile > 250) {
                                        f->wait_ticks_missile = 250;
                                    }
                                }
                                if (f->wait_ticks_missile > figure_properties[f->type].missile_delay && set_missile_target(f, &tile)) {
                                    f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                                    figure_create_missile(f, &tile, figure_properties[f->type].missile_type);
                                    play_sound_effect(SOUND_EFFECT_BALLISTA_SHOOT);
                                    f->wait_ticks_missile = 0;
                                    f->is_shooting = 1;
                                }
                                break;
                            }
                        }
                        int dir = figure_image_direction(f);
                        if (f->action_state == FIGURE_ACTION_BALLISTA_READY) {
                            f->image_id = image_group(GROUP_FIGURE_BALLISTA) + dir + 8 * BALLISTA_FIRING_OFFSETS[f->attack_image_offset / 4];
                        } else {
                            f->image_id = image_group(GROUP_FIGURE_BALLISTA) + dir;
                        }
                        break;
                    }
                    case FIGURE_BOLT:
                    {
                        f->use_cross_country = 1;
                        f->progress_on_tile++;
                        int should_die = figure_movement_move_ticks_cross_country(f, 10);
                        int target_id = get_target_on_tile(f);
                        if (target_id) {
                            struct figure_t *target = &figures[target_id];
                            missile_hit_target(f, target);
                            play_sound_effect(SOUND_EFFECT_BALLISTA_HIT_PERSON);
                        }
                        int dir = (16 + f->direction - 2 * city_view_orientation()) % 16;
                        f->image_id = image_group(GROUP_FIGURE_MISSILE) + 32 + dir;
                        if (f->progress_on_tile > 120) {
                            figure_delete(f);
                        }
                        if (should_die || target_id) {
                            play_sound_effect(SOUND_EFFECT_BALLISTA_HIT_GROUND);
                            figure_delete(f);
                        }
                        break;
                    }
                    case FIGURE_TOWER_SENTRY:
                        figure_tower_sentry_action(f);
                        break;
                    case FIGURE_JAVELIN:
                    {
                        f->use_cross_country = 1;
                        f->progress_on_tile++;
                        int should_die = figure_movement_move_ticks_cross_country(f, 4);
                        int target_id = get_target_on_tile(f);
                        if (target_id) {
                            struct figure_t *target = &figures[target_id];
                            missile_hit_target(f, target);
                            play_sound_effect(SOUND_EFFECT_JAVELIN);
                        }
                        int dir = (16 + f->direction - 2 * city_view_orientation()) % 16;
                        f->image_id = image_group(GROUP_FIGURE_MISSILE) + dir;
                        if (should_die || target_id) {
                            figure_delete(f);
                        }
                        if (f->progress_on_tile > 120) {
                            figure_delete(f);
                        }
                        break;
                    }
                    case FIGURE_PREFECT:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
                            figure_delete(f);
                            return;
                        }
                        f->terrain_usage = TERRAIN_USAGE_ROADS;
                        f->use_cross_country = 0;
                        figure_image_increase_offset(f, 12);
                        switch (f->action_state) {
                            case FIGURE_ACTION_PREFECT_CREATED:
                                f->is_invisible = 1;
                                f->image_offset = 0;
                                f->wait_ticks--;
                                if (f->wait_ticks <= 0) {
                                    int x_road, y_road;
                                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                        f->action_state = FIGURE_ACTION_PREFECT_ENTERING_EXITING;
                                        figure_movement_set_cross_country_destination(f, x_road, y_road);
                                        f->roam_length = 0;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_PREFECT_ENTERING_EXITING:
                                f->use_cross_country = 1;
                                f->is_invisible = 1;
                                if (figure_movement_move_ticks_cross_country(f, f->speed_multiplier) == 1) {
                                    if (map_building_at(f->grid_offset) == f->building_id) {
                                        // returned to own building
                                        figure_delete(f);
                                        return;
                                    } else {
                                        f->action_state = FIGURE_ACTION_PREFECT_ROAMING;
                                        figure_movement_init_roaming(f);
                                        f->roam_length = 0;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_PREFECT_ROAMING:
                                f->is_invisible = 0;
                                struct figure_t *target = melee_unit__set_closest_target(f);
                                if (target && calc_maximum_distance(f->x, f->y, b->x, b->y) < PREFECT_LEASH_RANGE) {
                                    f->terrain_usage = TERRAIN_USAGE_ANY;
                                    f->roam_length = figure_properties[f->type].max_roam_length;
                                    f->prefect_recent_guard_duty = 1;
                                    figure_movement_move_ticks(f, f->speed_multiplier);
                                    if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                        f->action_state = FIGURE_ACTION_PREFECT_RETURNING;
                                    }
                                    break;
                                }
                                if (fight_fire(f)) {
                                    break;
                                }
                                f->roam_length++;
                                if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                                    f->action_state = FIGURE_ACTION_PREFECT_RETURNING;
                                }
                                figure_movement_roam_ticks(f, f->speed_multiplier);
                                break;
                            case FIGURE_ACTION_PREFECT_RETURNING:
                                if (f->prefect_recent_guard_duty) {
                                    f->terrain_usage = TERRAIN_USAGE_ANY;
                                }
                                int x_road, y_road;
                                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                    f->destination_x = x_road;
                                    f->destination_y = y_road;
                                    figure_route_remove(f);
                                }
                                figure_movement_move_ticks(f, f->speed_multiplier);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_PREFECT_ENTERING_EXITING;
                                    figure_movement_set_cross_country_destination(f, b->x, b->y);
                                    f->roam_length = 0;
                                    f->prefect_recent_guard_duty = 0;
                                    f->target_figure_id = 0;
                                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_PREFECT_GOING_TO_FIRE:
                                f->terrain_usage = TERRAIN_USAGE_ANY;
                                figure_movement_move_ticks(f, f->speed_multiplier);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_PREFECT_AT_FIRE;
                                    figure_route_remove(f);
                                    f->roam_length = 0;
                                    f->wait_ticks = 50;
                                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_PREFECT_AT_FIRE:
                                struct building_t *burn = &all_buildings[f->destination_building_id];
                                int distance = calc_maximum_distance(f->x, f->y, burn->x, burn->y);
                                if (burn->state == BUILDING_STATE_IN_USE && burn->type == BUILDING_BURNING_RUIN && distance < 2) {
                                    burn->fire_duration = 32;
                                    play_sound_effect(SOUND_EFFECT_FIRE_SPLASH);
                                } else {
                                    f->wait_ticks = 1;
                                }
                                f->attack_direction = calc_general_direction(f->x, f->y, burn->x, burn->y);
                                if (f->attack_direction >= 8) {
                                    f->attack_direction = 0;
                                }
                                f->wait_ticks--;
                                if (f->wait_ticks <= 0) {
                                    f->wait_ticks_missile = 20;
                                    if (!fight_fire(f)) {
                                        if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                                            f->action_state = FIGURE_ACTION_PREFECT_RETURNING;
                                            f->destination_x = x_road;
                                            f->destination_y = y_road;
                                            figure_route_remove(f);
                                        } else {
                                            figure_delete(f);
                                            return;
                                        }
                                    }
                                }
                                break;
                        }
                        // graphic id
                        int dir;
                        if (f->action_state == FIGURE_ACTION_PREFECT_AT_FIRE) {
                            dir = f->attack_direction;
                        } else if (f->direction < 8) {
                            dir = f->direction;
                        } else {
                            dir = f->previous_tile_direction;
                        }
                        dir = figure_image_normalize_direction(dir);
                        switch (f->action_state) {
                            case FIGURE_ACTION_PREFECT_GOING_TO_FIRE:
                                f->image_id = image_group(GROUP_FIGURE_PREFECT_WITH_BUCKET) + dir + 8 * f->image_offset;
                                break;
                            case FIGURE_ACTION_PREFECT_AT_FIRE:
                                f->image_id = image_group(GROUP_FIGURE_PREFECT_WITH_BUCKET) + dir + 96 + 8 * (f->image_offset / 2);
                                break;
                            default:
                                f->image_id = image_group(GROUP_FIGURE_PREFECT) + dir + 8 * f->image_offset;
                                break;
                        }
                    }
                    break;
                    case FIGURE_FORT_STANDARD:
                        figure_image_increase_offset(f, 16);
                        f->image_id = image_group(GROUP_FIGURE_FORT_STANDARD_POLE) + 20 - legion_formations[f->formation_id].morale / 5;
                        if (legion_formations[f->formation_id].figure_type == FIGURE_FORT_LEGIONARY) {
                            f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + f->image_offset / 2;
                        } else if (legion_formations[f->formation_id].figure_type == FIGURE_FORT_MOUNTED) {
                            f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 18 + f->image_offset / 2;
                        } else {
                            f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 9 + f->image_offset / 2;
                        }
                        break;
                    case FIGURE_FORT_JAVELIN:
                    case FIGURE_FORT_MOUNTED:
                    case FIGURE_FORT_LEGIONARY:
                    {
                        figure_image_increase_offset(f, 12);
                        struct formation_t *m = &legion_formations[f->formation_id];
                        if (f->is_shooting) {
                            f->attack_image_offset++;
                            if (f->attack_image_offset > 100) {
                                f->attack_image_offset = 0;
                                f->is_shooting = 0;
                            }
                        } else {
                            f->wait_ticks_missile++;
                            if (f->wait_ticks_missile > 250) {
                                f->wait_ticks_missile = 250;
                            }
                        }
                        switch (f->action_state) {
                            case FIGURE_ACTION_SOLDIER_AT_REST:
                                map_figure_update(f);
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_SOLDIER_GOING_TO_FORT:
                                rout_unit(f);
                                break;
                            case FIGURE_ACTION_SOLDIER_RETURNING_TO_BARRACKS:
                                figure_movement_move_ticks(f, f->speed_multiplier);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                }
                                break;
                            case FIGURE_ACTION_SOLDIER_GOING_TO_STANDARD:
                                figure_movement_move_ticks(f, f->speed_multiplier);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                                    f->action_state = FIGURE_ACTION_SOLDIER_AT_STANDARD;
                                    if (f->type == FIGURE_FORT_LEGIONARY && rand() % 100 == 1) {
                                        play_sound_effect(SOUND_EFFECT_FORMATION_SHIELD);
                                    }
                                    if (m->layout == FORMATION_DOUBLE_LINE_1 || m->layout == FORMATION_SINGLE_LINE_1) {
                                        if (m->standard_y < m->prev_standard_y) {
                                            m->direction = DIR_0_TOP;
                                        } else if (m->standard_y > m->prev_standard_y) {
                                            m->direction = DIR_4_BOTTOM;
                                        }
                                    } else if (m->layout == FORMATION_DOUBLE_LINE_2 || m->layout == FORMATION_SINGLE_LINE_2) {
                                        if (m->standard_x < m->prev_standard_x) {
                                            m->direction = DIR_6_LEFT;
                                        } else if (m->standard_x > m->prev_standard_x) {
                                            m->direction = DIR_2_RIGHT;
                                        }
                                    } else if (m->layout == FORMATION_TORTOISE) {
                                        int dx = (m->standard_x < m->prev_standard_x) ? (m->prev_standard_x - m->standard_x) : (m->standard_x - m->prev_standard_x);
                                        int dy = (m->standard_y < m->prev_standard_y) ? (m->prev_standard_y - m->standard_y) : (m->standard_y - m->prev_standard_y);
                                        if (dx > dy) {
                                            if (m->standard_x < m->prev_standard_x) {
                                                m->direction = DIR_6_LEFT;
                                            } else if (m->standard_x > m->prev_standard_x) {
                                                m->direction = DIR_2_RIGHT;
                                            }
                                        } else {
                                            if (m->standard_y < m->prev_standard_y) {
                                                m->direction = DIR_0_TOP;
                                            } else if (m->standard_y > m->prev_standard_y) {
                                                m->direction = DIR_4_BOTTOM;
                                            }
                                        }
                                    }
                                    m->prev_standard_x = m->standard_x;
                                    m->prev_standard_y = m->standard_y;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                }
                                break;
                            case FIGURE_ACTION_SOLDIER_AT_STANDARD:
                                f->image_offset = 0;
                                map_figure_update(f);
                                if (f->type == FIGURE_FORT_JAVELIN) {
                                    struct map_point_t tile = { -1, -1 };
                                    if (f->wait_ticks_missile > figure_properties[f->type].missile_delay && set_missile_target(f, &tile)) {
                                        f->is_shooting = 1;
                                        f->wait_ticks_missile = 0;
                                        f->attack_image_offset = 1;
                                        f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                                        figure_create_missile(f, &tile, figure_properties[f->type].missile_type);
                                    } else {
                                        if (!f->is_shooting) {
                                            f->attack_image_offset = 0;
                                        }
                                    }
                                } else if (f->type == FIGURE_FORT_LEGIONARY) {
                                    // attack adjacent enemy
                                    for (int k = 0; k < 8; k++) {
                                        melee_attack_figure_at_offset(f, f->grid_offset + map_grid_direction_delta(k));
                                    }
                                }
                                break;
                            case FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY:
                                f->is_military_trained = 1;
                                if (f->type == FIGURE_FORT_MOUNTED) {
                                    f->mounted_charge_ticks = 20;
                                    f->mounted_charge_ticks_max = 20;
                                }
                                figure_movement_move_ticks(f, f->speed_multiplier);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    if (m->is_at_rest) {
                                        f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_FORT;
                                    } else {
                                        deploy_legion_unit_to_formation_location(f, m);
                                    }
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_SOLDIER_MOPPING_UP:
                            {
                                struct figure_t *target = melee_unit__set_closest_target(f);
                                if (target) {
                                    figure_movement_move_ticks(f, f->speed_multiplier);
                                    if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                        figure_route_remove(f);
                                        f->action_state = FIGURE_ACTION_SOLDIER_AT_STANDARD;
                                        f->target_figure_id = 0;
                                    }
                                } else {
                                    f->image_offset = 0;
                                    deploy_legion_unit_to_formation_location(f, m);
                                }
                                break;
                            }
                            case FIGURE_ACTION_SOLDIER_GOING_TO_DISTANT_BATTLE:
                            {
                                f->destination_x = scenario.exit_point.x;
                                f->destination_y = scenario.exit_point.y;
                                figure_movement_move_ticks(f, f->speed_multiplier);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->is_invisible = 1;
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            }
                            case FIGURE_ACTION_SOLDIER_RETURNING_FROM_DISTANT_BATTLE:
                                f->is_invisible = 0;
                                f->destination_x = m->standard_x + formation_layout_position_x(FORMATION_AT_REST, f->index_in_formation);
                                f->destination_y = m->standard_y + formation_layout_position_y(FORMATION_AT_REST, f->index_in_formation);
                                f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
                                figure_movement_move_ticks(f, f->speed_multiplier);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_SOLDIER_AT_REST;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                        }
                        int dir;
                        if (f->is_shooting) {
                            dir = f->direction;
                            m->direction = f->direction;
                        } else if (f->action_state == FIGURE_ACTION_SOLDIER_AT_STANDARD) {
                            dir = m->direction;
                        } else if (f->direction < 8) {
                            dir = f->direction;
                        } else {
                            dir = f->previous_tile_direction;
                        }
                        dir = figure_image_normalize_direction(dir);
                        if (f->type == FIGURE_FORT_JAVELIN) {
                            int image_id = image_group(GROUP_BUILDING_FORT_JAVELIN);
                            if (f->action_state == FIGURE_ACTION_SOLDIER_AT_STANDARD) {
                                f->image_id = image_id + 96 + dir + 8 * MISSILE_LAUNCHER_OFFSETS[f->attack_image_offset / 2];
                            } else {
                                f->image_id = image_id + dir + 8 * f->image_offset;
                            }
                        } else if (f->type == FIGURE_FORT_MOUNTED) {
                            int image_id = image_group(GROUP_FIGURE_FORT_MOUNTED);
                            f->image_id = image_id + dir + 8 * f->image_offset;
                        } else if (f->type == FIGURE_FORT_LEGIONARY) {
                            int image_id = image_group(GROUP_BUILDING_FORT_LEGIONARY);
                            if (f->action_state == FIGURE_ACTION_SOLDIER_AT_STANDARD) {
                                if (f->figure_is_halted && m->layout == FORMATION_TORTOISE && m->missile_attack_timeout) {
                                    f->image_id = image_id + dir + 144;
                                } else {
                                    f->image_id = image_id + dir;
                                }
                            } else {
                                f->image_id = image_id + dir + 8 * f->image_offset;
                            }
                        }
                        break;
                    }
                    case FIGURE_MARKET_BUYER:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        if (b->state != BUILDING_STATE_IN_USE || b->figure_id2 != f->id) {
                            figure_delete(f);
                            return;
                        }
                        figure_image_increase_offset(f, 12);
                        switch (f->action_state) {
                            case FIGURE_ACTION_MARKET_BUYER_GOING_TO_STORAGE:
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    if (f->collecting_item_id > 3) {
                                        int resource;
                                        switch (f->collecting_item_id) {
                                            case INVENTORY_POTTERY: resource = RESOURCE_POTTERY; break;
                                            case INVENTORY_FURNITURE: resource = RESOURCE_FURNITURE; break;
                                            case INVENTORY_OIL: resource = RESOURCE_OIL; break;
                                            case INVENTORY_WINE: resource = RESOURCE_WINE; break;
                                            default:
                                                figure_delete(f);
                                                return;
                                        }
                                        struct building_t *warehouse = &all_buildings[f->destination_building_id];
                                        int num_loads;
                                        int stored = building_warehouse_get_amount(warehouse, resource);
                                        if (stored < 2) {
                                            num_loads = stored;
                                        } else {
                                            num_loads = 2;
                                        }
                                        if (num_loads <= 0) {
                                            figure_delete(f);
                                            return;
                                        }
                                        building_warehouse_remove_resource(warehouse, resource, num_loads);
                                        // create delivery boys
                                        int boy1 = create_delivery_boy(f->id, f);
                                        if (num_loads > 1) {
                                            create_delivery_boy(boy1, f);
                                        }
                                    } else {
                                        int resource;
                                        switch (f->collecting_item_id) {
                                            case INVENTORY_WHEAT: resource = RESOURCE_WHEAT; break;
                                            case INVENTORY_VEGETABLES: resource = RESOURCE_VEGETABLES; break;
                                            case INVENTORY_FRUIT: resource = RESOURCE_FRUIT; break;
                                            case INVENTORY_MEAT: resource = RESOURCE_MEAT; break;
                                            default:
                                                figure_delete(f);
                                                return;
                                        }
                                        struct building_t *granary = &all_buildings[f->destination_building_id];
                                        int max_units = (f->collecting_item_id == INVENTORY_WHEAT ? 800 : 600) - all_buildings[f->building_id].data.market.inventory[f->collecting_item_id];
                                        int granary_units = granary->data.granary.resource_stored[resource];
                                        int num_loads;
                                        if (granary_units >= 800) {
                                            num_loads = 8;
                                        } else if (granary_units >= 700) {
                                            num_loads = 7;
                                        } else if (granary_units >= 600) {
                                            num_loads = 6;
                                        } else if (granary_units >= 500) {
                                            num_loads = 5;
                                        } else if (granary_units >= 400) {
                                            num_loads = 4;
                                        } else if (granary_units >= 300) {
                                            num_loads = 3;
                                        } else if (granary_units >= 200) {
                                            num_loads = 2;
                                        } else if (granary_units >= 100) {
                                            num_loads = 1;
                                        } else {
                                            num_loads = 0;
                                        }
                                        if (num_loads > max_units / 100) {
                                            num_loads = max_units / 100;
                                        }
                                        if (num_loads <= 0) {
                                            figure_delete(f);
                                            return;
                                        }
                                        building_granary_remove_resource(granary, resource, 100 * num_loads);
                                        // create delivery boys
                                        int previous_boy = f->id;
                                        for (int k = 0; k < num_loads; k++) {
                                            previous_boy = create_delivery_boy(previous_boy, f);
                                        }
                                    }
                                    f->action_state = FIGURE_ACTION_MARKET_BUYER_RETURNING;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    f->action_state = FIGURE_ACTION_MARKET_BUYER_RETURNING;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                    figure_route_remove(f);
                                }
                                break;
                            case FIGURE_ACTION_MARKET_BUYER_RETURNING:
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                }
                                break;
                        }
                        f->image_id = image_group(GROUP_FIGURE_MARKET_LADY) + figure_image_direction(f) + 8 * f->image_offset;
                        break;
                    }
                    case FIGURE_MARKET_TRADER:
                        struct building_t *market = &all_buildings[f->building_id];
                        if (market->state != BUILDING_STATE_IN_USE || market->figure_id != f->id) {
                            figure_delete(f);
                            return;
                        }
                        if (f->action_state == FIGURE_ACTION_ROAMING) {
                            // force return on out of stock
                            int max_stock = 0;
                            if (market->id > 0 && market->type == BUILDING_MARKET) {
                                for (int k = INVENTORY_OIL; k <= INVENTORY_FURNITURE; k++) {
                                    int stock = market->data.market.inventory[k];
                                    if (stock > max_stock) {
                                        max_stock = stock;
                                    }
                                }
                            }
                            int stock = building_market_get_max_food_stock(market) + max_stock;
                            if (f->roam_length >= 96 && stock <= 0) {
                                f->roam_length = figure_properties[f->type].max_roam_length;
                            }
                        }
                        roamer_action(f, 1);
                        figure_image_increase_offset(f, 12);
                        f->image_id = image_group(GROUP_FIGURE_MARKET_LADY) + figure_image_direction(f) + 8 * f->image_offset;
                        break;
                    case FIGURE_DELIVERY_BOY:
                    {
                        f->is_invisible = 0;
                        figure_image_increase_offset(f, 12);
                        struct figure_t *leader = &figures[f->leading_figure_id];
                        if (f->leading_figure_id <= 0) {
                            figure_delete(f);
                            return;
                        } else {
                            if (figure_is_alive(leader)) {
                                if (leader->type == FIGURE_MARKET_BUYER || leader->type == FIGURE_DELIVERY_BOY) {
                                    figure_movement_follow_ticks(f, 1);
                                } else {
                                    figure_delete(f);
                                    return;
                                }
                            } else { // leader arrived at market, drop resource at market
                                all_buildings[f->building_id].data.market.inventory[f->collecting_item_id] += 100;
                                figure_delete(f);
                                return;
                            }
                        }
                        if (leader->is_invisible) {
                            f->is_invisible = 1;
                        }
                        int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
                        f->image_id = image_group(GROUP_FIGURE_DELIVERY_BOY) + dir + 8 * f->image_offset;
                        break;
                    }
                    case FIGURE_WAREHOUSEMAN:
                    {
                        f->terrain_usage = TERRAIN_USAGE_ROADS;
                        figure_image_increase_offset(f, 12);
                        f->cart_image_id = 0;
                        int road_network_id = map_road_network_get(f->grid_offset);
                        switch (f->action_state) {
                            case FIGURE_ACTION_WAREHOUSEMAN_CREATED:
                            {
                                struct building_t *b = &all_buildings[f->building_id];
                                if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
                                    figure_delete(f);
                                    return;
                                }
                                f->wait_ticks++;
                                if (f->wait_ticks > 2) {
                                    if (all_buildings[f->building_id].type == BUILDING_GRANARY) {
                                        struct map_point_t dst;
                                        int dst_building_id;
                                        struct building_t *granary = &all_buildings[f->building_id];
                                        if (!f->resource_id) {
                                            // getting granaryman
                                            dst_building_id = building_granary_for_getting(granary, &dst);
                                            if (dst_building_id) {
                                                f->loads_sold_or_carrying = 0;
                                                set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_GETTING_FOOD, dst_building_id, dst.x, dst.y);
                                            } else {
                                                figure_delete(f);
                                            }
                                            return;
                                        }
                                        // delivering resource
                                        // priority 1: another granary
                                        dst_building_id = building_granary_for_storing(f->x, f->y, f->resource_id, road_network_id, 0,
                                            0, &dst);
                                        if (dst_building_id) {
                                            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
                                            building_granary_remove_resource(granary, f->resource_id, 100);
                                            return;
                                        }
                                        // priority 2: warehouse
                                        dst_building_id = building_warehouse_for_storing(0, f->x, f->y,
                                            f->resource_id, road_network_id, 0, &dst);
                                        if (dst_building_id) {
                                            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
                                            building_granary_remove_resource(granary, f->resource_id, 100);
                                            return;
                                        }
                                        // priority 3: granary even though resource is on stockpile
                                        dst_building_id = building_granary_for_storing(f->x, f->y,
                                            f->resource_id, road_network_id, 1, 0, &dst);
                                        if (dst_building_id) {
                                            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
                                            building_granary_remove_resource(granary, f->resource_id, 100);
                                            return;
                                        }
                                        // nowhere to go to: kill figure
                                        figure_delete(f);
                                    } else {
                                        struct map_point_t dst;
                                        int dst_building_id = 0;
                                        if (!f->resource_id) {
                                            // getting warehouseman
                                            dst_building_id = building_warehouse_for_getting(
                                                &all_buildings[f->building_id], f->collecting_item_id, &dst);
                                            if (dst_building_id) {
                                                f->loads_sold_or_carrying = 0;
                                                set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_GETTING_RESOURCE, dst_building_id, dst.x, dst.y);
                                                f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
                                            } else {
                                                figure_delete(f);
                                            }
                                            return;
                                        }
                                        // delivering resource
                                        // priority 1: weapons to barracks
                                        if (f->resource_id == RESOURCE_WEAPONS && !city_data.resource.stockpiled[RESOURCE_WEAPONS] && building_count_active(BUILDING_BARRACKS)) {
                                            b = &all_buildings[city_data.building.barracks_building_id];
                                            if (b->loads_stored < 5 && city_data.military.legionary_legions) {
                                                if (map_has_road_access(b->x, b->y, b->size, &dst) && b->road_network_id == road_network_id) {
                                                    dst_building_id = b->id;
                                                }
                                            }
                                        }
                                        if (dst_building_id) {
                                            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
                                            remove_resource_from_warehouse(f);
                                            return;
                                        }
                                        // priority 2: raw materials to workshop
                                        dst_building_id = building_get_workshop_for_raw_material_with_room(f->x, f->y, f->resource_id, road_network_id, &dst);
                                        if (dst_building_id) {
                                            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
                                            remove_resource_from_warehouse(f);
                                            return;
                                        }
                                        // priority 3: food to granary
                                        dst_building_id = building_granary_for_storing(f->x, f->y, f->resource_id, road_network_id, 0, 0, &dst);
                                        if (dst_building_id) {
                                            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
                                            remove_resource_from_warehouse(f);
                                            return;
                                        }
                                        // priority 4: food to getting granary
                                        if (!scenario.rome_supplies_wheat && resource_is_food(f->resource_id) && !city_data.resource.stockpiled[f->resource_id]) {
                                            int min_dist = INFINITE;
                                            int min_building_id = 0;
                                            for (int k = 1; k < MAX_BUILDINGS; k++) {
                                                b = &all_buildings[k];
                                                if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_GRANARY) {
                                                    continue;
                                                }
                                                if (!b->has_road_access || b->road_network_id != road_network_id) {
                                                    continue;
                                                }
                                                if (calc_percentage(b->num_workers, building_properties[b->type].n_laborers) < 100) {
                                                    continue;
                                                }
                                                struct building_storage_t *s = building_storage_get(b->storage_id);
                                                if (s->resource_state[f->resource_id] != BUILDING_STORAGE_STATE_GETTING || s->empty_all) {
                                                    continue;
                                                }
                                                if (b->data.granary.resource_stored[RESOURCE_NONE] > ONE_LOAD) {
                                                    // there is room
                                                    int dist = calc_maximum_distance(b->x + 1, b->y + 1, f->x, f->y);
                                                    if (dist < min_dist) {
                                                        min_dist = dist;
                                                        min_building_id = k;
                                                    }
                                                }
                                            }
                                            struct building_t *min = &all_buildings[min_building_id];
                                            dst.x = min->x + 1;
                                            dst.y = min->y + 1;
                                            dst_building_id = min_building_id;
                                            if (dst_building_id) {
                                                set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
                                                remove_resource_from_warehouse(f);
                                                return;
                                            }
                                        }
                                        // priority 5: resource to other warehouse
                                        dst_building_id = building_warehouse_for_storing(f->building_id, f->x, f->y, f->resource_id, road_network_id, 0, &dst);
                                        if (dst_building_id) {
                                            if (dst_building_id == f->building_id) {
                                                figure_delete(f);
                                            } else {
                                                set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
                                                remove_resource_from_warehouse(f);
                                            }
                                            return;
                                        }
                                        // priority 6: raw material to well-stocked workshop
                                        struct building_t *min_building = 0;
                                        if (!city_data.resource.stockpiled[f->resource_id]) {
                                            int output_type = resource_to_workshop_type(f->resource_id);
                                            if (output_type != WORKSHOP_NONE) {
                                                int min_dist = INFINITE;
                                                for (int k = 1; k < MAX_BUILDINGS; k++) {
                                                    b = &all_buildings[k];
                                                    if (b->state != BUILDING_STATE_IN_USE || !building_is_workshop(b->type)) {
                                                        continue;
                                                    }
                                                    if (!b->has_road_access) {
                                                        continue;
                                                    }
                                                    if (b->subtype.workshop_type == output_type && b->road_network_id == road_network_id) {
                                                        int dist = 10 * b->loads_stored +
                                                            calc_maximum_distance(b->x, b->y, f->x, f->y);
                                                        if (dist < min_dist) {
                                                            min_dist = dist;
                                                            min_building = b;
                                                        }
                                                    }
                                                }
                                                if (min_building) {
                                                    dst.x = min_building->road_access_x;
                                                    dst.y = min_building->road_access_y;
                                                }
                                            }
                                        }
                                        if (min_building->id) {
                                            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, min_building->id, dst.x, dst.y);
                                            remove_resource_from_warehouse(f);
                                            return;
                                        }
                                        // no destination: kill figure
                                        figure_delete(f);
                                    }
                                }
                                f->image_offset = 0;
                                break;
                            }
                            case FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE:
                                if (f->loads_sold_or_carrying == 1) {
                                    f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_FOOD) +
                                        8 * f->resource_id - 8 + resource_image_offset(f->resource_id, RESOURCE_IMAGE_FOOD_CART);
                                } else {
                                    set_cart_graphic(f);
                                }
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_WAREHOUSEMAN_AT_DELIVERY_BUILDING;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_WAREHOUSEMAN_AT_DELIVERY_BUILDING:
                                f->wait_ticks++;
                                if (f->wait_ticks > 4) {
                                    struct building_t *b = &all_buildings[f->destination_building_id];
                                    switch (b->type) {
                                        case BUILDING_GRANARY:
                                            building_granary_add_resource(b, f->resource_id, 0);
                                            break;
                                        case BUILDING_BARRACKS:
                                            b->loads_stored++;
                                            break;
                                        case BUILDING_WAREHOUSE:
                                        case BUILDING_WAREHOUSE_SPACE:
                                            building_warehouse_add_resource(b, f->resource_id);
                                            break;
                                        default: // workshop
                                            building_workshop_add_raw_material(b);
                                            break;
                                    }
                                    // BUG: what if warehouse/granary is full and returns false?
                                    f->action_state = FIGURE_ACTION_WAREHOUSEMAN_RETURNING_EMPTY;
                                    f->wait_ticks = 0;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_EMPTY:
                                f->cart_image_id = EMPTY_CART_IMG_ID;
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                }
                                break;
                            case FIGURE_ACTION_WAREHOUSEMAN_GETTING_FOOD:
                                f->cart_image_id = EMPTY_CART_IMG_ID;
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_WAREHOUSEMAN_AT_GRANARY;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_WAREHOUSEMAN_AT_GRANARY:
                                f->wait_ticks++;
                                if (f->wait_ticks > 4) {
                                    struct building_t *src = &all_buildings[f->destination_building_id];
                                    struct building_t *dst = &all_buildings[f->building_id];
                                    struct building_storage_t *s_src = building_storage_get(src->storage_id);
                                    struct building_storage_t *s_dst = building_storage_get(dst->storage_id);
                                    int max_amount = 0;
                                    int max_resource = 0;
                                    if (s_dst->resource_state[RESOURCE_WHEAT] == BUILDING_STORAGE_STATE_GETTING &&
                                            s_src->resource_state[RESOURCE_WHEAT] != BUILDING_STORAGE_STATE_GETTING) {
                                        if (src->data.granary.resource_stored[RESOURCE_WHEAT] > max_amount) {
                                            max_amount = src->data.granary.resource_stored[RESOURCE_WHEAT];
                                            max_resource = RESOURCE_WHEAT;
                                        }
                                    }
                                    if (s_dst->resource_state[RESOURCE_VEGETABLES] == BUILDING_STORAGE_STATE_GETTING &&
                                            s_src->resource_state[RESOURCE_VEGETABLES] != BUILDING_STORAGE_STATE_GETTING) {
                                        if (src->data.granary.resource_stored[RESOURCE_VEGETABLES] > max_amount) {
                                            max_amount = src->data.granary.resource_stored[RESOURCE_VEGETABLES];
                                            max_resource = RESOURCE_VEGETABLES;
                                        }
                                    }
                                    if (s_dst->resource_state[RESOURCE_FRUIT] == BUILDING_STORAGE_STATE_GETTING &&
                                            s_src->resource_state[RESOURCE_FRUIT] != BUILDING_STORAGE_STATE_GETTING) {
                                        if (src->data.granary.resource_stored[RESOURCE_FRUIT] > max_amount) {
                                            max_amount = src->data.granary.resource_stored[RESOURCE_FRUIT];
                                            max_resource = RESOURCE_FRUIT;
                                        }
                                    }
                                    if (s_dst->resource_state[RESOURCE_MEAT] == BUILDING_STORAGE_STATE_GETTING &&
                                            s_src->resource_state[RESOURCE_MEAT] != BUILDING_STORAGE_STATE_GETTING) {
                                        if (src->data.granary.resource_stored[RESOURCE_MEAT] > max_amount) {
                                            max_amount = src->data.granary.resource_stored[RESOURCE_MEAT];
                                            max_resource = RESOURCE_MEAT;
                                        }
                                    }
                                    if (max_amount > 800) {
                                        max_amount = 800;
                                    }
                                    if (max_amount > dst->data.granary.resource_stored[RESOURCE_NONE]) {
                                        max_amount = dst->data.granary.resource_stored[RESOURCE_NONE];
                                    }
                                    building_granary_remove_resource(src, max_resource, max_amount);
                                    f->loads_sold_or_carrying = max_amount / UNITS_PER_LOAD;
                                    f->resource_id = max_resource;
                                    f->action_state = FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_FOOD;
                                    f->wait_ticks = 0;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                    figure_route_remove(f);
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_FOOD:
                                // update graphic
                                if (f->loads_sold_or_carrying <= 0) {
                                    f->cart_image_id = EMPTY_CART_IMG_ID;
                                } else if (f->loads_sold_or_carrying == 1) {
                                    set_cart_graphic(f);
                                } else {
                                    if (f->loads_sold_or_carrying >= 8) {
                                        f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_FOOD) +
                                            CART_OFFSET_8_LOADS_FOOD[f->resource_id];
                                    } else {
                                        f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_FOOD) +
                                            CART_OFFSET_MULTIPLE_LOADS_FOOD[f->resource_id];
                                    }
                                    f->cart_image_id += resource_image_offset(f->resource_id, RESOURCE_IMAGE_FOOD_CART);
                                }
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    for (int k = 0; k < f->loads_sold_or_carrying; k++) {
                                        building_granary_add_resource(&all_buildings[f->building_id], f->resource_id, 0);
                                    }
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_WAREHOUSEMAN_GETTING_RESOURCE:
                                f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
                                f->cart_image_id = EMPTY_CART_IMG_ID;
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_WAREHOUSEMAN_AT_WAREHOUSE;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_WAREHOUSEMAN_AT_WAREHOUSE:
                                f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
                                f->wait_ticks++;
                                if (f->wait_ticks > 4) {
                                    f->loads_sold_or_carrying = 0;
                                    while (f->loads_sold_or_carrying < 4 && 0 == building_warehouse_remove_resource(
                                        &all_buildings[f->destination_building_id], f->collecting_item_id, 1)) {
                                        f->loads_sold_or_carrying++;
                                    }
                                    f->resource_id = f->collecting_item_id;
                                    f->action_state = FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_RESOURCE;
                                    f->wait_ticks = 0;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                    figure_route_remove(f);
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_RESOURCE:
                                f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
                                // update graphic
                                if (f->loads_sold_or_carrying <= 0) {
                                    f->cart_image_id = EMPTY_CART_IMG_ID;
                                } else if (f->loads_sold_or_carrying == 1) {
                                    set_cart_graphic(f);
                                } else {
                                    if (f->resource_id == RESOURCE_WHEAT || f->resource_id == RESOURCE_VEGETABLES ||
                                        f->resource_id == RESOURCE_FRUIT || f->resource_id == RESOURCE_MEAT) {
                                        f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_FOOD) +
                                            CART_OFFSET_MULTIPLE_LOADS_FOOD[f->resource_id];
                                    } else {
                                        f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_RESOURCE) +
                                            CART_OFFSET_MULTIPLE_LOADS_NON_FOOD[f->resource_id];
                                    }
                                    f->cart_image_id += resource_image_offset(f->resource_id, RESOURCE_IMAGE_FOOD_CART);
                                }
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    for (int k = 0; k < f->loads_sold_or_carrying; k++) {
                                        building_warehouse_add_resource(&all_buildings[f->building_id], f->resource_id);
                                    }
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                        }
                        update_image_cartpusher(f);
                    }
                    break;
                    case FIGURE_PROTESTER:
                    {
                        figure_image_increase_offset(f, 64);
                        f->wait_ticks++;
                        if (f->wait_ticks > 200) {
                            figure_delete(f);
                            return;
                        }
                        f->image_id = image_group(GROUP_FIGURE_CRIMINAL) + CRIMINAL_OFFSETS[f->image_offset / 4] + 104;
                    }
                    break;
                    case FIGURE_CRIMINAL:
                    {
                        figure_image_increase_offset(f, 32);
                        f->wait_ticks++;
                        if (f->wait_ticks > 200) {
                            figure_delete(f);
                            return;
                        }
                        f->image_id = image_group(GROUP_FIGURE_CRIMINAL) + CRIMINAL_OFFSETS[f->image_offset / 2] + 104;
                    }
                    break;
                    case FIGURE_RIOTER:
                    {
                        switch (f->action_state) {
                            case FIGURE_ACTION_RIOTER_CREATED:
                                figure_image_increase_offset(f, 32);
                                f->wait_ticks++;
                                if (f->wait_ticks >= 160) {
                                    f->action_state = FIGURE_ACTION_RIOTER_MOVING;
                                    int x_tile, y_tile;
                                    int building_id = formation_rioter_get_target_building(&x_tile, &y_tile);
                                    if (building_id) {
                                        f->destination_x = x_tile;
                                        f->destination_y = y_tile;
                                        f->destination_building_id = building_id;
                                        figure_route_remove(f);
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_RIOTER_MOVING:
                                figure_image_increase_offset(f, 12);
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    int x_tile, y_tile;
                                    int building_id = formation_rioter_get_target_building(&x_tile, &y_tile);
                                    if (building_id) {
                                        f->destination_x = x_tile;
                                        f->destination_y = y_tile;
                                        f->destination_building_id = building_id;
                                        figure_route_remove(f);
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    f->action_state = FIGURE_ACTION_RIOTER_CREATED;
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_ATTACK) {
                                    if (f->image_offset > 12) {
                                        f->image_offset = 0;
                                    }
                                }
                                break;
                        }
                        int dir;
                        if (f->direction == DIR_FIGURE_ATTACK) {
                            dir = f->attack_direction;
                        } else if (f->direction < 8) {
                            dir = f->direction;
                        } else {
                            dir = f->previous_tile_direction;
                        }
                        dir = figure_image_normalize_direction(dir);

                        if (f->direction == DIR_FIGURE_ATTACK) {
                            f->image_id = image_group(GROUP_FIGURE_CRIMINAL) + 104 + CRIMINAL_OFFSETS[f->image_offset % 16];
                        } else if (f->action_state == FIGURE_ACTION_RIOTER_MOVING) {
                            f->image_id = image_group(GROUP_FIGURE_CRIMINAL) + dir + 8 * f->image_offset;
                        } else {
                            f->image_id = image_group(GROUP_FIGURE_CRIMINAL) + 104 + CRIMINAL_OFFSETS[f->image_offset / 2];
                        }
                    }
                    break;
                    case FIGURE_TRADE_CARAVAN:
                        f->is_invisible = 0;
                        figure_image_increase_offset(f, 12);
                        switch (f->action_state) {
                            case FIGURE_ACTION_TRADE_CARAVAN_CREATED:
                                f->is_invisible = 1;
                                f->wait_ticks++;
                                if (f->wait_ticks > 20) {
                                    f->wait_ticks = 0;
                                    int x_base, y_base;
                                    if (city_data.building.trade_center_building_id) {
                                        struct building_t *trade_center = &all_buildings[city_data.building.trade_center_building_id];
                                        x_base = trade_center->x;
                                        y_base = trade_center->y;
                                    } else {
                                        x_base = f->x;
                                        y_base = f->y;
                                    }
                                    go_to_next_warehouse(f, x_base, y_base);
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_TRADE_CARAVAN_ARRIVING:
                                figure_movement_move_ticks(f, 1);
                                switch (f->direction) {
                                    case DIR_FIGURE_AT_DESTINATION:
                                        f->action_state = FIGURE_ACTION_TRADE_CARAVAN_TRADING;
                                        break;
                                    case DIR_FIGURE_REROUTE:
                                        figure_route_remove(f);
                                        break;
                                    case DIR_FIGURE_LOST:
                                        figure_delete(f);
                                        return;
                                }
                                if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_TRADE_CARAVAN_TRADING:
                                f->wait_ticks++;
                                if (f->wait_ticks > 10) {
                                    f->wait_ticks = 0;
                                    int move_on = 0;
                                    if (figure_trade_caravan_can_buy(f, f->destination_building_id, f->empire_city_id)) {
                                        int resource = trader_get_buy_resource(f->destination_building_id, f->empire_city_id);
                                        if (resource) {
                                            empire_objects[f->empire_city_id].resource_bought[resource]++;
                                            trader_record_bought_resource(f->trader_id, resource);
                                            f->trader_amount_bought++;
                                        } else {
                                            move_on++;
                                        }
                                    } else {
                                        move_on++;
                                    }
                                    if (figure_trade_caravan_can_sell(f, f->destination_building_id, f->empire_city_id)) {
                                        int resource = RESOURCE_NONE;
                                        struct building_t *warehouse = &all_buildings[f->destination_building_id];
                                        if (warehouse->type == BUILDING_WAREHOUSE) {
                                            int resource_to_import = city_data.trade.caravan_import_resource;
                                            int imp = RESOURCE_WHEAT;
                                            while (imp < RESOURCE_TYPES_MAX && !can_import_resource_from_trade_city(f->empire_city_id, resource_to_import)) {
                                                imp++;
                                                resource_to_import = city_trade_next_caravan_import_resource();
                                            }
                                            if (imp < RESOURCE_TYPES_MAX) {
                                                // add to existing bay with room
                                                struct building_t *space = warehouse;
                                                for (int k = 0; k < 8; k++) {
                                                    space = &all_buildings[space->next_part_building_id];
                                                    if (space->id > 0 && space->loads_stored > 0 && space->loads_stored < 4 &&
                                                        space->subtype.warehouse_resource_id == resource_to_import) {
                                                        building_warehouse_space_add_import(space, resource_to_import);
                                                        city_trade_next_caravan_import_resource();
                                                        resource = resource_to_import;
                                                        break;
                                                    }
                                                }
                                                if (!resource) {
                                                    // add to empty bay
                                                    space = warehouse;
                                                    for (int k = 0; k < 8; k++) {
                                                        space = &all_buildings[space->next_part_building_id];
                                                        if (space->id > 0 && !space->loads_stored) {
                                                            building_warehouse_space_add_import(space, resource_to_import);
                                                            city_trade_next_caravan_import_resource();
                                                            resource = resource_to_import;
                                                        }
                                                    }
                                                    if (!resource) {
                                                        // find another importable resource that can be added to this warehouse
                                                        for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                                                            resource_to_import = city_trade_next_caravan_backup_import_resource();
                                                            if (can_import_resource_from_trade_city(f->empire_city_id, resource_to_import)) {
                                                                space = warehouse;
                                                                for (int k = 0; k < 8; k++) {
                                                                    space = &all_buildings[space->next_part_building_id];
                                                                    if (space->id > 0 && space->loads_stored < 4
                                                                        && space->subtype.warehouse_resource_id == resource_to_import) {
                                                                        building_warehouse_space_add_import(space, resource_to_import);
                                                                        resource = resource_to_import;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (resource) {
                                            empire_objects[f->empire_city_id].resource_sold[resource]++;
                                            trader_record_sold_resource(f->trader_id, resource);
                                            f->loads_sold_or_carrying++;
                                        } else {
                                            move_on++;
                                        }
                                    } else {
                                        move_on++;
                                    }
                                    if (move_on == 2) {
                                        go_to_next_warehouse(f, f->x, f->y);
                                    }
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_TRADE_CARAVAN_LEAVING:
                                figure_movement_move_ticks(f, 1);
                                switch (f->direction) {
                                    case DIR_FIGURE_AT_DESTINATION:
                                        f->action_state = FIGURE_ACTION_TRADE_CARAVAN_CREATED;
                                        figure_delete(f);
                                        return;
                                    case DIR_FIGURE_REROUTE:
                                        figure_route_remove(f);
                                        break;
                                    case DIR_FIGURE_LOST:
                                        figure_delete(f);
                                        return;
                                }
                                break;
                        }
                        int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
                        f->image_id = image_group(GROUP_FIGURE_TRADE_CARAVAN) + dir + 8 * f->image_offset;
                        break;
                    case FIGURE_TRADE_CARAVAN_DONKEY:
                    {
                        f->is_invisible = 0;
                        figure_image_increase_offset(f, 12);
                        struct figure_t *leader = &figures[f->leading_figure_id];
                        if (f->leading_figure_id <= 0) {
                            figure_delete(f);
                            return;
                        } else {
                            if (leader->type != FIGURE_TRADE_CARAVAN && leader->type != FIGURE_TRADE_CARAVAN_DONKEY) {
                                figure_delete(f);
                                return;
                            } else {
                                figure_movement_follow_ticks(f, 1);
                            }
                        }
                        if (leader->is_invisible) {
                            f->is_invisible = 1;
                        }
                        dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
                        f->image_id = image_group(GROUP_FIGURE_TRADE_CARAVAN) + dir + 8 * f->image_offset;
                        break;
                    }
                    case FIGURE_TRADE_SHIP:
                        f->is_invisible = 0;
                        figure_image_increase_offset(f, 12);
                        switch (f->action_state) {
                            case FIGURE_ACTION_TRADE_SHIP_CREATED:
                                f->loads_sold_or_carrying = 12;
                                f->trader_amount_bought = 0;
                                f->is_invisible = 1;
                                f->wait_ticks++;
                                if (f->wait_ticks > 20) {
                                    f->wait_ticks = 0;
                                    struct map_point_t tile;
                                    int dock_id = building_dock_get_free_destination(f->id, &tile);
                                    if (dock_id) {
                                        f->destination_building_id = dock_id;
                                        f->action_state = FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK;
                                        f->destination_x = tile.x;
                                        f->destination_y = tile.y;
                                    } else if (building_dock_get_queue_destination(&tile)) {
                                        f->action_state = FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK_QUEUE;
                                        f->destination_x = tile.x;
                                        f->destination_y = tile.y;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK:
                                figure_movement_move_ticks(f, 1);
                                f->height_adjusted_ticks = 0;
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_TRADE_SHIP_MOORED;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    if (!city_message_get_category_count(MESSAGE_CAT_BLOCKED_DOCK)) {
                                        city_message_post(1, MESSAGE_NAVIGATION_IMPOSSIBLE, 0, 0);
                                        city_message_increase_category_count(MESSAGE_CAT_BLOCKED_DOCK);
                                    }
                                    return;
                                }
                                if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                                    f->action_state = FIGURE_ACTION_TRADE_SHIP_LEAVING;
                                    f->wait_ticks = 0;
                                    f->destination_x = scenario.river_exit_point.x;
                                    f->destination_y = scenario.river_exit_point.y;
                                }
                                break;
                            case FIGURE_ACTION_TRADE_SHIP_MOORED:
                            {
                                int trade_ship_done_trading = 1;
                                struct building_t *b = &all_buildings[f->destination_building_id];
                                if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_DOCK && b->num_workers > 0) {
                                    for (int k = 0; k < 3; k++) {
                                        if (b->data.dock.docker_ids[k]) {
                                            struct figure_t *docker = &figures[b->data.dock.docker_ids[k]];
                                            if (figure_is_alive(docker) && docker->action_state != FIGURE_ACTION_DOCKER_IDLING) {
                                                trade_ship_done_trading = 0;
                                                break;
                                            }
                                        }
                                    }
                                }
                                int trade_ship_lost_queue = 1;
                                if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_DOCK && b->num_workers > 0 && b->data.dock.trade_ship_id == f->id) {
                                    trade_ship_lost_queue = 0;
                                }
                                if (trade_ship_lost_queue) {
                                    f->trade_ship_failed_dock_attempts = 0;
                                    f->action_state = FIGURE_ACTION_TRADE_SHIP_LEAVING;
                                    f->wait_ticks = 0;
                                    f->destination_x = scenario.river_entry_point.x;
                                    f->destination_y = scenario.river_entry_point.y;
                                } else if (trade_ship_done_trading) {
                                    f->trade_ship_failed_dock_attempts = 0;
                                    f->action_state = FIGURE_ACTION_TRADE_SHIP_LEAVING;
                                    f->wait_ticks = 0;
                                    f->destination_x = scenario.river_entry_point.x;
                                    f->destination_y = scenario.river_entry_point.y;
                                    struct building_t *dst = &all_buildings[f->destination_building_id];
                                    dst->data.dock.queued_docker_id = 0;
                                    dst->data.dock.num_ships = 0;
                                }
                                switch (all_buildings[f->destination_building_id].data.dock.orientation) {
                                    case 0: f->direction = DIR_2_RIGHT; break;
                                    case 1: f->direction = DIR_4_BOTTOM; break;
                                    case 2: f->direction = DIR_6_LEFT; break;
                                    default:f->direction = DIR_0_TOP; break;
                                }
                                f->image_offset = 0;
                                city_message_reset_category_count(MESSAGE_CAT_BLOCKED_DOCK);
                                break;
                            }
                            case FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK_QUEUE:
                                figure_movement_move_ticks(f, 1);
                                f->height_adjusted_ticks = 0;
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_TRADE_SHIP_ANCHORED;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_TRADE_SHIP_ANCHORED:
                                f->wait_ticks++;
                                if (f->wait_ticks > 40) {
                                    struct map_point_t tile;
                                    int dock_id = building_dock_get_free_destination(f->id, &tile);
                                    if (dock_id) {
                                        f->destination_building_id = dock_id;
                                        f->action_state = FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK;
                                        f->destination_x = tile.x;
                                        f->destination_y = tile.y;
                                    } else if (map_figure_at(f->grid_offset) != f->id &&
                                        building_dock_get_queue_destination(&tile)) {
                                        f->action_state = FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK_QUEUE;
                                        f->destination_x = tile.x;
                                        f->destination_y = tile.y;
                                    }
                                    f->wait_ticks = 0;
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_TRADE_SHIP_LEAVING:
                                figure_movement_move_ticks(f, 1);
                                f->height_adjusted_ticks = 0;
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_TRADE_SHIP_CREATED;
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                        }
                        dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
                        f->image_id = image_group(GROUP_FIGURE_SHIP) + dir;
                        break;
                    case FIGURE_INDIGENOUS_NATIVE:
                    {
                        struct building_t *b = &all_buildings[f->building_id];
                        if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
                            figure_delete(f);
                            return;
                        }
                        figure_image_increase_offset(f, 12);
                        switch (f->action_state) {
                            case FIGURE_ACTION_NATIVE_GOING_TO_MEETING_CENTER:
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_NATIVE_RETURNING_FROM_MEETING;
                                    f->destination_x = f->source_x;
                                    f->destination_y = f->source_y;
                                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_NATIVE_RETURNING_FROM_MEETING:
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                                    f->direction == DIR_FIGURE_REROUTE ||
                                    f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_NATIVE_CREATED:
                            {
                                f->image_offset = 0;
                                f->wait_ticks++;
                                if (f->wait_ticks > 10 + (f->id & 3)) {
                                    f->wait_ticks = 0;
                                    if (city_data.military.native_attack_duration) {
                                        f->action_state = FIGURE_ACTION_NATIVE_ATTACKING;
                                        struct building_t *min_building = 0;
                                        int min_distance = 10000;
                                        for (int k = 1; k < MAX_BUILDINGS; k++) {
                                            b = &all_buildings[k];
                                            if (b->state != BUILDING_STATE_IN_USE) {
                                                continue;
                                            }
                                            switch (b->type) {
                                                case BUILDING_MISSION_POST:
                                                case BUILDING_FORT_LEGIONARIES:
                                                case BUILDING_FORT_JAVELIN:
                                                case BUILDING_FORT_MOUNTED:
                                                case BUILDING_FORT_GROUND:
                                                case BUILDING_NATIVE_HUT:
                                                case BUILDING_NATIVE_CROPS:
                                                case BUILDING_NATIVE_MEETING:
                                                case BUILDING_WAREHOUSE:
                                                    break;
                                                default:
                                                {
                                                    int distance = calc_maximum_distance(city_data.building.main_native_meeting.x, city_data.building.main_native_meeting.y, b->x, b->y);
                                                    if (distance < min_distance) {
                                                        min_building = b;
                                                        min_distance = distance;
                                                    }
                                                }
                                            }
                                        }
                                        if (min_building) {
                                            f->destination_x = min_building->x;
                                            f->destination_y = min_building->y;
                                        }
                                    } else {
                                        int x_tile, y_tile;
                                        struct building_t *meeting = &all_buildings[b->subtype.native_meeting_center_id];
                                        if (map_terrain_get_adjacent_road_or_clear_land(
                                            meeting->x, meeting->y, meeting->size, &x_tile, &y_tile)) {
                                            f->action_state = FIGURE_ACTION_NATIVE_GOING_TO_MEETING_CENTER;
                                            f->destination_x = x_tile;
                                            f->destination_y = y_tile;
                                        }
                                    }
                                    figure_route_remove(f);
                                }
                                break;
                            }
                            case FIGURE_ACTION_NATIVE_ATTACKING:
                                f->terrain_usage = TERRAIN_USAGE_ENEMY;
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                                    f->direction == DIR_FIGURE_REROUTE ||
                                    f->direction == DIR_FIGURE_LOST) {
                                    f->action_state = FIGURE_ACTION_NATIVE_CREATED;
                                }
                                break;
                        }
                        dir = get_direction(f);
                        if (f->action_state == FIGURE_ACTION_NATIVE_ATTACKING) {
                            f->image_id = 297 + dir + 8 * f->image_offset;
                        } else {
                            f->image_id = 201 + dir + 8 * f->image_offset;
                        }
                    }
                    break;
                    case FIGURE_NATIVE_TRADER:
                        f->is_invisible = 0;
                        figure_image_increase_offset(f, 12);
                        f->cart_image_id = 0;
                        switch (f->action_state) {
                            case FIGURE_ACTION_NATIVE_TRADER_GOING_TO_WAREHOUSE:
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                                    f->action_state = FIGURE_ACTION_NATIVE_TRADER_AT_WAREHOUSE;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                } else if (f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                }
                                if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                                    figure_delete(f);
                                    return;
                                }
                                break;
                            case FIGURE_ACTION_NATIVE_TRADER_RETURNING:
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                                    figure_delete(f);
                                    return;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                }
                                break;
                            case FIGURE_ACTION_NATIVE_TRADER_CREATED:
                                f->is_invisible = 1;
                                f->wait_ticks++;
                                if (f->wait_ticks > 10) {
                                    f->wait_ticks = 0;
                                    struct map_point_t tile;
                                    int building_id = get_closest_warehouse(f, f->x, f->y, 0, &tile);
                                    if (building_id) {
                                        f->action_state = FIGURE_ACTION_NATIVE_TRADER_GOING_TO_WAREHOUSE;
                                        f->destination_building_id = building_id;
                                        f->destination_x = tile.x;
                                        f->destination_y = tile.y;
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                f->image_offset = 0;
                                break;
                            case FIGURE_ACTION_NATIVE_TRADER_AT_WAREHOUSE:
                                f->wait_ticks++;
                                if (f->wait_ticks > 10) {
                                    f->wait_ticks = 0;
                                    if (figure_trade_caravan_can_buy(f, f->destination_building_id, 0)) {
                                        int resource = trader_get_buy_resource(f->destination_building_id, 0);
                                        trader_record_bought_resource(f->trader_id, resource);
                                        f->trader_amount_bought += 3;
                                    } else {
                                        struct map_point_t tile;
                                        int building_id = get_closest_warehouse(f, f->x, f->y, 0, &tile);
                                        if (building_id) {
                                            f->action_state = FIGURE_ACTION_NATIVE_TRADER_GOING_TO_WAREHOUSE;
                                            f->destination_building_id = building_id;
                                            f->destination_x = tile.x;
                                            f->destination_y = tile.y;
                                        } else {
                                            f->action_state = FIGURE_ACTION_NATIVE_TRADER_RETURNING;
                                            f->destination_x = f->source_x;
                                            f->destination_y = f->source_y;
                                        }
                                    }
                                }
                                f->image_offset = 0;
                                break;
                        }
                        dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
                        f->image_id = image_group(GROUP_FIGURE_CARTPUSHER) + dir + 8 * f->image_offset;
                        f->cart_image_id = image_group(GROUP_FIGURE_MIGRANT_CART) + 8 + 8 * f->resource_id;
                        if (f->cart_image_id) {
                            f->cart_image_id += dir;
                            figure_image_set_cart_offset(f, dir);
                        }
                        break;
                    case FIGURE_WOLF:
                    {
                        struct formation_t *m = &herd_formations[f->formation_id];
                        figure_image_increase_offset(f, 12);
                        switch (f->action_state) {
                            case FIGURE_ACTION_HERD_ANIMAL_AT_REST:
                                // replenish wolf pack
                                if (m->num_figures < m->max_figures) {
                                    m->wolf_spawn_delay++;
                                    if (m->wolf_spawn_delay > 1500) {
                                        int spawn_location_x = m->destination_x + HERD_FORMATION_LAYOUT_POSITION_X_OFFSETS[WOLF_PACK_SIZE - 1];
                                        int spawn_location_y = m->destination_y + HERD_FORMATION_LAYOUT_POSITION_Y_OFFSETS[WOLF_PACK_SIZE - 1];
                                        if (!map_terrain_is(map_grid_offset(spawn_location_x, spawn_location_y), TERRAIN_IMPASSABLE)) {
                                            struct figure_t *wolf = figure_create(m->figure_type, spawn_location_x, spawn_location_y, f->direction);
                                            wolf->action_state = FIGURE_ACTION_HERD_ANIMAL_AT_REST;
                                            wolf->formation_id = m->id;
                                            m->wolf_spawn_delay = 0;
                                        }
                                    }
                                }
                                break;
                            case FIGURE_ACTION_HERD_ANIMAL_MOVING:
                            {
                                struct figure_t *target = melee_unit__set_closest_target(f);
                                if (target) {
                                    figure_movement_move_ticks(f, 2);
                                    random_generate_next();
                                    if (random_byte() < 3) {
                                        play_sound_effect(SOUND_EFFECT_WOLF_HOWL);
                                    }
                                    break;
                                } else {
                                    figure_movement_move_ticks(f, 2);
                                    if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                                        f->direction = f->previous_tile_direction;
                                        f->action_state = FIGURE_ACTION_HERD_ANIMAL_AT_REST;
                                        break;
                                    } else if (f->direction == DIR_FIGURE_REROUTE) {
                                        figure_route_remove(f);
                                        break;
                                    } else if (f->routing_path_current_tile > MAX_WOLF_ROAM_DISTANCE * 2) {
                                        figure_route_remove(f);
                                        m->destination_x = f->x;
                                        m->destination_y = f->y;
                                    }
                                }
                                break;
                            }
                        }
                        dir = figure_image_direction(f);
                        if (f->action_state == FIGURE_ACTION_HERD_ANIMAL_AT_REST) {
                            f->image_id = image_group(GROUP_FIGURE_WOLF) + 152 + dir;
                        } else {
                            f->image_id = image_group(GROUP_FIGURE_WOLF) + dir + 8 * f->image_offset;
                        }
                    }
                    break;
                    case FIGURE_SHEEP:
                    {
                        figure_image_increase_offset(f, 6);
                        struct formation_t *m = &herd_formations[f->formation_id];
                        switch (f->action_state) {
                            case FIGURE_ACTION_HERD_ANIMAL_AT_REST:
                                f->wait_ticks++;
                                break;
                            case FIGURE_ACTION_HERD_ANIMAL_MOVING:
                                figure_movement_move_ticks(f, 2);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                                    f->direction = f->previous_tile_direction;
                                    f->action_state = FIGURE_ACTION_HERD_ANIMAL_AT_REST;
                                    f->wait_ticks = f->id & 0x1f;
                                    break;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                    break;
                                } else if (f->routing_path_current_tile > MAX_SHEEP_ROAM_DISTANCE * 2) {
                                    figure_route_remove(f);
                                    m->destination_x = f->x;
                                    m->destination_y = f->y;
                                }
                                break;
                        }
                        dir = figure_image_direction(f);
                        if (f->action_state == FIGURE_ACTION_HERD_ANIMAL_AT_REST) {
                            if (f->id & 3) {
                                f->image_id = image_group(GROUP_FIGURE_SHEEP) + 48 + dir + 8 * SHEEP_IMAGE_OFFSETS[f->wait_ticks & 0x3f];
                            } else {
                                f->image_id = image_group(GROUP_FIGURE_SHEEP) + 96 + dir;
                            }
                        } else {
                            f->image_id = image_group(GROUP_FIGURE_SHEEP) + dir + 8 * f->image_offset;
                        }
                    }
                    break;
                    case FIGURE_ZEBRA:
                    {
                        figure_image_increase_offset(f, 12);
                        struct formation_t *m = &herd_formations[f->formation_id];
                        switch (f->action_state) {
                            case FIGURE_ACTION_HERD_ANIMAL_AT_REST:
                                break;
                            case FIGURE_ACTION_HERD_ANIMAL_MOVING:
                                figure_movement_move_ticks(f, 2);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                                    f->direction = f->previous_tile_direction;
                                    f->action_state = FIGURE_ACTION_HERD_ANIMAL_AT_REST;
                                    break;
                                } else if (f->direction == DIR_FIGURE_REROUTE) {
                                    figure_route_remove(f);
                                    break;
                                } else if (f->routing_path_current_tile > MAX_SHEEP_ROAM_DISTANCE * 2) {
                                    figure_route_remove(f);
                                    m->destination_x = f->x;
                                    m->destination_y = f->y;
                                }
                                break;
                        }
                        dir = figure_image_direction(f);
                        if (f->action_state == FIGURE_ACTION_HERD_ANIMAL_AT_REST) {
                            f->image_id = image_group(GROUP_FIGURE_ZEBRA) + dir;
                        } else {
                            f->image_id = image_group(GROUP_FIGURE_ZEBRA) + dir + 8 * f->image_offset;
                        }
                    }
                    break;
                    case FIGURE_ENEMY_GLADIATOR:
                    {
                        figure_image_increase_offset(f, 12);
                        if (scenario.gladiator_revolt.state == EVENT_FINISHED) {
                            // end of gladiator revolt: kill gladiators
                            f->is_corpse = 1;
                            f->is_targetable = 0;
                            f->wait_ticks = 0;
                            f->direction = 0;
                            clear_targeting_on_unit_death(f);
                        }
                        switch (f->action_state) {
                            case FIGURE_ACTION_NATIVE_CREATED:
                                f->image_offset = 0;
                                f->wait_ticks++;
                                if (f->wait_ticks > 10 + (f->id & 3)) {
                                    f->wait_ticks = 0;
                                    f->action_state = FIGURE_ACTION_NATIVE_ATTACKING;
                                    int x_tile, y_tile;
                                    int building_id = formation_rioter_get_target_building(&x_tile, &y_tile);
                                    if (building_id) {
                                        f->destination_x = x_tile;
                                        f->destination_y = y_tile;
                                        f->destination_building_id = building_id;
                                        figure_route_remove(f);
                                    } else {
                                        figure_delete(f);
                                        return;
                                    }
                                }
                                break;
                            case FIGURE_ACTION_NATIVE_ATTACKING:
                                f->terrain_usage = TERRAIN_USAGE_ENEMY;
                                figure_movement_move_ticks(f, 1);
                                if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                                    f->direction == DIR_FIGURE_REROUTE ||
                                    f->direction == DIR_FIGURE_LOST) {
                                    f->action_state = FIGURE_ACTION_NATIVE_CREATED;
                                }
                                break;
                        }
                        dir = get_direction(f);
                        f->image_id = image_group(GROUP_FIGURE_GLADIATOR) + dir + 8 * f->image_offset;
                    }
                    break;
                    case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
                    case FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN:
                    case FIGURE_ENEMY_BRITON_SWORDSMAN:
                    case FIGURE_ENEMY_CELT_SWORDSMAN:
                    case FIGURE_ENEMY_PICT_SWORDSMAN:
                    case FIGURE_ENEMY_EGYPTIAN_SWORDSMAN:
                    case FIGURE_ENEMY_ETRUSCAN_SWORDSMAN:
                    case FIGURE_ENEMY_SAMNITE_SWORDSMAN:
                    case FIGURE_ENEMY_GAUL_SWORDSMAN:
                    case FIGURE_ENEMY_HELVETIUS_SWORDSMAN:
                    case FIGURE_ENEMY_HUN_SWORDSMAN:
                    case FIGURE_ENEMY_GOTH_SWORDSMAN:
                    case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
                    case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
                    case FIGURE_ENEMY_GREEK_SWORDSMAN:
                    case FIGURE_ENEMY_MACEDONIAN_SWORDSMAN:
                    case FIGURE_ENEMY_PERGAMUM_SWORDSMAN:
                    case FIGURE_ENEMY_IBERIAN_SWORDSMAN:
                    case FIGURE_ENEMY_JUDEAN_SWORDSMAN:
                    case FIGURE_ENEMY_SELEUCID_SWORDSMAN:
                    {
                        figure_image_increase_offset(f, 12);
                        melee_enemy_action(f);
                        dir = get_direction(f);
                        int image_id;
                        if (f->enemy_image_group == ENEMY_IMG_TYPE_BARBARIAN) {
                            image_id = 297;
                        } else {
                            image_id = 449;
                        }
                        f->image_id = image_id + dir + 8 * f->image_offset;
                    }
                    break;
                    case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
                    {
                        figure_image_increase_offset(f, 12);
                        ranged_enemy_action(f);
                        f->image_id = 601 + get_direction(f) + 8 * f->image_offset;
                    }
                    break;
                    case FIGURE_ENEMY_BRITON_CHARIOT:
                    case FIGURE_ENEMY_CELT_CHARIOT:
                    case FIGURE_ENEMY_PICT_CHARIOT:
                    {
                        figure_image_increase_offset(f, 12);
                        melee_enemy_action(f);
                        dir = get_direction(f);
                        f->image_id = 601 + dir + 8 * f->image_offset;
                    }
                    break;
                    case FIGURE_ENEMY_EGYPTIAN_CAMEL:
                    case FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER:
                    case FIGURE_ENEMY_SAMNITE_SPEAR_THROWER:
                    case FIGURE_ENEMY_GREEK_SPEAR_THROWER:
                    case FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER:
                    case FIGURE_ENEMY_PERGAMUM_ARCHER:
                    case FIGURE_ENEMY_IBERIAN_SPEAR_THROWER:
                    case FIGURE_ENEMY_JUDEAN_SPEAR_THROWER:
                    case FIGURE_ENEMY_SELEUCID_SPEAR_THROWER:
                    {
                        figure_image_increase_offset(f, 12);
                        ranged_enemy_action(f);
                        dir = get_direction(f);
                        if (f->action_state == FIGURE_ACTION_ENEMY_REGROUPING) {
                            f->image_id = 697 + dir + 8 * MISSILE_LAUNCHER_OFFSETS[f->attack_image_offset / 2];
                        } else {
                            f->image_id = 601 + dir + 8 * f->image_offset;
                        }
                    }
                    break;
                    case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
                    {
                        figure_image_increase_offset(f, 12);
                        ranged_enemy_action(f);
                        dir = get_direction(f);
                        if (f->action_state == FIGURE_ACTION_ENEMY_REGROUPING) {
                            f->image_id = 545 + dir + 8 * MISSILE_LAUNCHER_OFFSETS[f->attack_image_offset / 2];
                        } else {
                            f->image_id = 449 + dir + 8 * f->image_offset;
                        }
                    }
                    break;
                    case FIGURE_ENEMY_GAUL_AXEMAN:
                    case FIGURE_ENEMY_HELVETIUS_AXEMAN:
                    {
                        figure_image_increase_offset(f, 12);
                        melee_enemy_action(f);
                        dir = get_direction(f);
                        f->image_id = 601 + dir + 8 * f->image_offset;
                    }
                    break;
                    case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
                    case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
                    case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
                    {
                        figure_image_increase_offset(f, 12);
                        ranged_enemy_action(f);
                        dir = get_direction(f);
                        if (f->action_state == FIGURE_ACTION_ENEMY_REGROUPING) {
                            f->image_id = 697 + dir + 8 * MISSILE_LAUNCHER_OFFSETS[f->attack_image_offset / 2];
                        } else {
                            f->image_id = 601 + dir + 8 * f->image_offset;
                        }
                    }
                    break;
                    case FIGURE_ENEMY_CAESAR_JAVELIN:
                    case FIGURE_ENEMY_CAESAR_MOUNTED:
                    case FIGURE_ENEMY_CAESAR_LEGIONARY:
                        figure_image_increase_offset(f, 12);
                        melee_enemy_action(f);
                        dir = get_direction(f);
                        int img_group_base_id = image_group(GROUP_FIGURE_CAESAR_LEGIONARY);

                        if (f->direction == DIR_FIGURE_ATTACK) {
                            f->image_id = img_group_base_id + dir + 8 * ((f->attack_image_offset - 12) / 2);
                        }
                        if (f->figure_is_halted && enemy_formations[f->formation_id].missile_attack_timeout) {
                            f->image_id = img_group_base_id + 144 + dir + 8 * f->image_offset;
                        } else {
                            f->image_id = img_group_base_id + 48 + dir + 8 * f->image_offset;
                        }
                        break;
                    case FIGURE_ARROW:
                        f->use_cross_country = 1;
                        f->progress_on_tile++;
                        int should_die = figure_movement_move_ticks_cross_country(f, 8);
                        int target_id = get_target_on_tile(f);
                        if (target_id) {
                            struct figure_t *target = &figures[target_id];
                            missile_hit_target(f, target);
                            play_sound_effect(SOUND_EFFECT_ARROW_HIT);
                        }
                        dir = (16 + f->direction - 2 * city_view_orientation()) % 16;
                        f->image_id = image_group(GROUP_FIGURE_MISSILE) + 16 + dir;
                        if (f->progress_on_tile > 120) {
                            figure_delete(f);
                        }
                        if (should_die || target_id) {
                            figure_delete(f);
                        }
                        break;
                    case FIGURE_MAP_FLAG:
                        figure_editor_flag_action(f);
                        break;
                    case FIGURE_EXPLOSION:
                        f->use_cross_country = 1;
                        f->progress_on_tile++;
                        if (f->progress_on_tile > 44) {
                            figure_delete(f);
                            return;
                        }
                        figure_movement_move_ticks_cross_country(f, f->speed_multiplier);
                        if (f->progress_on_tile < 48) {
                            f->image_id = image_group(GROUP_FIGURE_EXPLOSION) + CLOUD_IMAGE_OFFSETS[f->progress_on_tile / 2];
                        } else {
                            f->image_id = image_group(GROUP_FIGURE_EXPLOSION) + 7;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
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

int game_animation_should_advance(int speed)
{
    return timers[speed].should_update;
}

static void clear_map_data(void)
{
    map_image_clear();
    map_building_clear();
    memset(terrain_grid.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
    map_aqueduct_clear();
    memset(map_figures.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
    map_property_clear();
    map_sprite_clear();
    map_random_clear();
    map_desirability_clear();
    memset(terrain_elevation.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
    map_road_network_clear();
    map_image_context_init();
    int map_width, map_height;
    map_grid_size(&map_width, &map_height);
    int y_start = (GRID_SIZE - map_height) / 2;
    int x_start = (GRID_SIZE - map_width) / 2;
    for (int y = 0; y < GRID_SIZE; y++) {
        int y_outside_map = y < y_start || y >= y_start + map_height;
        for (int x = 0; x < GRID_SIZE; x++) {
            if (y_outside_map || x < x_start || x >= x_start + map_width) {
                terrain_grid.items[x + GRID_SIZE * y] = TERRAIN_SHRUB | TERRAIN_WATER;
            }
        }
    }
    map_random_init();
    map_property_init_alternate_terrain();
}

static void prepare_map_for_editing(int map_is_new)
{
    image_load_climate(scenario.climate, 1, 0);
    if (map_is_new) {
        empire_load(0);
        empire_object_our_city_set_resources_sell();
    }
    figure_init_scenario();
    for (int id = MAP_FLAG_MIN; id < MAP_FLAG_MAX; id++) {
        figure_create(FIGURE_MAP_FLAG, -1, -1, 0)->resource_id = id;
    }
    figure_create_flotsam();
    map_tiles_update_all_elevation();
    map_tiles_update_all_earthquake();
    map_tiles_update_all_empty_land();
    map_tiles_update_all_roads();
    map_tiles_update_all_plazas();
    map_tiles_update_all_walls();
    map_tiles_update_all_aqueducts(0);
    map_natives_init_editor();
    map_routing_update_all();
    city_view_init();
    game_state_unpause();
}

void game_file_editor_create_scenario(int size)
{
    scenario_editor_create(size);
    map_grid_init(scenario.map.width, scenario.map.height, scenario.map.grid_start, scenario.map.grid_border_size);
    clear_map_data();
    map_image_init_edges();
    city_view_set_camera(76, 152);
    city_view_reset_orientation();
    prepare_map_for_editing(1);
}

static void init_file_piece(struct file_piece_t *piece, int size)
{
    void *data = malloc(size);
    if (data) {
        memset(data, 0, size);
        buffer_init(&piece->buf, data, size);
    }
}

static struct buffer_t *create_scenario_piece(int size)
{
    struct file_piece_t *piece = &scenario_data.pieces[scenario_data.num_pieces++];
    init_file_piece(piece, size);
    return &piece->buf;
}

static void init_scenario_data(void)
{
    if (scenario_data.num_pieces > 0) {
        for (int i = 0; i < scenario_data.num_pieces; i++) {
            buffer_reset(&scenario_data.pieces[i].buf);
        }
        return;
    }
    struct scenario_state_t *state = &scenario_data.state;
    state->graphic_ids = create_scenario_piece(52488);
    state->edge = create_scenario_piece(26244);
    state->terrain = create_scenario_piece(78732);
    state->bitfields = create_scenario_piece(26244);
    state->random = create_scenario_piece(26244);
    state->random_iv = create_scenario_piece(8);
    state->camera = create_scenario_piece(8);
    state->scenario = create_scenario_piece(52424);
    state->empire_objects = create_scenario_piece(20600);
}

static int game_file_io_read_scenario(const char *dir, const char *filename)
{
    log_info("Loading scenario", filename, 0);
    init_scenario_data();
    static char dir_prepended_filepath[DIR_PATH_MAX];
    prepend_dir_to_path(dir, filename, dir_prepended_filepath);
    FILE *fp = fopen(dir_prepended_filepath, "rb");
    if (!fp) {
        return 0;
    }
    for (int i = 0; i < scenario_data.num_pieces; i++) {
        size_t read_size = fread(scenario_data.pieces[i].buf.data, 1, scenario_data.pieces[i].buf.size, fp);
        if (read_size != (unsigned) scenario_data.pieces[i].buf.size) {
            log_error("Unable to load scenario", filename, 0);
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    map_image_load_state(scenario_data.state.graphic_ids);
    map_terrain_load_state(scenario_data.state.terrain);
    map_property_load_state(scenario_data.state.bitfields, scenario_data.state.edge);
    map_random_load_state(scenario_data.state.random);
    city_view_load_scenario_state(scenario_data.state.camera);
    random_load_state(scenario_data.state.random_iv);
    scenario_load_state(scenario_data.state.scenario);
    empire_object_load_state(scenario_data.state.empire_objects);
    return 1;
}

int game_file_editor_load_scenario(const char *dir, const char *scenario_file)
{
    clear_map_data();
    if (!game_file_io_read_scenario(dir, scenario_file)) {
        return 0;
    }
    map_grid_init(scenario.map.width, scenario.map.height, scenario.map.grid_start, scenario.map.grid_border_size);

    prepare_map_for_editing(0);
    return 1;
}

int game_file_editor_write_scenario(const char *dir, const char *scenario_file)
{
    scenario.native_images.hut = image_group(GROUP_EDITOR_BUILDING_NATIVE);
    scenario.native_images.meeting = image_group(GROUP_EDITOR_BUILDING_NATIVE) + 2;
    scenario.native_images.crops = image_group(GROUP_EDITOR_BUILDING_CROPS);
    scenario.empire.distant_battle_roman_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ROMAN_ARMY);
    scenario.empire.distant_battle_enemy_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ENEMY_ARMY);
    log_info("Saving scenario", scenario_file, 0);
    init_scenario_data();
    map_image_save_state(scenario_data.state.graphic_ids);
    map_terrain_save_state(scenario_data.state.terrain);
    map_property_save_state(scenario_data.state.bitfields, scenario_data.state.edge);
    map_random_save_state(scenario_data.state.random);
    city_view_save_scenario_state(scenario_data.state.camera);
    random_save_state(scenario_data.state.random_iv);
    scenario_save_state(scenario_data.state.scenario);
    empire_object_save_state(scenario_data.state.empire_objects);
    static char dir_prepended_filepath[DIR_PATH_MAX];
    prepend_dir_to_path(dir, scenario_file, dir_prepended_filepath);
    FILE *fp = fopen(dir_prepended_filepath, "wb");
    if (!fp) {
        log_error("Unable to save scenario", 0, 0);
        fclose(fp);
        return 0;
    }
    for (int i = 0; i < scenario_data.num_pieces; i++) {
        fwrite(scenario_data.pieces[i].buf.data, 1, scenario_data.pieces[i].buf.size, fp);
    }
    fclose(fp);
    return 1;
}

static struct buffer_t *create_savegame_piece(int size)
{
    struct file_piece_t *piece = &savegame_data.pieces[savegame_data.num_pieces++];
    init_file_piece(piece, size);
    return &piece->buf;
}

static void init_savegame_data(void)
{
    if (savegame_data.num_pieces > 0) {
        for (int i = 0; i < savegame_data.num_pieces; i++) {
            buffer_reset(&savegame_data.pieces[i].buf);
        }
        return;
    }
    struct savegame_state_t *state = &savegame_data.state;
    state->image_grid = create_savegame_piece(52488);
    state->edge_grid = create_savegame_piece(26244);
    state->building_grid = create_savegame_piece(52488);
    state->terrain_grid = create_savegame_piece(78732);
    state->aqueduct_grid = create_savegame_piece(26244);
    state->figure_grid = create_savegame_piece(52488);
    state->bitfields_grid = create_savegame_piece(26244);
    state->sprite_grid = create_savegame_piece(26244);
    state->random_grid = create_savegame_piece(26244);
    state->desirability_grid = create_savegame_piece(26244);
    state->building_damage_grid = create_savegame_piece(26244);
    state->aqueduct_backup_grid = create_savegame_piece(26244);
    state->sprite_backup_grid = create_savegame_piece(26244);
    state->figures = create_savegame_piece(136000);
    state->route_figures = create_savegame_piece(1200);
    state->route_paths = create_savegame_piece(300000);
    state->legion_formations = create_savegame_piece(420);
    state->herd_formations = create_savegame_piece(560);
    state->enemy_formations = create_savegame_piece(7000);
    state->city_data = create_savegame_piece(11551);
    state->player_name = create_savegame_piece(24);
    state->buildings = create_savegame_piece(154000);
    state->city_view_orientation = create_savegame_piece(4);
    state->game_time = create_savegame_piece(20);
    state->building_extra_highest_id_ever = create_savegame_piece(4);
    state->random_iv = create_savegame_piece(8);
    state->city_view_camera = create_savegame_piece(8);
    state->building_count_culture1 = create_savegame_piece(132);
    state->city_graph_order = create_savegame_piece(4);
    state->empire = create_savegame_piece(8);
    state->empire_objects = create_savegame_piece(20600);
    state->building_count_industry = create_savegame_piece(128);
    state->trade_prices = create_savegame_piece(64);
    state->figure_names = create_savegame_piece(76);
    state->culture_coverage = create_savegame_piece(56);
    state->scenario = create_savegame_piece(52424);
    state->messages = create_savegame_piece(14000);
    state->message_extra = create_savegame_piece(12);
    state->population_messages = create_savegame_piece(9);
    state->message_counts = create_savegame_piece(80);
    state->message_delays = create_savegame_piece(80);
    state->building_list_burning_totals = create_savegame_piece(8);
    state->city_sounds = create_savegame_piece(3920);
    state->building_extra_highest_id = create_savegame_piece(4);
    state->figure_traders = create_savegame_piece(4004);
    state->building_list_burning = create_savegame_piece(1000);
    state->building_list_small = create_savegame_piece(1000);
    state->building_list_large = create_savegame_piece(4000);
    state->building_count_military = create_savegame_piece(16);
    state->building_storages = create_savegame_piece(4400);
    state->building_count_culture2 = create_savegame_piece(32);
    state->building_count_support = create_savegame_piece(24);
    state->building_barracks_tower_sentry = create_savegame_piece(4);
    state->building_extra_sequence = create_savegame_piece(4);
    state->routing_counters = create_savegame_piece(8);
    state->building_count_culture3 = create_savegame_piece(40);
    state->building_extra_corrupt_houses = create_savegame_piece(8);
    state->bookmarks = create_savegame_piece(32);
}

int game_file_io_write_saved_game(const char *dir, const char *filename)
{
    init_savegame_data();
    log_info("Saving game", filename, 0);
    scenario_settings_save_state(savegame_data.state.player_name);
    map_image_save_state(savegame_data.state.image_grid);
    map_building_save_state(savegame_data.state.building_grid, savegame_data.state.building_damage_grid);
    map_terrain_save_state(savegame_data.state.terrain_grid);
    map_aqueduct_save_state(savegame_data.state.aqueduct_grid, savegame_data.state.aqueduct_backup_grid);
    map_figure_save_state(savegame_data.state.figure_grid);
    map_sprite_save_state(savegame_data.state.sprite_grid, savegame_data.state.sprite_backup_grid);
    map_property_save_state(savegame_data.state.bitfields_grid, savegame_data.state.edge_grid);
    map_random_save_state(savegame_data.state.random_grid);
    map_desirability_save_state(savegame_data.state.desirability_grid);
    figure_save_state(savegame_data.state.figures);
    figure_route_save_state(savegame_data.state.route_figures, savegame_data.state.route_paths);
    legion_formations_save_state(savegame_data.state.legion_formations);
    herd_formations_save_state(savegame_data.state.herd_formations);
    enemy_formations_save_state(savegame_data.state.enemy_formations);
    city_data_save_state(savegame_data.state.city_data, savegame_data.state.city_graph_order);
    building_save_state(savegame_data.state.buildings,
        savegame_data.state.building_extra_highest_id,
        savegame_data.state.building_extra_highest_id_ever,
        savegame_data.state.building_extra_sequence,
        savegame_data.state.building_extra_corrupt_houses);
    building_barracks_save_state(savegame_data.state.building_barracks_tower_sentry);
    city_view_save_state(savegame_data.state.city_view_orientation, savegame_data.state.city_view_camera);
    game_time_save_state(savegame_data.state.game_time);
    random_save_state(savegame_data.state.random_iv);
    building_count_save_state(savegame_data.state.building_count_industry,
                              savegame_data.state.building_count_culture1,
                              savegame_data.state.building_count_culture2,
                              savegame_data.state.building_count_culture3,
                              savegame_data.state.building_count_military,
                              savegame_data.state.building_count_support);
    empire_save_state(savegame_data.state.empire);
    empire_object_save_state(savegame_data.state.empire_objects);
    trade_prices_save_state(savegame_data.state.trade_prices);
    figure_name_save_state(savegame_data.state.figure_names);
    city_culture_save_state(savegame_data.state.culture_coverage);
    scenario_save_state(savegame_data.state.scenario);
    city_message_save_state(savegame_data.state.messages, savegame_data.state.message_extra,
                            savegame_data.state.message_counts, savegame_data.state.message_delays,
                            savegame_data.state.population_messages);
    save_city_sounds_state(savegame_data.state.city_sounds);
    traders_save_state(savegame_data.state.figure_traders);
    building_list_save_state(savegame_data.state.building_list_small, savegame_data.state.building_list_large,
                             savegame_data.state.building_list_burning, savegame_data.state.building_list_burning_totals);
    building_storage_save_state(savegame_data.state.building_storages);
    map_routing_save_state(savegame_data.state.routing_counters);
    map_bookmark_save_state(savegame_data.state.bookmarks);

    static char dir_prepended_filepath[DIR_PATH_MAX];
    prepend_dir_to_path(dir, filename, dir_prepended_filepath);

    FILE *fp = fopen(dir_prepended_filepath, "wb");
    if (!fp) {
        log_error("Unable to save game", 0, 0);
        return 0;
    }
    for (int i = 0; i < savegame_data.num_pieces; i++) {
        struct file_piece_t *piece = &savegame_data.pieces[i];
        fwrite(piece->buf.data, 1, piece->buf.size, fp);
    }
    fclose(fp);
    return 1;
}

int game_file_io_delete_saved_game(const char *filename)
{
    log_info("Deleting game", filename, 0);
    static char dir_prepended_filepath[DIR_PATH_MAX];
    prepend_dir_to_path(SAVES_DIR_PATH, filename, dir_prepended_filepath);
    int result = remove(dir_prepended_filepath);

    if (result == -1) {
        log_error("Unable to delete game", 0, 0);
        return 0;
    } else {
        return 1;
    }
}

int game_file_start_scenario(const char *scenario_selected)
{
    // assume scenario can be passed in with or without .map extension
    char scenario_file[FILE_NAME_MAX];
    string_copy(scenario_selected, scenario_file, FILE_NAME_MAX - 1);
    if (!file_has_extension(scenario_file, "map")) {
        file_append_extension(scenario_file, "map");

    }
    if (!file_exists(MAPS_DIR_PATH, scenario_file)) {
        return 0;
    }
    map_bookmarks_clear();
    city_victory_reset();
    building_construction_clear_type();
    city_data_init();
    city_message_init_scenario();
    game_state_init();
    game_animation_init();
    initialize_city_sounds();
    building_clear_all();
    building_storage_clear_all();
    figure_init_scenario();
    figure_name_init();
    reset_all_formations();
    figure_route_clear_all();
    game_time_init(2098);
    map_image_clear();
    map_building_clear();
    memset(terrain_grid.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
    map_aqueduct_clear();
    memset(map_figures.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
    map_property_clear();
    map_sprite_clear();
    map_random_clear();
    map_desirability_clear();
    memset(terrain_elevation.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
    map_road_network_clear();
    map_image_context_init();
    map_random_init();
    if (!game_file_load_scenario_data(scenario_file)) {
        return 0;
    }
    // reset trade prices
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        trade_prices[i] = DEFAULT_PRICES[i];
    }
    if (file_has_extension(scenario_file, "map")) {
        file_remove_extension(scenario_file);
    }
    string_copy(scenario_file, scenario.scenario_name, MAX_SCENARIO_NAME);
    map_grid_init(scenario.map.width, scenario.map.height, scenario.map.grid_start, scenario.map.grid_border_size);
    map_tiles_update_all_elevation();
    map_tiles_update_all_earthquake();
    map_tiles_add_entry_exit_flags();
    map_tiles_update_all_empty_land();
    map_tiles_update_all_roads();
    map_tiles_update_all_plazas();
    map_tiles_update_all_walls();
    map_tiles_update_all_aqueducts(0);
    map_natives_init();
    city_view_init();
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        if (scenario.fishing_points[i].x > 0) {
            random_generate_next();
            struct figure_t *fish = figure_create(FIGURE_FISH_GULLS, scenario.fishing_points[i].x, scenario.fishing_points[i].y, DIR_0_TOP);
            fish->terrain_usage = TERRAIN_USAGE_ANY;
            fish->image_offset = random_byte() & 0x1f;
            fish->progress_on_tile = random_byte() & 7;
            figure_movement_set_cross_country_direction(fish,
                fish->cross_country_x, fish->cross_country_y,
                15 * fish->destination_x, 15 * fish->destination_y, 0);
        }
    }
    create_herds();
    figure_create_flotsam();
    map_routing_update_all();
    if (scenario.entry_point.x == -1 || scenario.entry_point.y == -1) {
        scenario.entry_point.x = scenario.map.width - 1;
        scenario.entry_point.y = scenario.map.height / 2;
    }
    if (scenario.exit_point.x == -1 || scenario.exit_point.y == -1) {
        scenario.exit_point.x = scenario.entry_point.x;
        scenario.exit_point.y = scenario.entry_point.y;
    }
    game_time_init(scenario.start_year);
    load_custom_messages();
    empire_init_scenario();
    traders_clear();
    city_military_determine_distant_battle_city();
    image_load_climate(scenario.climate, 0, 0);
    city_data_init_scenario();
    game_state_unpause();
    set_player_name_from_config();
    city_data.ratings.favor = scenario.initial_favor;
    city_data.emperor.personal_savings = scenario.initial_personal_savings;
    city_data.emperor.player_rank = scenario.player_rank;
    city_emperor_set_salary_rank(city_data.emperor.player_rank);
    map_building_menu_items();
    city_message_init_scenario();
    return 1;
}

int game_file_load_scenario_data(const char *scenario_file)
{
    if (!game_file_io_read_scenario(MAPS_DIR_PATH, scenario_file)) {
        return 0;
    }
    city_view_reset_orientation();
    return 1;
}

int game_file_load_saved_game(const char *dir, const char *filename)
{
    init_savegame_data();
    log_info("Loading saved game", filename, 0);
    static char dir_prepended_filepath[DIR_PATH_MAX];
    prepend_dir_to_path(dir, filename, dir_prepended_filepath);
    FILE *fp = fopen(dir_prepended_filepath, "rb");
    if (!fp) {
        log_error("Unable to load game", 0, 0);
        return 0;
    }
    int result = 0;
    for (int i = 0; i < savegame_data.num_pieces; i++) {
        struct file_piece_t *piece = &savegame_data.pieces[i];
        result = fread(piece->buf.data, 1, piece->buf.size, fp) == (unsigned) piece->buf.size;
        // The last piece may be smaller than buf.size
        if (!result && i != (savegame_data.num_pieces - 1)) {
            log_error("Unable to load game", 0, 0);
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    scenario_settings_load_state(savegame_data.state.player_name);
    map_image_load_state(savegame_data.state.image_grid);
    map_building_load_state(savegame_data.state.building_grid, savegame_data.state.building_damage_grid);
    map_terrain_load_state(savegame_data.state.terrain_grid);
    map_aqueduct_load_state(savegame_data.state.aqueduct_grid, savegame_data.state.aqueduct_backup_grid);
    map_figure_load_state(savegame_data.state.figure_grid);
    map_sprite_load_state(savegame_data.state.sprite_grid, savegame_data.state.sprite_backup_grid);
    map_property_load_state(savegame_data.state.bitfields_grid, savegame_data.state.edge_grid);
    map_random_load_state(savegame_data.state.random_grid);
    map_desirability_load_state(savegame_data.state.desirability_grid);
    figure_load_state(savegame_data.state.figures);
    figure_route_load_state(savegame_data.state.route_figures, savegame_data.state.route_paths);
    legion_formations_load_state(savegame_data.state.legion_formations);
    herd_formations_load_state(savegame_data.state.herd_formations);
    enemy_formations_load_state(savegame_data.state.enemy_formations);
    city_data_load_state(savegame_data.state.city_data, savegame_data.state.city_graph_order);
    building_load_state(savegame_data.state.buildings,
                        savegame_data.state.building_extra_highest_id,
                        savegame_data.state.building_extra_highest_id_ever,
                        savegame_data.state.building_extra_sequence,
                        savegame_data.state.building_extra_corrupt_houses);
    building_barracks_load_state(savegame_data.state.building_barracks_tower_sentry);
    city_view_load_state(savegame_data.state.city_view_orientation, savegame_data.state.city_view_camera);
    game_time_load_state(savegame_data.state.game_time);
    random_load_state(savegame_data.state.random_iv);
    building_count_load_state(savegame_data.state.building_count_industry,
                              savegame_data.state.building_count_culture1,
                              savegame_data.state.building_count_culture2,
                              savegame_data.state.building_count_culture3,
                              savegame_data.state.building_count_military,
                              savegame_data.state.building_count_support);
    empire_load_state(savegame_data.state.empire);
    empire_object_load_state(savegame_data.state.empire_objects);
    trade_prices_load_state(savegame_data.state.trade_prices);
    figure_name_load_state(savegame_data.state.figure_names);
    city_culture_load_state(savegame_data.state.culture_coverage);
    scenario_load_state(savegame_data.state.scenario);
    city_message_load_state(savegame_data.state.messages, savegame_data.state.message_extra,
                            savegame_data.state.message_counts, savegame_data.state.message_delays,
                            savegame_data.state.population_messages);
    load_city_sounds_state(savegame_data.state.city_sounds);
    traders_load_state(savegame_data.state.figure_traders);
    building_list_load_state(savegame_data.state.building_list_small, savegame_data.state.building_list_large,
                             savegame_data.state.building_list_burning, savegame_data.state.building_list_burning_totals);
    building_storage_load_state(savegame_data.state.building_storages);
    map_routing_load_state(savegame_data.state.routing_counters);
    map_bookmark_load_state(savegame_data.state.bookmarks);
    scenario.empire.distant_battle_roman_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ROMAN_ARMY);
    scenario.empire.distant_battle_enemy_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ENEMY_ARMY);
    map_grid_init(scenario.map.width, scenario.map.height, scenario.map.grid_start, scenario.map.grid_border_size);
    city_view_init();
    map_routing_update_all();
    map_orientation_update_buildings();
    figure_route_clean();
    map_road_network_update();
    building_granaries_calculate_stocks();
    map_building_menu_items();
    city_message_init_problem_areas();
    initialize_city_sounds();
    building_construction_clear_type();
    game_undo_disable();
    game_state_reset_overlay();
    image_load_climate(scenario.climate, 0, 0);
    city_military_determine_distant_battle_city();
    map_tiles_determine_gardens();
    city_message_clear_scroll();
    game_state_unpause();
    building_storage_reset_building_ids();

    update_music(1);
    return 1;
}

void game_orientation_rotate_left(void)
{
    city_view_rotate_left();
    map_orientation_change(0);
    widget_minimap_invalidate();
    city_warning_show(WARNING_ORIENTATION);
}

void game_orientation_rotate_right(void)
{
    city_view_rotate_right();
    map_orientation_change(1);
    widget_minimap_invalidate();
    city_warning_show(WARNING_ORIENTATION);
}

void game_orientation_rotate_north(void)
{
    switch (city_view_orientation()) {
        case DIR_2_RIGHT:
            city_view_rotate_right();
            map_orientation_change(1);
            break;
        case DIR_4_BOTTOM:
            city_view_rotate_left();
            map_orientation_change(0);
            /* fall through */
        case DIR_6_LEFT:
            city_view_rotate_left();
            map_orientation_change(0);
            break;
        default: // already north
            return;
    }
    widget_minimap_invalidate();
    city_warning_show(WARNING_ORIENTATION);
}

void settings_load(void)
{
    // load defaults
    settings_data.fullscreen = 0;
    settings_data.window_width = 1280;
    settings_data.window_height = 800;
    settings_data.sound_effects.enabled = 1;
    settings_data.sound_effects.volume = 50;
    settings_data.sound_music.enabled = 1;
    settings_data.sound_music.volume = 50;
    settings_data.sound_speech.enabled = 1;
    settings_data.sound_speech.volume = 50;
    settings_data.sound_city.enabled = 1;
    settings_data.sound_city.volume = 50;
    settings_data.game_speed = 80;
    settings_data.scroll_speed = 70;
    settings_data.monthly_autosave = 0;
    settings_data.warnings = 1;
    settings_data.victory_video = 0;
    settings_data.last_advisor = ADVISOR_LABOR;

    int size = io_read_file_into_buffer(SETTINGS_FILE_PATH, settings_data.inf_file, INF_SIZE);
    if (!size) {
        return;
    }
    struct buffer_t buf;
    buffer_init(&buf, settings_data.inf_file, size);
    settings_data.fullscreen = buffer_read_i32(&buf);
    settings_data.window_width = buffer_read_i32(&buf);
    settings_data.window_height = buffer_read_i32(&buf);
    settings_data.sound_effects.enabled = buffer_read_u8(&buf);
    settings_data.sound_effects.volume = buffer_read_i32(&buf);
    settings_data.sound_music.enabled = buffer_read_u8(&buf);
    settings_data.sound_music.volume = buffer_read_i32(&buf);
    settings_data.sound_speech.enabled = buffer_read_u8(&buf);
    settings_data.sound_speech.volume = buffer_read_i32(&buf);
    settings_data.sound_city.enabled = buffer_read_u8(&buf);
    settings_data.sound_city.volume = buffer_read_i32(&buf);
    settings_data.game_speed = buffer_read_i32(&buf);
    settings_data.scroll_speed = buffer_read_i32(&buf);
    settings_data.monthly_autosave = buffer_read_u8(&buf);
    settings_data.warnings = buffer_read_u8(&buf);
    settings_data.victory_video = buffer_read_i32(&buf);
    settings_data.last_advisor = buffer_read_i32(&buf);
    if (settings_data.window_width + settings_data.window_height < 500) {
        // most likely migration from Caesar 3
        settings_data.window_width = 800;
        settings_data.window_height = 600;
    }
    if (settings_data.last_advisor <= ADVISOR_NONE || settings_data.last_advisor > ADVISOR_CHIEF) {
        settings_data.last_advisor = ADVISOR_LABOR;
    }
}

void settings_save(void)
{
    struct buffer_t b;
    struct buffer_t *buf = &b;
    buffer_init(buf, settings_data.inf_file, INF_SIZE);

    buffer_write_i32(buf, settings_data.fullscreen);
    buffer_write_i32(buf, settings_data.window_width);
    buffer_write_i32(buf, settings_data.window_height);

    buffer_write_u8(buf, settings_data.sound_effects.enabled);
    buffer_write_i32(buf, settings_data.sound_effects.volume);
    buffer_write_u8(buf, settings_data.sound_music.enabled);
    buffer_write_i32(buf, settings_data.sound_music.volume);
    buffer_write_u8(buf, settings_data.sound_speech.enabled);
    buffer_write_i32(buf, settings_data.sound_speech.volume);
    buffer_write_u8(buf, settings_data.sound_city.enabled);
    buffer_write_i32(buf, settings_data.sound_city.volume);

    buffer_write_i32(buf, settings_data.game_speed);
    buffer_write_i32(buf, settings_data.scroll_speed);

    buffer_write_u8(buf, settings_data.monthly_autosave);
    buffer_write_u8(buf, settings_data.warnings);
    buffer_write_i32(buf, settings_data.victory_video);
    buffer_write_i32(buf, settings_data.last_advisor);

    io_write_buffer_to_file(SETTINGS_FILE_PATH, settings_data.inf_file, INF_SIZE);
}

int setting_fullscreen(void)
{
    return settings_data.fullscreen;
}

void setting_window(int *width, int *height)
{
    *width = settings_data.window_width;
    *height = settings_data.window_height;
}

void setting_set_display(int fullscreen, int width, int height)
{
    settings_data.fullscreen = fullscreen;
    if (!fullscreen) {
        settings_data.window_width = width;
        settings_data.window_height = height;
    }
}

struct set_sound_t *get_sound(int type)
{
    switch (type) {
        case SOUND_MUSIC: return &settings_data.sound_music;
        case SOUND_EFFECTS: return &settings_data.sound_effects;
        case SOUND_SPEECH: return &settings_data.sound_speech;
        case SOUND_CITY: return &settings_data.sound_city;
        default: return 0;
    }
}

void setting_toggle_sound_enabled(int type)
{
    struct set_sound_t *sound = get_sound(type);
    if (sound) {
        sound->enabled = sound->enabled ? 0 : 1;
    }
}

void setting_increase_sound_volume(int type)
{
    struct set_sound_t *sound = get_sound(type);
    if (sound) {
        sound->volume = calc_bound(sound->volume + 1, 0, 100);
    }
}

void setting_decrease_sound_volume(int type)
{
    struct set_sound_t *sound = get_sound(type);
    if (sound) {
        sound->volume = calc_bound(sound->volume - 1, 0, 100);
    }
}

int setting_game_speed(void)
{
    return settings_data.game_speed;
}

void setting_decrease_game_speed(void)
{
    if (settings_data.game_speed > 100) {
        settings_data.game_speed -= 100;
    } else {
        settings_data.game_speed = calc_bound(settings_data.game_speed - 10, 10, 100);
    }
}

void setting_increase_game_speed(void)
{
    if (settings_data.game_speed >= 100) {
        if (settings_data.game_speed < 500) {
            settings_data.game_speed += 100;
        }
    } else {
        settings_data.game_speed = calc_bound(settings_data.game_speed + 10, 10, 100);
    }
}

int setting_scroll_speed(void)
{
    return settings_data.scroll_speed;
}

void setting_increase_scroll_speed(void)
{
    settings_data.scroll_speed = calc_bound(settings_data.scroll_speed + 10, 0, 100);
}

void setting_decrease_scroll_speed(void)
{
    settings_data.scroll_speed = calc_bound(settings_data.scroll_speed - 10, 0, 100);
}

int setting_warnings(void)
{
    return settings_data.warnings;
}

void setting_toggle_warnings(void)
{
    settings_data.warnings = settings_data.warnings ? 0 : 1;
}

int setting_monthly_autosave(void)
{
    return settings_data.monthly_autosave;
}

void setting_toggle_monthly_autosave(void)
{
    settings_data.monthly_autosave = settings_data.monthly_autosave ? 0 : 1;
}

int setting_victory_video(void)
{
    settings_data.victory_video = settings_data.victory_video ? 0 : 1;
    return settings_data.victory_video;
}

int setting_last_advisor(void)
{
    return settings_data.last_advisor;
}

void setting_set_last_advisor(int advisor)
{
    settings_data.last_advisor = advisor;
}

int game_state_is_paused(void)
{
    return state_data.paused;
}

void game_state_unpause(void)
{
    state_data.paused = 0;
}

void game_state_toggle_paused(void)
{
    state_data.paused = state_data.paused ? 0 : 1;
}

int game_state_overlay(void)
{
    return state_data.current_overlay;
}

void game_state_reset_overlay(void)
{
    state_data.current_overlay = OVERLAY_NONE;
    state_data.previous_overlay = OVERLAY_NONE;
}

void game_state_set_overlay(int overlay)
{
    if (overlay == OVERLAY_NONE) {
        state_data.previous_overlay = state_data.current_overlay;
    } else {
        state_data.previous_overlay = OVERLAY_NONE;
    }
    state_data.current_overlay = overlay;
}

void building_house_change_to(struct building_t *house, int type)
{
    house->type = type;
    house->subtype.house_level = house->type - BUILDING_HOUSE_SMALL_TENT;
    int image_id = image_group(HOUSE_IMAGE[house->subtype.house_level].group);
    if (house->house_is_merged) {
        image_id += 4;
        if (HOUSE_IMAGE[house->subtype.house_level].offset) {
            image_id += 1;
        }
    } else {
        image_id += HOUSE_IMAGE[house->subtype.house_level].offset;
        image_id += map_random_get(house->grid_offset) & (HOUSE_IMAGE[house->subtype.house_level].num_types - 1);
    }
    map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);
}

int game_time_tick(void)
{
    return time_data.tick;
}

int game_time_day(void)
{
    return time_data.day;
}

int game_time_month(void)
{
    return time_data.month;
}

int game_time_year(void)
{
    return time_data.year;
}

void game_time_save_state(struct buffer_t *buf)
{
    buffer_write_i32(buf, time_data.tick);
    buffer_write_i32(buf, time_data.day);
    buffer_write_i32(buf, time_data.month);
    buffer_write_i32(buf, time_data.year);
    buffer_write_i32(buf, time_data.total_days);
}

void game_time_load_state(struct buffer_t *buf)
{
    time_data.tick = buffer_read_i32(buf);
    time_data.day = buffer_read_i32(buf);
    time_data.month = buffer_read_i32(buf);
    time_data.year = buffer_read_i32(buf);
    time_data.total_days = buffer_read_i32(buf);
}

int game_can_undo(void)
{
    return undo_data.ready && undo_data.available;
}

void game_undo_disable(void)
{
    undo_data.available = 0;
}

void game_undo_add_building(struct building_t *b)
{
    if (b->id <= 0) {
        return;
    }
    undo_data.num_buildings = 0;
    int is_on_list = 0;
    for (int i = 0; i < MAX_UNDO_BUILDINGS; i++) {
        if (undo_data.buildings[i].id) {
            undo_data.num_buildings++;
        }
        if (undo_data.buildings[i].id == b->id) {
            is_on_list = 1;
        }
    }
    if (!is_on_list) {
        for (int i = 0; i < MAX_UNDO_BUILDINGS; i++) {
            if (!undo_data.buildings[i].id) {
                undo_data.num_buildings++;
                memcpy(&undo_data.buildings[i], b, sizeof(struct building_t));
                return;
            }
        }
        undo_data.available = 0;
    }
}

int game_undo_contains_building(int building_id)
{
    if (building_id <= 0 || !game_can_undo()) {
        return 0;
    }
    if (undo_data.num_buildings <= 0) {
        return 0;
    }
    for (int i = 0; i < MAX_UNDO_BUILDINGS; i++) {
        if (undo_data.buildings[i].id == building_id) {
            return 1;
        }
    }
    return 0;
}

int game_undo_start_build(int type)
{
    undo_data.ready = 0;
    undo_data.available = 1;
    undo_data.timeout_ticks = 0;
    undo_data.building_cost = 0;
    undo_data.type = type;
    clear_buildings();
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_UNDO) {
            undo_data.available = 0;
            return 0;
        }
        if (b->state == BUILDING_STATE_DELETED_BY_PLAYER) {
            undo_data.available = 0;
        }
    }
    map_image_backup();
    map_terrain_backup();
    map_aqueduct_backup();
    map_property_backup();
    map_sprite_backup();
    return 1;
}

void game_undo_restore_building_state(void)
{
    for (int i = 0; i < undo_data.num_buildings; i++) {
        if (undo_data.buildings[i].id) {
            struct building_t *b = &all_buildings[undo_data.buildings[i].id];
            if (b->state == BUILDING_STATE_DELETED_BY_PLAYER) {
                b->state = BUILDING_STATE_IN_USE;
            }
            b->is_deleted = 0;
        }
    }
    clear_buildings();
}

static void restore_map_images(void)
{
    int map_width, map_height;
    map_grid_size(&map_width, &map_height);
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (!map_building_at(grid_offset)) {
                map_image_restore_at(grid_offset);
            }
        }
    }
}

void game_undo_restore_map(int include_properties)
{
    map_terrain_restore();
    map_aqueduct_restore();
    if (include_properties) {
        map_property_restore();
    }
    restore_map_images();
}

void game_undo_finish_build(int cost)
{
    undo_data.ready = 1;
    undo_data.timeout_ticks = 500;
    undo_data.building_cost = cost;
    window_invalidate();
}

void game_undo_perform(void)
{
    if (!game_can_undo()) {
        return;
    }
    undo_data.available = 0;
    city_finance_process_construction(-undo_data.building_cost);
    if (undo_data.type == BUILDING_CLEAR_LAND) {
        for (int i = 0; i < undo_data.num_buildings; i++) {
            if (undo_data.buildings[i].id) {
                struct building_t *b = &all_buildings[undo_data.buildings[i].id];
                memcpy(b, &undo_data.buildings[i], sizeof(struct building_t));
                if (b->type == BUILDING_WAREHOUSE || b->type == BUILDING_GRANARY) {
                    if (!building_storage_restore(b->storage_id)) {
                        building_storage_reset_building_ids();
                    }
                }
                if (b->id) {
                    if (building_is_farm(b->type)) {
                        int image_offset;
                        switch (b->type) {
                            default:
                            case BUILDING_WHEAT_FARM: image_offset = 0; break;
                            case BUILDING_VEGETABLE_FARM: image_offset = 5; break;
                            case BUILDING_FRUIT_FARM: image_offset = 10; break;
                            case BUILDING_OLIVE_FARM: image_offset = 15; break;
                            case BUILDING_VINES_FARM: image_offset = 20; break;
                            case BUILDING_PIG_FARM: image_offset = 25; break;
                        }
                        map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + image_offset, 0);
                    } else {
                        int size = b->size;
                        if (building_is_house(b->type) && b->house_is_merged) {
                            size = 2;
                        }
                        map_building_tiles_add(b->id, b->x, b->y, size, 0, 0);
                        if (b->type == BUILDING_WHARF) {
                            b->data.industry.fishing_boat_id = 0;
                        }
                    }
                    b->state = BUILDING_STATE_IN_USE;
                }
            }
        }
        map_terrain_restore();
        map_aqueduct_restore();
        map_sprite_restore();
        map_image_restore();
        map_property_restore();
        map_property_clear_constructing_and_deleted();
    } else if (undo_data.type == BUILDING_AQUEDUCT || undo_data.type == BUILDING_ROAD ||
            undo_data.type == BUILDING_WALL) {
        map_terrain_restore();
        map_aqueduct_restore();
        restore_map_images();
    } else if (undo_data.type == BUILDING_LOW_BRIDGE || undo_data.type == BUILDING_SHIP_BRIDGE) {
        map_terrain_restore();
        map_sprite_restore();
        restore_map_images();
    } else if (undo_data.type == BUILDING_PLAZA || undo_data.type == BUILDING_GARDENS) {
        map_terrain_restore();
        map_aqueduct_restore();
        map_property_restore();
        restore_map_images();
    } else if (undo_data.num_buildings) {
        for (int i = 0; i < undo_data.num_buildings; i++) {
            if (undo_data.buildings[i].id) {
                struct building_t *b = &all_buildings[undo_data.buildings[i].id];
                if (b->type == BUILDING_ORACLE || (b->type >= BUILDING_LARGE_TEMPLE_CERES && b->type <= BUILDING_LARGE_TEMPLE_VENUS)) {
                    int building_id = city_data.resource.last_used_warehouse;
                    for (int j = 1; j < MAX_BUILDINGS; j++) {
                        building_id++;
                        if (building_id >= MAX_BUILDINGS) {
                            building_id = 1;
                        }
                        struct building_t *bb = &all_buildings[building_id];
                        if (bb->state == BUILDING_STATE_IN_USE && bb->type == BUILDING_WAREHOUSE) {
                            city_data.resource.last_used_warehouse = building_id;
                            building_warehouse_add_resource(b, RESOURCE_MARBLE);
                            building_warehouse_add_resource(b, RESOURCE_MARBLE);
                            break;
                        }
                    }
                }
                b->state = BUILDING_STATE_UNDO;
                building_update_state();
            }
        }
    }
    map_routing_update_land();
    map_routing_update_walls();
    undo_data.num_buildings = 0;
}
