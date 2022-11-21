#ifndef CORE_DIR_H
#define CORE_DIR_H

#include "core/file.h"

#define MAX_NUM_FILES 128

/**
 * @file
 * Directory-related functions.
 */

 /**
  * Directory listing
  */
struct dir_listing {
  char files[MAX_NUM_FILES][FILE_NAME_MAX]; /**< Filenames in UTF-8 encoding */
  int num_files; /**< Number of files in the list */
  int file_overflow;
};

const struct dir_listing *dir_list_files(const char *extension);

/**
 * Prepends given directory to path
 * @param dir_to_prepend Directory to prepend
 * @param filepath File path to prepend directory to
 * @param resulting_string The resulting new string
 */
void prepend_dir_to_path(const char *dir_to_prepend, const char *filepath, char *resulting_string);

#endif // CORE_DIR_H
