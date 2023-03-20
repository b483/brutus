#include "core/dir.h"

#if defined (_WIN32) && defined (__MINGW32__)
#include <dirent.h>
#elif defined (_WIN32)
#include "../ext/dirent/dirent.h"
#endif

#include <string.h>

static struct dir_listing listing;

const struct dir_listing *dir_list_files(const char *extension)
{
    memset(listing.files, 0, sizeof(listing.files));
    listing.num_files = 0;
    listing.file_overflow = 0;
    DIR *dir = 0;
    struct dirent *file_info;
    if (strcmp(extension, "map") == 0) {
        dir = opendir(MAPS_DIR_PATH);
    } else if (strcmp(extension, "sav") == 0) {
        dir = opendir(SAVES_DIR_PATH);
    }
    if (dir) {
        while ((file_info = readdir(dir)) != NULL) {
            if (listing.num_files >= MAX_NUM_FILES) {
                listing.file_overflow = 1;
                return &listing;
            }
            if (file_has_extension(file_info->d_name, extension)) {
                if (strlen(file_info->d_name) < FILE_NAME_MAX) {
                    strncpy(listing.files[listing.num_files], file_info->d_name, FILE_NAME_MAX);
                    listing.num_files++;
                }
            }
        }
        closedir(dir);
    }
    return &listing;
}

void prepend_dir_to_path(const char *dir_to_prepend, const char *filepath, char *resulting_string)
{
    size_t dir_len = strlen(dir_to_prepend) + 1;
    strncpy(resulting_string, dir_to_prepend, DIR_PATH_MAX);
#ifdef _WIN32
    resulting_string[dir_len - 1] = '\\';
#elif(__linux__)
    resulting_string[dir_len - 1] = '/';
#endif
    strncpy(&resulting_string[dir_len], filepath, DIR_PATH_MAX - dir_len - 1);
}
