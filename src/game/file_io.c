#include "file_io.h"

#include "building/barracks.h"
#include "building/count.h"
#include "building/list.h"
#include "building/storage.h"
#include "city/culture.h"
#include "city/data.h"
#include "core/file.h"
#include "core/log.h"
#include "city/message.h"
#include "city/view.h"
#include "core/dir.h"
#include "core/random.h"
#include "core/zip.h"
#include "empire/empire.h"
#include "empire/object.h"
#include "empire/trade_prices.h"
#include "empire/trade_route.h"
#include "figure/enemy_army.h"
#include "figure/formation.h"
#include "figure/name.h"
#include "figure/route.h"
#include "figure/trader.h"
#include "game/time.h"
#include "map/aqueduct.h"
#include "map/bookmark.h"
#include "map/building.h"
#include "map/desirability.h"
#include "map/elevation.h"
#include "map/figure.h"
#include "map/image.h"
#include "map/property.h"
#include "map/random.h"
#include "map/routing.h"
#include "map/sprite.h"
#include "map/terrain.h"
#include "scenario/editor_events.h"
#include "scenario/scenario.h"
#include "sound/city.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMPRESS_BUFFER_SIZE 600000
#define UNCOMPRESSED 0x80000000

static const int SAVE_GAME_VERSION = 0x66;

static char compress_buffer[COMPRESS_BUFFER_SIZE];

static int savegame_version;

typedef struct {
    buffer buf;
    int compressed;
} file_piece;

typedef struct {
    buffer *graphic_ids;
    buffer *edge;
    buffer *terrain;
    buffer *bitfields;
    buffer *random;
    buffer *elevation;
    buffer *random_iv;
    buffer *camera;
    buffer *scenario;
    buffer *empire_object;
} scenario_state;

static struct {
    int num_pieces;
    file_piece pieces[11];
    scenario_state state;
} scenario_data = { 0 };

typedef struct {
    buffer *file_version;
    buffer *image_grid;
    buffer *edge_grid;
    buffer *building_grid;
    buffer *terrain_grid;
    buffer *aqueduct_grid;
    buffer *figure_grid;
    buffer *bitfields_grid;
    buffer *sprite_grid;
    buffer *random_grid;
    buffer *desirability_grid;
    buffer *elevation_grid;
    buffer *building_damage_grid;
    buffer *aqueduct_backup_grid;
    buffer *sprite_backup_grid;
    buffer *figures;
    buffer *route_figures;
    buffer *route_paths;
    buffer *formations;
    buffer *formation_totals;
    buffer *city_data;
    buffer *player_name;
    buffer *buildings;
    buffer *city_view_orientation;
    buffer *game_time;
    buffer *building_extra_highest_id_ever;
    buffer *random_iv;
    buffer *city_view_camera;
    buffer *building_count_culture1;
    buffer *city_graph_order;
    buffer *empire;
    buffer *empire_objects;
    buffer *building_count_industry;
    buffer *trade_prices;
    buffer *figure_names;
    buffer *culture_coverage;
    buffer *scenario;
    buffer *earthquake;
    buffer *messages;
    buffer *message_extra;
    buffer *population_messages;
    buffer *message_counts;
    buffer *message_delays;
    buffer *building_list_burning_totals;
    buffer *figure_sequence;
    buffer *city_sounds;
    buffer *building_extra_highest_id;
    buffer *figure_traders;
    buffer *building_list_burning;
    buffer *building_list_small;
    buffer *building_list_large;
    buffer *building_count_military;
    buffer *enemy_army_totals;
    buffer *building_storages;
    buffer *building_count_culture2;
    buffer *building_count_support;
    buffer *trade_route_limit;
    buffer *trade_route_traded;
    buffer *building_barracks_tower_sentry;
    buffer *building_extra_sequence;
    buffer *routing_counters;
    buffer *building_count_culture3;
    buffer *enemy_armies;
    buffer *city_entry_exit_xy;
    buffer *building_extra_corrupt_houses;
    buffer *bookmarks;
    buffer *city_entry_exit_grid_offset;
} savegame_state;

static struct {
    int num_pieces;
    file_piece pieces[100];
    savegame_state state;
} savegame_data = { 0 };

static void init_file_piece(file_piece *piece, int size, int compressed)
{
    piece->compressed = compressed;
    void *data = malloc(size);
    if (data) {
        memset(data, 0, size);
        buffer_init(&piece->buf, data, size);
    }
}

static buffer *create_scenario_piece(int size)
{
    file_piece *piece = &scenario_data.pieces[scenario_data.num_pieces++];
    init_file_piece(piece, size, 0);
    return &piece->buf;
}

static buffer *create_savegame_piece(int size, int compressed)
{
    file_piece *piece = &savegame_data.pieces[savegame_data.num_pieces++];
    init_file_piece(piece, size, compressed);
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
    scenario_state *state = &scenario_data.state;
    state->graphic_ids = create_scenario_piece(52488);
    state->edge = create_scenario_piece(26244);
    state->terrain = create_scenario_piece(52488);
    state->bitfields = create_scenario_piece(26244);
    state->random = create_scenario_piece(26244);
    state->elevation = create_scenario_piece(26244);
    state->random_iv = create_scenario_piece(8);
    state->camera = create_scenario_piece(8);
    state->scenario = create_scenario_piece(51934);
    state->empire_object = create_scenario_piece(20600);
}

static void init_savegame_data(void)
{
    if (savegame_data.num_pieces > 0) {
        for (int i = 0; i < savegame_data.num_pieces; i++) {
            buffer_reset(&savegame_data.pieces[i].buf);
        }
        return;
    }
    savegame_state *state = &savegame_data.state;
    state->file_version = create_savegame_piece(4, 0);
    state->image_grid = create_savegame_piece(52488, 1);
    state->edge_grid = create_savegame_piece(26244, 1);
    state->building_grid = create_savegame_piece(52488, 1);
    state->terrain_grid = create_savegame_piece(52488, 1);
    state->aqueduct_grid = create_savegame_piece(26244, 1);
    state->figure_grid = create_savegame_piece(52488, 1);
    state->bitfields_grid = create_savegame_piece(26244, 1);
    state->sprite_grid = create_savegame_piece(26244, 1);
    state->random_grid = create_savegame_piece(26244, 0);
    state->desirability_grid = create_savegame_piece(26244, 1);
    state->elevation_grid = create_savegame_piece(26244, 1);
    state->building_damage_grid = create_savegame_piece(26244, 1);
    state->aqueduct_backup_grid = create_savegame_piece(26244, 1);
    state->sprite_backup_grid = create_savegame_piece(26244, 1);
    state->figures = create_savegame_piece(132000, 1);
    state->route_figures = create_savegame_piece(1200, 1);
    state->route_paths = create_savegame_piece(300000, 1);
    state->formations = create_savegame_piece(5400, 1);
    state->formation_totals = create_savegame_piece(12, 0);
    state->city_data = create_savegame_piece(11624, 1);
    state->player_name = create_savegame_piece(24, 0);
    state->buildings = create_savegame_piece(164000, 1);
    state->city_view_orientation = create_savegame_piece(4, 0);
    state->game_time = create_savegame_piece(20, 0);
    state->building_extra_highest_id_ever = create_savegame_piece(4, 0);
    state->random_iv = create_savegame_piece(8, 0);
    state->city_view_camera = create_savegame_piece(8, 0);
    state->building_count_culture1 = create_savegame_piece(132, 0);
    state->city_graph_order = create_savegame_piece(4, 0);
    state->empire = create_savegame_piece(8, 0);
    state->empire_objects = create_savegame_piece(20600, 1);
    state->building_count_industry = create_savegame_piece(128, 0);
    state->trade_prices = create_savegame_piece(128, 0);
    state->figure_names = create_savegame_piece(84, 0);
    state->culture_coverage = create_savegame_piece(56, 0);
    state->scenario = create_savegame_piece(51934, 0);
    state->earthquake = create_savegame_piece(52, 0);
    state->messages = create_savegame_piece(14000, 1);
    state->message_extra = create_savegame_piece(12, 0);
    state->population_messages = create_savegame_piece(9, 0);
    state->message_counts = create_savegame_piece(80, 0);
    state->message_delays = create_savegame_piece(80, 0);
    state->building_list_burning_totals = create_savegame_piece(8, 0);
    state->figure_sequence = create_savegame_piece(4, 0);
    state->city_sounds = create_savegame_piece(3920, 0);
    state->building_extra_highest_id = create_savegame_piece(4, 0);
    state->figure_traders = create_savegame_piece(4804, 0);
    state->building_list_burning = create_savegame_piece(1000, 1);
    state->building_list_small = create_savegame_piece(1000, 1);
    state->building_list_large = create_savegame_piece(4000, 1);
    state->building_count_military = create_savegame_piece(16, 0);
    state->enemy_army_totals = create_savegame_piece(20, 0);
    state->building_storages = create_savegame_piece(4400, 0);
    state->building_count_culture2 = create_savegame_piece(32, 0);
    state->building_count_support = create_savegame_piece(24, 0);
    state->trade_route_limit = create_savegame_piece(1280, 1);
    state->trade_route_traded = create_savegame_piece(1280, 1);
    state->building_barracks_tower_sentry = create_savegame_piece(4, 0);
    state->building_extra_sequence = create_savegame_piece(4, 0);
    state->routing_counters = create_savegame_piece(8, 0);
    state->building_count_culture3 = create_savegame_piece(40, 0);
    state->enemy_armies = create_savegame_piece(900, 0);
    state->city_entry_exit_xy = create_savegame_piece(16, 0);
    state->building_extra_corrupt_houses = create_savegame_piece(8, 0);
    state->bookmarks = create_savegame_piece(32, 0);
    state->city_entry_exit_grid_offset = create_savegame_piece(8, 0);
}

static void scenario_load_from_state(scenario_state *file)
{
    map_image_load_state(file->graphic_ids);
    map_terrain_load_state(file->terrain);
    map_property_load_state(file->bitfields, file->edge);
    map_random_load_state(file->random);
    map_elevation_load_state(file->elevation);
    city_view_load_scenario_state(file->camera);

    random_load_state(file->random_iv);

    scenario_load_state(file->scenario);
    empire_object_load_state(file->empire_object);
}

static void scenario_save_to_state(scenario_state *file)
{
    map_image_save_state(file->graphic_ids);
    map_terrain_save_state(file->terrain);
    map_property_save_state(file->bitfields, file->edge);
    map_random_save_state(file->random);
    map_elevation_save_state(file->elevation);
    city_view_save_scenario_state(file->camera);

    random_save_state(file->random_iv);

    scenario_save_state(file->scenario);
    empire_object_save_state(file->empire_object);
}

static void savegame_load_from_state(savegame_state *state)
{
    savegame_version = buffer_read_i32(state->file_version);

    scenario_settings_load_state(state->player_name);

    map_image_load_state(state->image_grid);
    map_building_load_state(state->building_grid, state->building_damage_grid);
    map_terrain_load_state(state->terrain_grid);
    map_aqueduct_load_state(state->aqueduct_grid, state->aqueduct_backup_grid);
    map_figure_load_state(state->figure_grid);
    map_sprite_load_state(state->sprite_grid, state->sprite_backup_grid);
    map_property_load_state(state->bitfields_grid, state->edge_grid);
    map_random_load_state(state->random_grid);
    map_desirability_load_state(state->desirability_grid);
    map_elevation_load_state(state->elevation_grid);

    figure_load_state(state->figures, state->figure_sequence);
    figure_route_load_state(state->route_figures, state->route_paths);
    formations_load_state(state->formations, state->formation_totals);

    city_data_load_state(state->city_data,
                         state->city_graph_order,
                         state->city_entry_exit_xy,
                         state->city_entry_exit_grid_offset);

    building_load_state(state->buildings,
                        state->building_extra_highest_id,
                        state->building_extra_highest_id_ever,
                        state->building_extra_sequence,
                        state->building_extra_corrupt_houses);
    building_barracks_load_state(state->building_barracks_tower_sentry);
    city_view_load_state(state->city_view_orientation, state->city_view_camera);
    game_time_load_state(state->game_time);
    random_load_state(state->random_iv);
    building_count_load_state(state->building_count_industry,
                              state->building_count_culture1,
                              state->building_count_culture2,
                              state->building_count_culture3,
                              state->building_count_military,
                              state->building_count_support);

    empire_load_state(state->empire);
    empire_object_load_state(state->empire_objects);
    trade_prices_load_state(state->trade_prices);
    figure_name_load_state(state->figure_names);
    city_culture_load_state(state->culture_coverage);

    scenario_load_state(state->scenario);
    scenario_earthquake_load_state(state->earthquake);
    city_message_load_state(state->messages, state->message_extra,
                            state->message_counts, state->message_delays,
                            state->population_messages);
    sound_city_load_state(state->city_sounds);
    traders_load_state(state->figure_traders);

    building_list_load_state(state->building_list_small, state->building_list_large,
                             state->building_list_burning, state->building_list_burning_totals);

    building_storage_load_state(state->building_storages);
    trade_routes_load_state(state->trade_route_limit, state->trade_route_traded);
    map_routing_load_state(state->routing_counters);
    enemy_armies_load_state(state->enemy_armies, state->enemy_army_totals);
    map_bookmark_load_state(state->bookmarks);
}

static void savegame_save_to_state(savegame_state *state)
{
    buffer_write_i32(state->file_version, savegame_version);

    scenario_settings_save_state(state->player_name);

    map_image_save_state(state->image_grid);
    map_building_save_state(state->building_grid, state->building_damage_grid);
    map_terrain_save_state(state->terrain_grid);
    map_aqueduct_save_state(state->aqueduct_grid, state->aqueduct_backup_grid);
    map_figure_save_state(state->figure_grid);
    map_sprite_save_state(state->sprite_grid, state->sprite_backup_grid);
    map_property_save_state(state->bitfields_grid, state->edge_grid);
    map_random_save_state(state->random_grid);
    map_desirability_save_state(state->desirability_grid);
    map_elevation_save_state(state->elevation_grid);

    figure_save_state(state->figures, state->figure_sequence);
    figure_route_save_state(state->route_figures, state->route_paths);
    formations_save_state(state->formations, state->formation_totals);

    city_data_save_state(state->city_data,
                         state->city_graph_order,
                         state->city_entry_exit_xy,
                         state->city_entry_exit_grid_offset);

    building_save_state(state->buildings,
                        state->building_extra_highest_id,
                        state->building_extra_highest_id_ever,
                        state->building_extra_sequence,
                        state->building_extra_corrupt_houses);
    building_barracks_save_state(state->building_barracks_tower_sentry);
    city_view_save_state(state->city_view_orientation, state->city_view_camera);
    game_time_save_state(state->game_time);
    random_save_state(state->random_iv);
    building_count_save_state(state->building_count_industry,
                              state->building_count_culture1,
                              state->building_count_culture2,
                              state->building_count_culture3,
                              state->building_count_military,
                              state->building_count_support);
    empire_save_state(state->empire);
    empire_object_save_state(state->empire_objects);
    trade_prices_save_state(state->trade_prices);
    figure_name_save_state(state->figure_names);
    city_culture_save_state(state->culture_coverage);

    scenario_save_state(state->scenario);

    scenario_earthquake_save_state(state->earthquake);
    city_message_save_state(state->messages, state->message_extra,
                            state->message_counts, state->message_delays,
                            state->population_messages);
    sound_city_save_state(state->city_sounds);
    traders_save_state(state->figure_traders);

    building_list_save_state(state->building_list_small, state->building_list_large,
                             state->building_list_burning, state->building_list_burning_totals);

    building_storage_save_state(state->building_storages);
    trade_routes_save_state(state->trade_route_limit, state->trade_route_traded);
    map_routing_save_state(state->routing_counters);
    enemy_armies_save_state(state->enemy_armies, state->enemy_army_totals);
    map_bookmark_save_state(state->bookmarks);
}

int game_file_io_read_scenario(const char *dir, const char *filename)
{
    log_info("Loading scenario", filename, 0);
    init_scenario_data();

    static char dir_prepended_filepath[FILE_NAME_MAX];
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

    scenario_load_from_state(&scenario_data.state);
    return 1;
}

int game_file_io_write_scenario(const char *dir, const char *filename)
{
    log_info("Saving scenario", filename, 0);
    init_scenario_data();
    scenario_save_to_state(&scenario_data.state);

    static char dir_prepended_filepath[FILE_NAME_MAX];
    prepend_dir_to_path(dir, filename, dir_prepended_filepath);

    FILE *fp = fopen(dir_prepended_filepath, "wb");
    if (!fp) {
        log_error("Unable to save scenario", 0, 0);
        return 0;
    }
    for (int i = 0; i < scenario_data.num_pieces; i++) {
        fwrite(scenario_data.pieces[i].buf.data, 1, scenario_data.pieces[i].buf.size, fp);
    }
    fclose(fp);
    return 1;
}

static int read_int32(FILE *fp)
{
    uint8_t data[4];
    if (fread(&data, 1, 4, fp) != 4) {
        return 0;
    }
    buffer buf;
    buffer_init(&buf, data, 4);
    return buffer_read_i32(&buf);
}

static void write_int32(FILE *fp, int value)
{
    uint8_t data[4];
    buffer buf;
    buffer_init(&buf, data, 4);
    buffer_write_i32(&buf, value);
    fwrite(&data, 1, 4, fp);
}

static int read_compressed_chunk(FILE *fp, void *buffer, int bytes_to_read)
{
    if (bytes_to_read > COMPRESS_BUFFER_SIZE) {
        return 0;
    }
    int input_size = read_int32(fp);
    if ((unsigned int) input_size == UNCOMPRESSED) {
        if (fread(buffer, 1, bytes_to_read, fp) != (unsigned) bytes_to_read) {
            return 0;
        }
    } else {
        if (fread(compress_buffer, 1, input_size, fp) != (unsigned) input_size
            || !zip_decompress(compress_buffer, input_size, buffer, &bytes_to_read)) {
            return 0;
        }
    }
    return 1;
}

static int write_compressed_chunk(FILE *fp, const void *buffer, int bytes_to_write)
{
    if (bytes_to_write > COMPRESS_BUFFER_SIZE) {
        return 0;
    }
    int output_size = COMPRESS_BUFFER_SIZE;
    if (zip_compress(buffer, bytes_to_write, compress_buffer, &output_size)) {
        write_int32(fp, output_size);
        fwrite(compress_buffer, 1, output_size, fp);
    } else {
        // unable to compress: write uncompressed
        write_int32(fp, UNCOMPRESSED);
        fwrite(buffer, 1, bytes_to_write, fp);
    }
    return 1;
}

static int savegame_read_from_file(FILE *fp)
{
    for (int i = 0; i < savegame_data.num_pieces; i++) {
        file_piece *piece = &savegame_data.pieces[i];
        int result = 0;
        if (piece->compressed) {
            result = read_compressed_chunk(fp, piece->buf.data, piece->buf.size);
        } else {
            result = fread(piece->buf.data, 1, piece->buf.size, fp) == (unsigned) piece->buf.size;
        }
        // The last piece may be smaller than buf.size
        if (!result && i != (savegame_data.num_pieces - 1)) {
            return 0;
        }
    }
    return 1;
}

static void savegame_write_to_file(FILE *fp)
{
    for (int i = 0; i < savegame_data.num_pieces; i++) {
        file_piece *piece = &savegame_data.pieces[i];
        if (piece->compressed) {
            write_compressed_chunk(fp, piece->buf.data, piece->buf.size);
        } else {
            fwrite(piece->buf.data, 1, piece->buf.size, fp);
        }
    }
}

int game_file_io_read_saved_game(const char *dir, const char *filename, int offset)
{
    init_savegame_data();

    log_info("Loading saved game", filename, 0);
    static char dir_prepended_filepath[FILE_NAME_MAX];
    prepend_dir_to_path(dir, filename, dir_prepended_filepath);

    FILE *fp = fopen(dir_prepended_filepath, "rb");
    if (!fp) {
        log_error("Unable to load game", 0, 0);
        return 0;
    }
    if (offset) {
        fseek(fp, offset, SEEK_SET);
    }
    int result = savegame_read_from_file(fp);
    fclose(fp);
    if (!result) {
        log_error("Unable to load game", 0, 0);
        return 0;
    }
    savegame_load_from_state(&savegame_data.state);
    return 1;
}

int game_file_io_write_saved_game(const char *dir, const char *filename)
{
    init_savegame_data();

    log_info("Saving game", filename, 0);
    savegame_version = SAVE_GAME_VERSION;
    savegame_save_to_state(&savegame_data.state);

    static char dir_prepended_filepath[FILE_NAME_MAX];
    prepend_dir_to_path(dir, filename, dir_prepended_filepath);

    FILE *fp = fopen(dir_prepended_filepath, "wb");
    if (!fp) {
        log_error("Unable to save game", 0, 0);
        return 0;
    }
    savegame_write_to_file(fp);
    fclose(fp);
    return 1;
}

int game_file_io_delete_saved_game(const char *filename)
{
    log_info("Deleting game", filename, 0);
    static char dir_prepended_filepath[FILE_NAME_MAX];
    prepend_dir_to_path(SAVES_DIR_PATH, filename, dir_prepended_filepath);
    int result = remove(dir_prepended_filepath);

    if (result == -1) {
        log_error("Unable to delete game", 0, 0);
        return 0;
    } else {
        return 1;
    }
}
