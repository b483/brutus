#ifndef GAME_FILE_EDITOR_H
#define GAME_FILE_EDITOR_H

/**
 * Clears city/scenario memory not applicable to editing scenarios
 */
void game_file_editor_clear_data(void);

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

#endif // GAME_FILE_EDITOR_H
