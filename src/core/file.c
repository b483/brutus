#include "file.h"

#include "core/dir.h"

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#endif

#include <string.h>

int file_has_extension(const char *filename, const char *extension)
{
    if (!extension || !*extension) {
        return 1;
    }
    char c;
    do {
        c = *filename;
        filename++;
    } while (c != '.' && c);
    if (!c) {
        filename--;
    }
    return strcmp(filename, extension) == 0;
}

void file_change_extension(char *filename, const char *new_extension)
{
    char c;
    do {
        c = *filename;
        filename++;
    } while (c != '.' && c);
    if (c == '.') {
        filename[0] = new_extension[0];
        filename[1] = new_extension[1];
        filename[2] = new_extension[2];
        filename[3] = 0;
    }
}

void file_append_extension(char *filename, const char *extension)
{
    char c;
    do {
        c = *filename;
        filename++;
    } while (c);
    filename--;
    filename[0] = '.';
    filename[1] = extension[0];
    filename[2] = extension[1];
    filename[3] = extension[2];
    filename[4] = 0;
}

void file_remove_extension(uint8_t *filename)
{
    uint8_t c;
    do {
        c = *filename;
        filename++;
    } while (c != '.' && c);
    if (c == '.') {
        filename--;
        *filename = 0;
    }
}

int file_exists(const char *dir, const char *filename)
{
    if (dir) {
        static char filepath_to_save[DIR_PATH_MAX];
        filepath_to_save[DIR_PATH_MAX - 1] = 0;
        prepend_dir_to_path(dir, filename, filepath_to_save);
        if (access(filepath_to_save, F_OK) == 0) {
            return 1;
        } else {
            return 0;
        }
    } else {
        if (access(filename, F_OK) == 0) {
            return 1;
        } else {
            return 0;
        }
    }
}
