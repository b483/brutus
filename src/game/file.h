#ifndef GAME_FILE_H
#define GAME_FILE_H

#include <stdint.h>

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

#endif // GAME_FILE_H
