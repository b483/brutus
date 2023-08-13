#include "file.h"

#include "core/string.h"

#include <io.h>
#define F_OK 0
#define access _access

#if defined (__MINGW32__)
#include <dirent.h>
#else
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
    if (string_equals(extension, "map")) {
        dir = opendir(MAPS_DIR_PATH);
    } else if (string_equals(extension, "sav")) {
        dir = opendir(SAVES_DIR_PATH);
    }
    if (dir) {
        while ((file_info = readdir(dir)) != NULL) {
            if (listing.num_files >= MAX_NUM_FILES) {
                listing.file_overflow = 1;
                return &listing;
            }
            if (file_has_extension(file_info->d_name, extension)) {
                if (string_length(file_info->d_name) < FILE_NAME_MAX) {
                    string_copy(file_info->d_name, listing.files[listing.num_files], FILE_NAME_MAX);
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
    size_t dir_len = string_length(dir_to_prepend) + 1;
    string_copy(dir_to_prepend, resulting_string, DIR_PATH_MAX);
    resulting_string[dir_len - 1] = '\\';
    string_copy(filepath, &resulting_string[dir_len], DIR_PATH_MAX - dir_len - 1);
}

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
    return string_equals(filename, extension);
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

void file_remove_extension(char *filename)
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

int io_read_file_into_buffer(const char *filepath, void *buffer, int max_size)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    if (size > max_size) {
        size = max_size;
    }
    fseek(fp, 0, SEEK_SET);
    int bytes_read = (int) fread(buffer, 1, (size_t) size, fp);
    fclose(fp);
    return bytes_read;
}

int io_read_file_part_into_buffer(const char *filepath, void *buffer, int size, int offset_in_file)
{
    int bytes_read = 0;
    FILE *fp = fopen(filepath, "rb");
    if (fp) {
        int seek_result = fseek(fp, offset_in_file, SEEK_SET);
        if (seek_result == 0) {
            bytes_read = (int) fread(buffer, 1, (size_t) size, fp);
        }
        fclose(fp);
    }
    return bytes_read;
}

int io_write_buffer_to_file(const char *filepath, const void *buffer, int size)
{
    // Find existing file to overwrite
    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        return 0;
    }
    int bytes_written = (int) fwrite(buffer, 1, (size_t) size, fp);
    fclose(fp);
    return bytes_written;
}