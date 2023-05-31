#ifndef GRAPHICS_MENU_H
#define GRAPHICS_MENU_H

#include "input/mouse.h"

#define TOP_MENU_HEIGHT 24

struct menu_item_t {
    short text_group;
    short text_number;
    void (*left_click_handler)(int param);
    int parameter;
    int hidden;
};

struct menu_bar_item_t {
    short text_group;
    struct menu_item_t *items;
    int num_items;
    short x_start;
    short x_end;
    int calculated_width_blocks;
    int calculated_height_blocks;
};

void menu_bar_draw(struct menu_bar_item_t *items, int num_items, int max_width);
int menu_bar_handle_mouse(const struct mouse_t *m, struct menu_bar_item_t *items, int num_items, int *focus_menu_id);

void menu_draw(struct menu_bar_item_t *menu, int focus_item_id);
int menu_handle_mouse(const struct mouse_t *m, struct menu_bar_item_t *menu, int *focus_item_id);
void menu_update_text(struct menu_bar_item_t *menu, int index, int text_number);

#endif // GRAPHICS_MENU_H
