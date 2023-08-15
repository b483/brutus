#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

enum {
    GROUP_EDITOR_SCENARIO_IMAGE = 22,
    GROUP_EDITOR_BUILDING_CROPS = 23,
    GROUP_EDITOR_EMPIRE_MAP = 47,
    GROUP_EDITOR_SIDE_PANEL = 80,
    GROUP_EDITOR_EMPIRE_PANELS = 172,
    GROUP_EDITOR_BUILDING_NATIVE = 183,
    GROUP_EDITOR_EMPIRE_FOREIGN_CITY = 223,
    GROUP_EDITOR_TRADE_AMOUNT = 243,
};

extern char *climate_types_strings[];

void show_editor_map(void);

void editor_set_active(int active);
int editor_is_active(void);

#endif // EDITOR_EDITOR_H
