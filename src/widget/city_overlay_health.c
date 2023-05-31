#include "city_overlay_health.h"

#include "game/state.h"

static int show_building_barber(const struct building_t *b)
{
    return b->type == BUILDING_BARBER;
}

static int show_building_bathhouse(const struct building_t *b)
{
    return b->type == BUILDING_BATHHOUSE;
}

static int show_building_clinic(const struct building_t *b)
{
    return b->type == BUILDING_DOCTOR;
}

static int show_building_hospital(const struct building_t *b)
{
    return b->type == BUILDING_HOSPITAL;
}

static int show_figure_barber(const struct figure_t *f)
{
    return f->type == FIGURE_BARBER;
}

static int show_figure_bathhouse(const struct figure_t *f)
{
    return f->type == FIGURE_BATHHOUSE_WORKER;
}

static int show_figure_clinic(const struct figure_t *f)
{
    return f->type == FIGURE_DOCTOR;
}

static int show_figure_hospital(const struct figure_t *f)
{
    return f->type == FIGURE_SURGEON;
}

static int get_column_height_barber(const struct building_t *b)
{
    return b->house_size && b->data.house.barber ? b->data.house.barber / 10 : NO_COLUMN;
}

static int get_column_height_bathhouse(const struct building_t *b)
{
    return b->house_size && b->data.house.bathhouse ? b->data.house.bathhouse / 10 : NO_COLUMN;
}

static int get_column_height_clinic(const struct building_t *b)
{
    return b->house_size && b->data.house.clinic ? b->data.house.clinic / 10 : NO_COLUMN;
}

static int get_column_height_hospital(const struct building_t *b)
{
    return b->house_size && b->data.house.hospital ? b->data.house.hospital / 10 : NO_COLUMN;
}
const struct city_overlay_t *city_overlay_for_barber(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_BARBER,
        COLUMN_TYPE_ACCESS,
        show_building_barber,
        show_figure_barber,
        get_column_height_barber,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_bathhouse(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_BATHHOUSE,
        COLUMN_TYPE_ACCESS,
        show_building_bathhouse,
        show_figure_bathhouse,
        get_column_height_bathhouse,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_clinic(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_CLINIC,
        COLUMN_TYPE_ACCESS,
        show_building_clinic,
        show_figure_clinic,
        get_column_height_clinic,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_hospital(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_HOSPITAL,
        COLUMN_TYPE_ACCESS,
        show_building_hospital,
        show_figure_hospital,
        get_column_height_hospital,
        0,
        0
    };
    return &overlay;
}
