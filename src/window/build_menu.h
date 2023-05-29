#ifndef WINDOW_BUILD_MENU_H
#define WINDOW_BUILD_MENU_H

#include <stdint.h>

#define BUILD_MENU_BUTTONS_COUNT 12
#define MAX_ITEMS_PER_BUILD_MENU 11

#define MAX_ITEMS_PER_SUBMENU 6

enum {
    MENU_VACANT_HOUSE = 0,
    MENU_CLEAR_LAND = 1,
    MENU_ROAD = 2,
    MENU_WATER = 3,
    MENU_HEALTH = 4,
    MENU_TEMPLES = 5,
    MENU_EDUCATION = 6,
    MENU_ENTERTAINMENT = 7,
    MENU_ADMINISTRATION = 8,
    MENU_ENGINEERING = 9,
    MENU_SECURITY = 10,
    MENU_INDUSTRY = 11,
};

extern const int BUILDING_MENU_SUBMENU_ITEM_MAPPING[BUILD_MENU_BUTTONS_COUNT][MAX_ITEMS_PER_BUILD_MENU][MAX_ITEMS_PER_SUBMENU];

struct submenu_t {
    int building_id;
    uint8_t *submenu_string;
    int submenu_items[MAX_ITEMS_PER_SUBMENU];
};

struct build_menu_t {
    int is_enabled;
    struct submenu_t menu_items[MAX_ITEMS_PER_BUILD_MENU];
};

extern struct build_menu_t build_menus[BUILD_MENU_BUTTONS_COUNT];

void map_building_menu_items(void);

int window_build_menu_image(void);

void window_build_menu_show(int submenu);

void window_build_menu_hide(void);

#endif // WINDOW_BUILD_MENU_H
