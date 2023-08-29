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
#include "figure/name.h"
#include "figure/route.h"
#include "figure/trader.h"
#include "figuretype/animal.h"
#include "figuretype/cartpusher.h"
#include "figuretype/crime.h"
#include "figuretype/docker.h"
#include "figuretype/editor.h"
#include "figuretype/enemy.h"
#include "figuretype/entertainer.h"
#include "figuretype/maintenance.h"
#include "figuretype/market.h"
#include "figuretype/migrant.h"
#include "figuretype/missile.h"
#include "figuretype/service.h"
#include "figuretype/soldier.h"
#include "figuretype/trader.h"
#include "figuretype/wall.h"
#include "figuretype/water.h"
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
            case 45: figure_generate_criminals(); break;
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
                    for (int j = 0; j < MAX_OBJECTS; j++) {
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
                        figure_immigrant_action(f);
                        break;
                    case FIGURE_EMIGRANT:
                        figure_emigrant_action(f);
                        break;
                    case FIGURE_HOMELESS:
                        figure_homeless_action(f);
                        break;
                    case FIGURE_PATRICIAN:
                        figure_patrician_action(f);
                        break;
                    case FIGURE_CART_PUSHER:
                        figure_cartpusher_action(f);
                        break;
                    case FIGURE_LABOR_SEEKER:
                        figure_labor_seeker_action(f);
                        break;
                    case FIGURE_BARBER:
                        figure_barber_action(f);
                        break;
                    case FIGURE_BATHHOUSE_WORKER:
                        figure_bathhouse_worker_action(f);
                        break;
                    case FIGURE_DOCTOR:
                    case FIGURE_SURGEON:
                        figure_doctor_action(f);
                        break;
                    case FIGURE_PRIEST:
                        figure_priest_action(f);
                        break;
                    case FIGURE_SCHOOL_CHILD:
                        figure_school_child_action(f);
                        break;
                    case FIGURE_TEACHER:
                        figure_teacher_action(f);
                        break;
                    case FIGURE_LIBRARIAN:
                        figure_librarian_action(f);
                        break;
                    case FIGURE_MISSIONARY:
                        figure_missionary_action(f);
                        break;
                    case FIGURE_ACTOR:
                    case FIGURE_GLADIATOR:
                    case FIGURE_LION_TAMER:
                    case FIGURE_CHARIOTEER:
                        figure_entertainer_action(f);
                        break;
                    case FIGURE_HIPPODROME_HORSES:
                        figure_hippodrome_horse_action(f);
                        break;
                    case FIGURE_TAX_COLLECTOR:
                        figure_tax_collector_action(f);
                        break;
                    case FIGURE_ENGINEER:
                        figure_engineer_action(f);
                        break;
                    case FIGURE_FISHING_BOAT:
                        figure_fishing_boat_action(f);
                        break;
                    case FIGURE_FISH_GULLS:
                        figure_seagulls_action(f);
                        break;
                    case FIGURE_SHIPWRECK:
                        figure_shipwreck_action(f);
                        break;
                    case FIGURE_DOCKER:
                        figure_docker_action(f);
                        break;
                    case FIGURE_FLOTSAM:
                        if (scenario.river_exit_point.x != -1 && scenario.river_exit_point.y != -1) {
                            figure_flotsam_action(f);
                        }
                        break;
                    case FIGURE_BALLISTA:
                        figure_ballista_action(f);
                        break;
                    case FIGURE_BOLT:
                        figure_bolt_action(f);
                        break;
                    case FIGURE_TOWER_SENTRY:
                        figure_tower_sentry_action(f);
                        break;
                    case FIGURE_JAVELIN:
                        figure_javelin_action(f);
                        break;
                    case FIGURE_PREFECT:
                        figure_prefect_action(f);
                        break;
                    case FIGURE_FORT_STANDARD:
                        figure_military_standard_action(f);
                        break;
                    case FIGURE_FORT_JAVELIN:
                    case FIGURE_FORT_MOUNTED:
                    case FIGURE_FORT_LEGIONARY:
                        figure_soldier_action(f);
                        break;
                    case FIGURE_MARKET_BUYER:
                        figure_market_buyer_action(f);
                        break;
                    case FIGURE_MARKET_TRADER:
                        figure_market_trader_action(f);
                        break;
                    case FIGURE_DELIVERY_BOY:
                        figure_delivery_boy_action(f);
                        break;
                    case FIGURE_WAREHOUSEMAN:
                        figure_warehouseman_action(f);
                        break;
                    case FIGURE_PROTESTER:
                        figure_protestor_action(f);
                        break;
                    case FIGURE_CRIMINAL:
                        figure_criminal_action(f);
                        break;
                    case FIGURE_RIOTER:
                        figure_rioter_action(f);
                        break;
                    case FIGURE_TRADE_CARAVAN:
                        figure_trade_caravan_action(f);
                        break;
                    case FIGURE_TRADE_CARAVAN_DONKEY:
                        figure_trade_caravan_donkey_action(f);
                        break;
                    case FIGURE_TRADE_SHIP:
                        figure_trade_ship_action(f);
                        break;
                    case FIGURE_INDIGENOUS_NATIVE:
                        figure_indigenous_native_action(f);
                        break;
                    case FIGURE_NATIVE_TRADER:
                        figure_native_trader_action(f);
                        break;
                    case FIGURE_WOLF:
                        figure_wolf_action(f);
                        break;
                    case FIGURE_SHEEP:
                        figure_sheep_action(f);
                        break;
                    case FIGURE_ZEBRA:
                        figure_zebra_action(f);
                        break;
                    case FIGURE_ENEMY_GLADIATOR:
                        figure_enemy_gladiator_action(f);
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
                        figure_enemy_swordsman_action(f);
                        break;
                    case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
                        figure_enemy_elephant_action(f);
                        break;
                    case FIGURE_ENEMY_BRITON_CHARIOT:
                    case FIGURE_ENEMY_CELT_CHARIOT:
                    case FIGURE_ENEMY_PICT_CHARIOT:
                        figure_enemy_chariot_action(f);
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
                        figure_enemy_heavy_ranged_action(f);
                        break;
                    case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
                        figure_enemy_light_ranged_spearman_action(f);
                        break;
                    case FIGURE_ENEMY_GAUL_AXEMAN:
                    case FIGURE_ENEMY_HELVETIUS_AXEMAN:
                        figure_enemy_axeman_action(f);
                        break;
                    case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
                    case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
                    case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
                        figure_enemy_mounted_archer_action(f);
                        break;
                    case FIGURE_ENEMY_CAESAR_JAVELIN:
                    case FIGURE_ENEMY_CAESAR_MOUNTED:
                    case FIGURE_ENEMY_CAESAR_LEGIONARY:
                        figure_enemy_caesar_legionary_action(f);
                        break;
                    case FIGURE_ARROW:
                        figure_arrow_action(f);
                        break;
                    case FIGURE_MAP_FLAG:
                        figure_editor_flag_action(f);
                        break;
                    case FIGURE_EXPLOSION:
                        figure_explosion_cloud_action(f);
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
    figure_create_editor_flags();
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
    figure_create_fishing_points();
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
