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

const struct city_overlay_t *city_overlay_for_entertainment(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_ENTERTAINMENT,
        COLUMN_TYPE_ACCESS,
        show_building_entertainment,
        show_figure_entertainment,
        get_column_height_entertainment,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_theater(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_THEATER,
        COLUMN_TYPE_ACCESS,
        show_building_theater,
        show_figure_theater,
        get_column_height_theater,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_amphitheater(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_AMPHITHEATER,
        COLUMN_TYPE_ACCESS,
        show_building_amphitheater,
        show_figure_amphitheater,
        get_column_height_amphitheater,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_colosseum(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_COLOSSEUM,
        COLUMN_TYPE_ACCESS,
        show_building_colosseum,
        show_figure_colosseum,
        get_column_height_colosseum,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_hippodrome(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_HIPPODROME,
        COLUMN_TYPE_ACCESS,
        show_building_hippodrome,
        show_figure_hippodrome,
        get_column_height_hippodrome,
        0,
        0
    };
    return &overlay;
}
