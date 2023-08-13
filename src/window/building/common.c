#include "common.h"

#include "building/building.h"
#include "city/data.h"
#include "city/labor.h"
#include "city/population.h"
#include "city/view.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "sound/sound.h"

void window_building_set_possible_position(int *x_offset, int *y_offset, int width_blocks, int height_blocks)
{
    int dialog_width = BLOCK_SIZE * width_blocks;
    int dialog_height = BLOCK_SIZE * height_blocks;
    int stub;
    int width;
    city_view_get_viewport(&stub, &stub, &width, &stub);
    width -= MARGIN_POSITION;

    if (*y_offset + dialog_height > screen_height() - MARGIN_POSITION) {
        *y_offset -= dialog_height;
    }

    *y_offset = (*y_offset < MIN_Y_POSITION) ? MIN_Y_POSITION : *y_offset;

    if (*x_offset + dialog_width > width) {
        *x_offset = width - dialog_width;
    }
}

int window_building_get_vertical_offset(struct building_info_context_t *c, int new_window_height)
{
    new_window_height = new_window_height * BLOCK_SIZE;
    int old_window_height = c->height_blocks * BLOCK_SIZE;
    int y_offset = c->y_offset;

    int center = (old_window_height / 2) + y_offset;
    int new_window_y = center - (new_window_height / 2);

    if (new_window_y < MIN_Y_POSITION) {
        new_window_y = MIN_Y_POSITION;
    } else {
        int height = screen_height() - MARGIN_POSITION;

        if (new_window_y + new_window_height > height) {
            new_window_y = height - new_window_height;
        }
    }

    return new_window_y;
}

static int draw_employment_info(struct building_t *b, int consider_house_covering)
{
    int text_id;
    if (b->num_workers >= building_properties[b->type].n_laborers) {
        text_id = 0;
    } else if (city_data.population.population <= 0) {
        text_id = 16; // no people in city
    } else if (!consider_house_covering) {
        text_id = 19;
    } else if (b->houses_covered <= 0) {
        text_id = 17; // no employees nearby
    } else if (b->houses_covered < 40) {
        text_id = 20; // poor access to employees
    } else if (city_labor_category(b->labor_category)->workers_allocated <= 0) {
        text_id = 18; // no people allocated
    } else {
        text_id = 19; // too few people allocated
    }
    if (!text_id && consider_house_covering && b->houses_covered < 40) {
        text_id = 20; // poor access to employees
    }
    return text_id;
}

static void draw_employment_details(struct building_info_context_t *c, struct building_t *b, int y_offset, int text_id)
{
    y_offset += c->y_offset;
    image_draw(image_group(GROUP_CONTEXT_ICONS) + 14, c->x_offset + 40, y_offset + 6);
    if (text_id) {
        int width = lang_text_draw_amount(8, 12, b->num_workers,
            c->x_offset + 60, y_offset + 10, FONT_NORMAL_BROWN);
        width += text_draw_number(building_properties[b->type].n_laborers, '(', "",
            c->x_offset + 70 + width, y_offset + 10, FONT_NORMAL_BROWN);
        lang_text_draw(69, 0, c->x_offset + 70 + width, y_offset + 10, FONT_NORMAL_BROWN);
        lang_text_draw(69, text_id, c->x_offset + 70, y_offset + 26, FONT_NORMAL_BROWN);
    } else {
        int width = lang_text_draw_amount(8, 12, b->num_workers,
            c->x_offset + 60, y_offset + 16, FONT_NORMAL_BROWN);
        width += text_draw_number(building_properties[b->type].n_laborers, '(', "",
            c->x_offset + 70 + width, y_offset + 16, FONT_NORMAL_BROWN);
        lang_text_draw(69, 0, c->x_offset + 70 + width, y_offset + 16, FONT_NORMAL_BROWN);
    }
}

void window_building_draw_employment(struct building_info_context_t *c, int y_offset)
{
    struct building_t *b = &all_buildings[c->building_id];
    int text_id = draw_employment_info(b, 1);
    draw_employment_details(c, b, y_offset, text_id);
}

void window_building_draw_employment_without_house_cover(struct building_info_context_t *c, int y_offset)
{
    struct building_t *b = &all_buildings[c->building_id];
    int text_id = draw_employment_info(b, 0);
    draw_employment_details(c, b, y_offset, text_id);
}

void window_building_draw_description(struct building_info_context_t *c, int text_group, int text_id)
{
    lang_text_draw_multiline(text_group, text_id, c->x_offset + 32, c->y_offset + 56,
        BLOCK_SIZE * (c->width_blocks - 4), FONT_NORMAL_BLACK);
}

void window_building_draw_description_at(struct building_info_context_t *c, int y_offset, int text_group, int text_id)
{
    lang_text_draw_multiline(text_group, text_id, c->x_offset + 32, c->y_offset + y_offset,
        BLOCK_SIZE * (c->width_blocks - 4), FONT_NORMAL_BLACK);
}

void window_building_play_sound(struct building_info_context_t *c, const char *sound_file)
{
    if (c->can_play_sound) {
        play_speech_file(sound_file);
        c->can_play_sound = 0;
    }
}
