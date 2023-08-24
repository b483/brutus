#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "building/building.h"

int game_pre_init(void);

int game_init(void);

int reload_language(int is_editor, int reload_images);

int game_init_editor(void);

void game_run(void);

void game_draw(void);

void game_exit(void);

int game_animation_should_advance(int speed);

/**
 * Create data for a new scenario
 * @param size Size of the map to create
 */
void game_file_editor_create_scenario(int size);

/**
 * Load scenario from disk and init it for using in the editor
 * @param dir Directory to load from
 * @param scenario_file File to load
 * @return Boolean true on success, false on failure
 */
int game_file_editor_load_scenario(const char *dir, const char *scenario_file);

/**
 * Write scenario to disk
 * @param dir Directory to save in
 * @param scenario_file File to save to
 * @return Boolean true on success, false on failure
 */
int game_file_editor_write_scenario(const char *dir, const char *scenario_file);

int game_file_io_write_saved_game(const char *dir, const char *filename);

int game_file_io_delete_saved_game(const char *filename);

/**
 * Start scenario from filename
 * @param scenario_file File to load
 * @return Boolean true on success, false on failure
 */
int game_file_start_scenario(const char *scenario_file);

/**
 * Load scenario data only, without starting it
 * @param scenario_file File to load
 * @return Boolean true on success, false on failure
 */
int game_file_load_scenario_data(const char *scenario_file);

/**
 * Load saved game
 * @param filename File to load
 * @return Boolean true on success, false on failure
 */
int game_file_load_saved_game(const char *dir, const char *filename);

enum {
    DIR_0_TOP = 0,
    DIR_1_TOP_RIGHT = 1,
    DIR_2_RIGHT = 2,
    DIR_3_BOTTOM_RIGHT = 3,
    DIR_4_BOTTOM = 4,
    DIR_5_BOTTOM_LEFT = 5,
    DIR_6_LEFT = 6,
    DIR_7_TOP_LEFT = 7,
    DIR_8_NONE = 8,
    DIR_FIGURE_AT_DESTINATION = 8,
    DIR_FIGURE_REROUTE = 9,
    DIR_FIGURE_LOST = 10,
    DIR_FIGURE_ATTACK = 11,
};

void game_orientation_rotate_left(void);

void game_orientation_rotate_right(void);

void game_orientation_rotate_north(void);

enum {
    SOUND_MUSIC = 1,
    SOUND_SPEECH = 2,
    SOUND_EFFECTS = 3,
    SOUND_CITY = 4,
};

struct set_sound_t {
    int enabled;
    int volume;
};

void settings_load(void);

void settings_save(void);

int setting_fullscreen(void);
void setting_window(int *width, int *height);
void setting_set_display(int fullscreen, int width, int height);

struct set_sound_t *get_sound(int type);

void setting_toggle_sound_enabled(int type);
void setting_increase_sound_volume(int type);
void setting_decrease_sound_volume(int type);

int setting_game_speed(void);
void setting_decrease_game_speed(void);
void setting_increase_game_speed(void);

int setting_scroll_speed(void);
void setting_increase_scroll_speed(void);
void setting_decrease_scroll_speed(void);

int setting_warnings(void);
void setting_toggle_warnings(void);

int setting_monthly_autosave(void);
void setting_toggle_monthly_autosave(void);

int setting_victory_video(void);

int setting_last_advisor(void);
void setting_set_last_advisor(int advisor);

enum {
    OVERLAY_NONE = 0,
    OVERLAY_WATER = 2,
    OVERLAY_RELIGION = 4,
    OVERLAY_FIRE = 8,
    OVERLAY_DAMAGE = 9,
    OVERLAY_CRIME = 10,
    OVERLAY_ENTERTAINMENT = 11,
    OVERLAY_THEATER = 12,
    OVERLAY_AMPHITHEATER = 13,
    OVERLAY_COLOSSEUM = 14,
    OVERLAY_HIPPODROME = 15,
    OVERLAY_EDUCATION = 16,
    OVERLAY_SCHOOL = 17,
    OVERLAY_LIBRARY = 18,
    OVERLAY_ACADEMY = 19,
    OVERLAY_BARBER = 20,
    OVERLAY_BATHHOUSE = 21,
    OVERLAY_CLINIC = 22,
    OVERLAY_HOSPITAL = 23,
    OVERLAY_TAX_INCOME = 24,
    OVERLAY_FOOD_STOCKS = 25,
    OVERLAY_DESIRABILITY = 26,
    OVERLAY_WORKERS_UNUSED = 27,
    OVERLAY_NATIVE = 28,
    OVERLAY_PROBLEMS = 29
};

int game_state_is_paused(void);

void game_state_toggle_paused(void);

void game_state_unpause(void);

int game_state_overlay(void);

void game_state_reset_overlay(void);

void game_state_set_overlay(int overlay);

void building_house_change_to(struct building_t *house, int type);

/**
 * The current game year
 *
 */
int game_time_year(void);

/**
 * The current game month within the year
 */
int game_time_month(void);

/**
 * The current game day within the month
 */
int game_time_day(void);

/**
 * The current game tick within the day
 */
int game_time_tick(void);

/**
 * Saves the game time
 * @param buf Buffer
 */
void game_time_save_state(struct buffer_t *buf);

/**
 * Loads the game time
 * @param buf Buffer
 */
void game_time_load_state(struct buffer_t *buf);

int game_can_undo(void);

void game_undo_disable(void);

void game_undo_add_building(struct building_t *b);

int game_undo_contains_building(int building_id);

void game_undo_restore_building_state(void);

void game_undo_restore_map(int include_properties);

int game_undo_start_build(int type);

void game_undo_finish_build(int cost);

void game_undo_perform(void);

#endif // GAME_GAME_H
