#ifndef PLATFORM_ARGUMENTS_H
#define PLATFORM_ARGUMENTS_H

struct brutus_args_t {
    int display_scale_percentage;
    int cursor_scale_percentage;
    int force_windowed;
};

int platform_parse_arguments(int argc, char **argv, struct brutus_args_t *output_args);

#endif // PLATFORM_ARGUMENTS_H
