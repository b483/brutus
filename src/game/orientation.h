#ifndef GAME_ORIENTATION_H
#define GAME_ORIENTATION_H

enum {
    DIR_0_TOP = 0,
    DIR_1_TOP_RIGHT = 1,
    DIR_2_RIGHT = 2,
    DIR_3_BOTTOM_RIGHT = 3,
    DIR_4_BOTTOM = 4,
    DIR_5_BOTTOM_LEFT = 5,
    DIR_6_LEFT = 6,
    DIR_7_TOP_LEFT = 7,
    DIR_8_NONE = 8,
    DIR_FIGURE_AT_DESTINATION = 8,
    DIR_FIGURE_REROUTE = 9,
    DIR_FIGURE_LOST = 10,
    DIR_FIGURE_ATTACK = 11,
};

void game_orientation_rotate_left(void);

void game_orientation_rotate_right(void);

void game_orientation_rotate_north(void);

#endif // GAME_ORIENTATION_H
