#include "city_overlay_entertainment.h"

#include "game/state.h"

static int show_building_entertainment(const struct building_t *b)
{
    return
        b->type == BUILDING_ACTOR_COLONY || b->type == BUILDING_THEATER ||
        b->type == BUILDING_GLADIATOR_SCHOOL || b->type == BUILDING_AMPHITHEATER ||
        b->type == BUILDING_LION_HOUSE || b->type == BUILDING_COLOSSEUM ||
        b->type == BUILDING_CHARIOT_MAKER || b->type == BUILDING_HIPPODROME;
}

static int show_building_theater(const struct building_t *b)
{
    return b->type == BUILDING_ACTOR_COLONY || b->type == BUILDING_THEATER;
}

static int show_building_amphitheater(const struct building_t *b)
{
    return b->type == BUILDING_ACTOR_COLONY
        || b->type == BUILDING_GLADIATOR_SCHOOL
        || b->type == BUILDING_AMPHITHEATER;
}

static int show_building_colosseum(const struct building_t *b)
{
    return b->type == BUILDING_GLADIATOR_SCHOOL || b->type == BUILDING_LION_HOUSE || b->type == BUILDING_COLOSSEUM;
}

static int show_building_hippodrome(const struct building_t *b)
{
    return b->type == BUILDING_CHARIOT_MAKER || b->type == BUILDING_HIPPODROME;
}

static struct building_t *get_entertainment_building(const struct figure_t *f)
{
    if (f->action_state == FIGURE_ACTION_ENTERTAINER_ROAMING ||
        f->action_state == FIGURE_ACTION_ENTERTAINER_RETURNING) {
        return &all_buildings[f->building_id];
    } else {
        return &all_buildings[f->destination_building_id];
    }
}

static int show_figure_entertainment(const struct figure_t *f)
{
    return f->type == FIGURE_ACTOR || f->type == FIGURE_GLADIATOR ||
        f->type == FIGURE_LION_TAMER || f->type == FIGURE_CHARIOTEER;
}

static int show_figure_theater(const struct figure_t *f)
{
    if (f->type == FIGURE_ACTOR) {
        return get_entertainment_building(f)->type == BUILDING_THEATER;
    }
    return 0;
}

static int show_figure_amphitheater(const struct figure_t *f)
{
    if (f->type == FIGURE_ACTOR || f->type == FIGURE_GLADIATOR) {
        return get_entertainment_building(f)->type == BUILDING_AMPHITHEATER;
    }
    return 0;
}

static int show_figure_colosseum(const struct figure_t *f)
{
    if (f->type == FIGURE_GLADIATOR) {
        return get_entertainment_building(f)->type == BUILDING_COLOSSEUM;
    } else if (f->type == FIGURE_LION_TAMER) {
        return 1;
    }
    return 0;
}

static int show_figure_hippodrome(const struct figure_t *f)
{
    return f->type == FIGURE_CHARIOTEER;
}

static int get_column_height_entertainment(const struct building_t *b)
{
    return b->house_size && b->data.house.entertainment ? b->data.house.entertainment / 10 : NO_COLUMN;
}

static int get_column_height_theater(const struct building_t *b)
{
    return b->house_size && b->data.house.theater ? b->data.house.theater / 10 : NO_COLUMN;
}

static int get_column_height_amphitheater(const struct building_t *b)
{
    return b->house_size && b->data.house.amphitheater_actor ? b->data.house.amphitheater_actor / 10 : NO_COLUMN;
}

static int get_column_height_colosseum(const struct building_t *b)
{
    return b->house_size && b->data.house.colosseum_gladiator ? b->data.house.colosseum_gladiator / 10 : NO_COLUMN;
}

static int get_column_height_hippodrome(const struct building_t *b)
{
    return b->house_size && b->data.house.hippodrome ? b->data.house.hippodrome / 10 : NO_COLUMN;
}

static int get_tooltip_entertainment(__attribute__((unused)) tooltip_context *c, const struct building_t *b)
{
    if (b->data.house.entertainment <= 0) {
        return 64;
    } else if (b->data.house.entertainment < 10) {
        return 65;
    } else if (b->data.house.entertainment < 20) {
        return 66;
    } else if (b->data.house.entertainment < 30) {
        return 67;
    } else if (b->data.house.entertainment < 40) {
        return 68;
    } else if (b->data.house.entertainment < 50) {
        return 69;
    } else if (b->data.house.entertainment < 60) {
        return 70;
    } else if (b->data.house.entertainment < 70) {
        return 71;
    } else if (b->data.house.entertainment < 80) {
        return 72;
    } else if (b->data.house.entertainment < 90) {
        return 73;
    } else {
        return 74;
    }
}

static int get_tooltip_theater(__attribute__((unused)) tooltip_context *c, const struct building_t *b)
{
    if (b->data.house.theater <= 0) {
        return 75;
    } else if (b->data.house.theater >= 80) {
        return 76;
    } else if (b->data.house.theater >= 20) {
        return 77;
    } else {
        return 78;
    }
}

static int get_tooltip_amphitheater(__attribute__((unused)) tooltip_context *c, const struct building_t *b)
{
    if (b->data.house.amphitheater_actor <= 0) {
        return 79;
    } else if (b->data.house.amphitheater_actor >= 80) {
        return 80;
    } else if (b->data.house.amphitheater_actor >= 20) {
        return 81;
    } else {
        return 82;
    }
}

static int get_tooltip_colosseum(__attribute__((unused)) tooltip_context *c, const struct building_t *b)
{
    if (b->data.house.colosseum_gladiator <= 0) {
        return 83;
    } else if (b->data.house.colosseum_gladiator >= 80) {
        return 84;
    } else if (b->data.house.colosseum_gladiator >= 20) {
        return 85;
    } else {
        return 86;
    }
}

static int get_tooltip_hippodrome(__attribute__((unused)) tooltip_context *c, const struct building_t *b)
{
    if (b->data.house.hippodrome <= 0) {
        return 87;
    } else if (b->data.house.hippodrome >= 80) {
        return 88;
    } else if (b->data.house.hippodrome >= 20) {
        return 89;
    } else {
        return 90;
    }
}

const city_overlay *city_overlay_for_entertainment(void)
{
    static city_overlay overlay = {
        OVERLAY_ENTERTAINMENT,
        COLUMN_TYPE_ACCESS,
        show_building_entertainment,
        show_figure_entertainment,
        get_column_height_entertainment,
        0,
        get_tooltip_entertainment,
        0,
        0
    };
    return &overlay;
}

const city_overlay *city_overlay_for_theater(void)
{
    static city_overlay overlay = {
        OVERLAY_THEATER,
        COLUMN_TYPE_ACCESS,
        show_building_theater,
        show_figure_theater,
        get_column_height_theater,
        0,
        get_tooltip_theater,
        0,
        0
    };
    return &overlay;
}

const city_overlay *city_overlay_for_amphitheater(void)
{
    static city_overlay overlay = {
        OVERLAY_AMPHITHEATER,
        COLUMN_TYPE_ACCESS,
        show_building_amphitheater,
        show_figure_amphitheater,
        get_column_height_amphitheater,
        0,
        get_tooltip_amphitheater,
        0,
        0
    };
    return &overlay;
}

const city_overlay *city_overlay_for_colosseum(void)
{
    static city_overlay overlay = {
        OVERLAY_COLOSSEUM,
        COLUMN_TYPE_ACCESS,
        show_building_colosseum,
        show_figure_colosseum,
        get_column_height_colosseum,
        0,
        get_tooltip_colosseum,
        0,
        0
    };
    return &overlay;
}

const city_overlay *city_overlay_for_hippodrome(void)
{
    static city_overlay overlay = {
        OVERLAY_HIPPODROME,
        COLUMN_TYPE_ACCESS,
        show_building_hippodrome,
        show_figure_hippodrome,
        get_column_height_hippodrome,
        0,
        get_tooltip_hippodrome,
        0,
        0
    };
    return &overlay;
}
