#ifndef WINDOW_EDITOR_BUILD_MENU_H
#define WINDOW_EDITOR_BUILD_MENU_H

enum {
    MENU_NONE = -1,
    MENU_SHRUB = 0,
    MENU_ELEVATION = 1,
    MENU_ROCK = 2,
    MENU_BRUSH_SIZE = 3,
    MENU_EARTHQUAKE_POINTS = 4,
    MENU_INVASION_POINTS = 5,
    MENU_PEOPLE_POINTS = 6,
    MENU_RIVER_POINTS = 7,
    MENU_NATIVE_BUILDINGS = 8,
    MENU_ANIMAL_POINTS = 9,
};

void window_editor_build_menu_show(int submenu);

void window_editor_build_menu_hide(void);

#endif // WINDOW_EDITOR_BUILD_MENU_H
