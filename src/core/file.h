#ifndef CORE_FILE_H
#define CORE_FILE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_NAME_MAX 64
#define DIR_PATH_MAX 255

// the path to the folder where brutus.exe is located
extern char EXECUTABLE_DIR_PATH[DIR_PATH_MAX];

// the path to "data_dir.txt" within the Brutus directory
extern char DATA_TEXT_FILE_PATH[DIR_PATH_MAX];

// the path to "brutus.settings" within the Brutus directory
extern char SETTINGS_FILE_PATH[DIR_PATH_MAX];

// the path to "brutus.configs" within the Brutus directory
extern char CONFIGS_FILE_PATH[DIR_PATH_MAX];

// the path to "brutus.hconfigs" within the Brutus directory
extern char HOTKEY_CONFIGS_FILE_PATH[DIR_PATH_MAX];

// the path to the /maps folder in the Brutus directory
extern char MAPS_DIR_PATH[DIR_PATH_MAX];

// the path to the /saves folder in the Brutus directory
extern char SAVES_DIR_PATH[DIR_PATH_MAX];

// the path to the folder where c3.exe is located
extern char GAME_DATA_PATH[DIR_PATH_MAX];

/**
 * Checks whether the file has the given extension
 * @param filename Filename to check
 * @param extension Extension
 * @return boolean true if the file has the given extension, false otherwise
 */
int file_has_extension(const char *filename, const char *extension);

/**
 * Replaces the current extension by the given new extension.
 * Filename is unchanged if there was no extension.
 * @param[in,out] filename Filename to change
 * @param new_extension New extension
 */
void file_change_extension(char *filename, const char *new_extension);

/**
 * Appends the extension to the file
 * @param[in,out] filename Filename to change
 * @param extension Extension to append
 */
void file_append_extension(char *filename, const char *extension);

/**
 * Removes the extension from the file
 * @param[in,out] filename Filename to change
 */
void file_remove_extension(uint8_t *filename);

/**
 * Check if file exists
 * @param dir Directory to check in
 * @param filename Filename to check
 * @return boolean true if the file exists, false otherwise
 */
int file_exists(const char *dir, const char *filename);

#endif // CORE_FILE_H
