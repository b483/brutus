#include "city_overlay_education.h"

#include "game/state.h"

static int show_building_education(const struct building_t *b)
{
    return b->type == BUILDING_SCHOOL || b->type == BUILDING_LIBRARY || b->type == BUILDING_ACADEMY;
}

static int show_building_school(const struct building_t *b)
{
    return b->type == BUILDING_SCHOOL;
}

static int show_building_library(const struct building_t *b)
{
    return b->type == BUILDING_LIBRARY;
}

static int show_building_academy(const struct building_t *b)
{
    return b->type == BUILDING_ACADEMY;
}

static int show_figure_education(const struct figure_t *f)
{
    return f->type == FIGURE_SCHOOL_CHILD || f->type == FIGURE_LIBRARIAN || f->type == FIGURE_TEACHER;
}

static int show_figure_school(const struct figure_t *f)
{
    return f->type == FIGURE_SCHOOL_CHILD;
}

static int show_figure_library(const struct figure_t *f)
{
    return f->type == FIGURE_LIBRARIAN;
}

static int show_figure_academy(const struct figure_t *f)
{
    return f->type == FIGURE_TEACHER;
}

static int get_column_height_education(const struct building_t *b)
{
    return b->house_size && b->data.house.education ? b->data.house.education * 3 - 1 : NO_COLUMN;
}

static int get_column_height_school(const struct building_t *b)
{
    return b->house_size && b->data.house.school ? b->data.house.school / 10 : NO_COLUMN;
}

static int get_column_height_library(const struct building_t *b)
{
    return b->house_size && b->data.house.library ? b->data.house.library / 10 : NO_COLUMN;
}

static int get_column_height_academy(const struct building_t *b)
{
    return b->house_size && b->data.house.academy ? b->data.house.academy / 10 : NO_COLUMN;
}

const struct city_overlay_t *city_overlay_for_education(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_EDUCATION,
        COLUMN_TYPE_ACCESS,
        show_building_education,
        show_figure_education,
        get_column_height_education,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_school(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_SCHOOL,
        COLUMN_TYPE_ACCESS,
        show_building_school,
        show_figure_school,
        get_column_height_school,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_library(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_LIBRARY,
        COLUMN_TYPE_ACCESS,
        show_building_library,
        show_figure_library,
        get_column_height_library,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_academy(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_ACADEMY,
        COLUMN_TYPE_ACCESS,
        show_building_academy,
        show_figure_academy,
        get_column_height_academy,
        0,
        0
    };
    return &overlay;
}
