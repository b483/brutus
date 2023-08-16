#ifndef INPUT_CURSOR_H
#define INPUT_CURSOR_H

enum {
    CURSOR_ARROW = 0,
    CURSOR_SHOVEL = 1,
    CURSOR_SWORD = 2,
    CURSOR_MAX,
};

enum {
    CURSOR_SCALE_1 = 0,
    CURSOR_SCALE_1_5 = 1,
    CURSOR_SCALE_2 = 2,
};

struct cursor_t {
    int hotspot_x;
    int hotspot_y;
    int width;
    int height;
    const char *data;
};

const struct cursor_t *input_cursor_data(int cursor_id, int scale);

void input_cursor_update(int window);

#endif // INPUT_CURSOR_H
