#include "core/dir.h"

#include "core/config.h"
#include "core/file.h"
#include "core/string.h"
#include "platform/file_manager.h"

#include <stdlib.h>
#include <string.h>

#define BASE_MAX_FILES 100

static struct {
    dir_listing listing;
    int max_files;
    char *cased_filename;
} data;

static void allocate_listing_files(int min, int max)
{
    for (int i = min; i < max; i++) {
        data.listing.files[i] = malloc(FILE_NAME_MAX * sizeof(char));
        data.listing.files[i][0] = 0;
    }
}

static void clear_dir_listing(void)
{
    data.listing.num_files = 0;
    if (data.max_files <= 0) {
        data.listing.files = (char **) malloc(BASE_MAX_FILES * sizeof(char *));
        allocate_listing_files(0, BASE_MAX_FILES);
        data.max_files = BASE_MAX_FILES;
    } else {
        for (int i = 0; i < data.max_files; i++) {
            data.listing.files[i][0] = 0;
        }
    }
}

static void expand_dir_listing(void)
{
    int old_max_files = data.max_files;

    data.max_files = 2 * old_max_files;
    data.listing.files = (char **) realloc(data.listing.files, data.max_files * sizeof(char *));
    allocate_listing_files(old_max_files, data.max_files);
}

static int compare_lower(const void *va, const void *vb)
{
    // arguments are pointers to char*
    return platform_file_manager_compare_filename(*(const char **) va, *(const char **) vb);
}

static int add_to_listing(const char *filename)
{
    if (data.listing.num_files >= data.max_files) {
        expand_dir_listing();
    }
    strncpy(data.listing.files[data.listing.num_files], filename, FILE_NAME_MAX);
    data.listing.files[data.listing.num_files][FILE_NAME_MAX - 1] = 0;
    ++data.listing.num_files;
    return LIST_CONTINUE;
}

const dir_listing *dir_find_files_with_extension(const char *extension)
{
    clear_dir_listing();
    if (strcmp(extension, "map") == 0) {
        platform_file_manager_list_directory_contents(MAPS_DIR_PATH, TYPE_FILE, extension, add_to_listing);
    } else if (strcmp(extension, "sav") == 0) {
        platform_file_manager_list_directory_contents(SAVES_DIR_PATH, TYPE_FILE, extension, add_to_listing);
    }

    qsort(data.listing.files, data.listing.num_files, sizeof(char *), compare_lower);
    return &data.listing;
}

const dir_listing *dir_find_all_subdirectories(void)
{
    clear_dir_listing();
    platform_file_manager_list_directory_contents(0, TYPE_DIR, 0, add_to_listing);
    qsort(data.listing.files, data.listing.num_files, sizeof(char *), compare_lower);
    return &data.listing;
}

static int compare_case(const char *filename)
{
    if (platform_file_manager_compare_filename(filename, data.cased_filename) == 0) {
        strcpy(data.cased_filename, filename);
        return LIST_MATCH;
    }
    return LIST_NO_MATCH;
}

static int correct_case(const char *dir, char *filename, int type)
{
    data.cased_filename = filename;
    return platform_file_manager_list_directory_contents(dir, type, 0, compare_case) == LIST_MATCH;
}

static void move_left(char *str)
{
    while (*str) {
        str[0] = str[1];
        str++;
    }
    *str = 0;
}

void prepend_dir_to_path(const char *dir_to_prepend, const char *filepath, char *resulting_string)
{
    size_t dir_len = strlen(dir_to_prepend) + 1;
    strncpy(resulting_string, dir_to_prepend, 2 * FILE_NAME_MAX - 1);
#ifdef _WIN32
    resulting_string[dir_len - 1] = '\\';
#else
    resulting_string[dir_len - 1] = '/';
#endif
    strncpy(&resulting_string[dir_len], filepath, 2 * FILE_NAME_MAX - dir_len - 1);
}

const char *get_case_corrected_file(const char *dir, const char *filepath)
{
    static char corrected_filename[2 * FILE_NAME_MAX];
    corrected_filename[2 * FILE_NAME_MAX - 1] = 0;

    size_t dir_len = 0;
    if (dir) {
        prepend_dir_to_path(dir, filepath, corrected_filename);
    } else {
        dir = ".";
        strncpy(&corrected_filename[dir_len], filepath, 2 * FILE_NAME_MAX - dir_len - 1);
    }

    FILE *fp = file_open(corrected_filename, "rb");
    if (fp) {
        file_close(fp);
        return corrected_filename;
    }

    if (!platform_file_manager_should_case_correct_file()) {
        return 0;
    }

    strncpy(&corrected_filename[dir_len], filepath, 2 * FILE_NAME_MAX - dir_len - 1);

    char *slash = strchr(&corrected_filename[dir_len], '/');
    if (!slash) {
        slash = strchr(&corrected_filename[dir_len], '\\');
    }
    if (slash) {
        *slash = 0;
        if (correct_case(dir, &corrected_filename[dir_len], TYPE_DIR)) {
            char *path = slash + 1;
            if (*path == '\\') {
                // double backslash: move everything to the left
                move_left(path);
            }
            if (correct_case(corrected_filename, path, TYPE_FILE)) {
                *slash = '/';
                return corrected_filename;
            }
        }
    } else {
        if (correct_case(dir, corrected_filename, TYPE_FILE)) {
            return corrected_filename;
        }
    }
    return 0;
}
