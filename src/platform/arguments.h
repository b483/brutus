#ifndef PLATFORM_ARGUMENTS_H
#define PLATFORM_ARGUMENTS_H

typedef struct {
    const char *data_directory;
    int display_scale_percentage;
    int cursor_scale_percentage;
    int force_windowed;
} brutus_args;

int platform_parse_arguments(int argc, char **argv, brutus_args *output_args);

#endif // PLATFORM_ARGUMENTS_H
