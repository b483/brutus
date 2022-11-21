#include "core/io.h"

#include <stdio.h>

#include "core/file.h"

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
