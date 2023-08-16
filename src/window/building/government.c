#include "government.h"

#include "building/building.h"
#include "city/resource.h"
#include "graphics/graphics.h"
#include "window/building/figures.h"

void window_building_draw_forum(struct building_info_context_t *c)
{
    c->help_id = 76;
    window_building_play_sound(c, "wavs/forum.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(106, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    image_draw(COIN_IMAGE_ID, c->x_offset + 16, c->y_offset + 36);

    struct building_t *b = &all_buildings[c->building_id];
    int width = lang_text_draw(106, 2, c->x_offset + 44, c->y_offset + 43, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 0, b->tax_income_or_storage,
        c->x_offset + 44 + width, c->y_offset + 43, FONT_NORMAL_BLACK);

    if (!c->has_road_access) {
        window_building_draw_description(c, 69, 25);
    } else if (b->num_workers <= 0) {
        window_building_draw_description_at(c, 72, 106, 10);
    } else if (c->worker_percentage >= 100) {
        window_building_draw_description_at(c, 72, 106, 5);
    } else if (c->worker_percentage >= 75) {
        window_building_draw_description_at(c, 72, 106, 6);
    } else if (c->worker_percentage >= 50) {
        window_building_draw_description_at(c, 72, 106, 7);
    } else if (c->worker_percentage >= 25) {
        window_building_draw_description_at(c, 72, 106, 8);
    } else {
        window_building_draw_description_at(c, 72, 106, 9);
    }

    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);
}

void window_building_draw_senate(struct building_info_context_t *c)
{
    c->can_go_to_advisor = 1;
    c->help_id = 77;
    window_building_play_sound(c, "wavs/senate.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(105, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    image_draw(COIN_IMAGE_ID, c->x_offset + 16, c->y_offset + 36);

    struct building_t *b = &all_buildings[c->building_id];
    int width = lang_text_draw(105, 2, c->x_offset + 44, c->y_offset + 43, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 0, b->tax_income_or_storage,
        c->x_offset + 44 + width, c->y_offset + 43, FONT_NORMAL_BLACK);

    if (!c->has_road_access) {
        window_building_draw_description(c, 69, 25);
    } else if (b->num_workers <= 0) {
        window_building_draw_description_at(c, 72, 106, 10);
    } else if (c->worker_percentage >= 100) {
        window_building_draw_description_at(c, 72, 106, 5);
    } else if (c->worker_percentage >= 75) {
        window_building_draw_description_at(c, 72, 106, 6);
    } else if (c->worker_percentage >= 50) {
        window_building_draw_description_at(c, 72, 106, 7);
    } else if (c->worker_percentage >= 25) {
        window_building_draw_description_at(c, 72, 106, 8);
    } else {
        window_building_draw_description_at(c, 72, 106, 9);
    }

    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);

    lang_text_draw(105, 3, c->x_offset + 60, c->y_offset + 220, FONT_NORMAL_BLACK);
}

void window_building_draw_governor_home(struct building_info_context_t *c)
{
    c->help_id = 78;
    window_building_play_sound(c, "wavs/gov_palace.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(103, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 143, 103, 1);
}

void window_building_draw_garden(struct building_info_context_t *c)
{
    c->help_id = 80;
    window_building_play_sound(c, "wavs/park.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(79, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 158, 79, 1);
}

void window_building_draw_plaza(struct building_info_context_t *c)
{
    c->help_id = 80;
    window_building_play_sound(c, "wavs/plaza.wav");
    window_building_prepare_figure_list(c);
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(137, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    window_building_draw_figure_list(c);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 113, 137, 1);
}

void window_building_draw_statue(struct building_info_context_t *c)
{
    c->help_id = 79;
    window_building_play_sound(c, "wavs/statue.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(80, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 158, 80, 1);
}

void window_building_draw_triumphal_arch(struct building_info_context_t *c)
{
    c->help_id = 79;
    window_building_play_sound(c, "wavs/statue.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(80, 2, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 158, 80, 3);
}
