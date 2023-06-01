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

#define MAX_NUM_FILES 128

struct dir_listing {
  char files[MAX_NUM_FILES][FILE_NAME_MAX]; /**< Filenames in UTF-8 encoding */
  int num_files; /**< Number of files in the list */
  int file_overflow;
};

const struct dir_listing *dir_list_files(const char *extension);

void prepend_dir_to_path(const char *dir_to_prepend, const char *filepath, char *resulting_string);

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
void file_remove_extension(char *filename);

/**
 * Check if file exists
 * @param dir Directory to check in
 * @param filename Filename to check
 * @return boolean true if the file exists, false otherwise
 */
int file_exists(const char *dir, const char *filename);

/**
 * Reads the entire file into the buffer
 * @param filepath File to read
 * @param buffer Buffer to read into
 * @param max_size Max size to read
 * @return Number of bytes read
 */
int io_read_file_into_buffer(const char *filepath, void *buffer, int max_size);

/**
 * Reads part of the file into the buffer
 * @param filepath File to read
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @param offset_in_file Offset into the file to start reading
 */
int io_read_file_part_into_buffer(const char *filepath, void *buffer, int size, int offset_in_file);

/**
 * Writes the entire buffer to the file
 * @param filepath File to write
 * @param buffer Buffer to write
 * @param size Number of bytes to write
 * @return Number of bytes written
 */
int io_write_buffer_to_file(const char *filepath, const void *buffer, int size);

#endif // CORE_FILE_H
