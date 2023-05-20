#include "military.h"

#include "building/count.h"
#include "city/view.h"
#include "core/calc.h"
#include "core/log.h"
#include "figure/formation_legion.h"
#include "graphics/generic_button.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "sound/speech.h"
#include "window/city.h"

static void button_return_to_fort(int param1, int param2);
static void button_layout(int index, int param2);

static generic_button layout_buttons[] = {
    {19, 139, 84, 84, button_layout, button_none, 0, 0},
    {104, 139, 84, 84, button_layout, button_none, 1, 0},
    {189, 139, 84, 84, button_layout, button_none, 2, 0},
    {274, 139, 84, 84, button_layout, button_none, 3, 0},
    {359, 139, 84, 84, button_layout, button_none, 4, 0}
};

static generic_button return_button[] = {
    {0, 0, 288, 32, button_return_to_fort, button_none, 0, 0},
};

static struct {
    int focus_button_id;
    int return_button_id;
    building_info_context *context_for_callback;
} data;

void window_building_draw_wall(building_info_context *c)
{
    c->help_id = 85;
    window_building_play_sound(c, "wavs/wall.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(139, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 158, 139, 1);
}

void window_building_draw_gatehouse(building_info_context *c)
{
    c->help_id = 85;
    window_building_play_sound(c, "wavs/gatehouse.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(90, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 158, 90, 1);
}

void window_building_draw_tower(building_info_context *c)
{
    c->help_id = 85;
    window_building_play_sound(c, "wavs/tower.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(91, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);

    struct building_t *b = &all_buildings[c->building_id];
    if (!c->has_road_access) {
        window_building_draw_description(c, 69, 25);
    } else if (b->num_workers <= 0) {
        window_building_draw_description(c, 91, 2);
    } else if (b->figure_id) {
        window_building_draw_description(c, 91, 3);
    } else {
        window_building_draw_description(c, 91, 4);
    }
    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);
}

void window_building_draw_barracks(building_info_context *c)
{
    c->help_id = 37;
    window_building_play_sound(c, "wavs/barracks.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(136, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    image_draw(image_group(GROUP_RESOURCE_ICONS) + RESOURCE_WEAPONS, c->x_offset + 64, c->y_offset + 38);

    struct building_t *b = &all_buildings[c->building_id];
    if (b->loads_stored < 1) {
        lang_text_draw_amount(8, 10, 0, c->x_offset + 92, c->y_offset + 44, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw_amount(8, 10, b->loads_stored, c->x_offset + 92, c->y_offset + 44, FONT_NORMAL_BLACK);
    }

    if (!c->has_road_access) {
        window_building_draw_description_at(c, 70, 69, 25);
    } else if (b->num_workers <= 0) {
        window_building_draw_description_at(c, 70, 136, 3);
    } else if (!c->barracks_soldiers_requested) {
        window_building_draw_description_at(c, 70, 136, 4);
    } else {
        int offset = 0;
        if (b->loads_stored > 0) {
            offset = 4;
        }
        if (c->worker_percentage >= 100) {
            window_building_draw_description_at(c, 70, 136, 5 + offset);
        } else if (c->worker_percentage >= 66) {
            window_building_draw_description_at(c, 70, 136, 6 + offset);
        } else if (c->worker_percentage >= 33) {
            window_building_draw_description_at(c, 70, 136, 7 + offset);
        } else {
            window_building_draw_description_at(c, 70, 136, 8 + offset);
        }
    }
    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);
}

void window_building_draw_military_academy(building_info_context *c)
{
    c->help_id = 88;
    window_building_play_sound(c, "wavs/mil_acad.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(135, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);

    struct building_t *b = &all_buildings[c->building_id];
    if (!c->has_road_access) {
        window_building_draw_description(c, 69, 25);
    } else if (b->num_workers <= 0) {
        window_building_draw_description(c, 135, 2);
    } else if (c->worker_percentage < 100) {
        window_building_draw_description(c, 135, 1);
    } else {
        window_building_draw_description(c, 135, 3);
    }
    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);
}

void window_building_draw_fort(building_info_context *c)
{
    c->help_id = 87;
    window_building_play_sound(c, "wavs/fort.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(89, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 158, 89, legion_formations[c->formation_id].cursed_by_mars ? 1 : 2);
}

void window_building_draw_legion_info(building_info_context *c)
{
    int text_id;
    c->help_id = 87;
    struct formation_t *m = &legion_formations[c->formation_id];
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    // legion name
    lang_text_draw_centered(138, m->id, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    // animal symbol at the top of banner pole
    int icon_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_ICONS) + m->id;
    const image *icon_image = image_get(icon_image_id);
    image_draw(icon_image_id, c->x_offset + 16 + (40 - icon_image->width) / 2, c->y_offset + 16);
    // legion banner
    int flag_image_id = image_group(GROUP_FIGURE_FORT_FLAGS);
    if (m->figure_type == FIGURE_FORT_JAVELIN) {
        flag_image_id += 9;
    } else if (m->figure_type == FIGURE_FORT_MOUNTED) {
        flag_image_id += 18;
    }
    const image *flag_image = image_get(flag_image_id);
    image_draw(flag_image_id, c->x_offset + 16 + (40 - flag_image->width) / 2, c->y_offset + 16 + icon_image->height);
    // banner pole and morale ball
    int pole_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_POLE) + 20 - m->morale / 5;
    image_draw(pole_image_id, c->x_offset + 16 + (40 - image_get(pole_image_id)->width) / 2, c->y_offset + 16 + icon_image->height + flag_image->height);
    // number of soldiers
    lang_text_draw(138, 23, c->x_offset + 100, c->y_offset + 60, FONT_NORMAL_BLACK);
    text_draw_number(m->num_figures, ' ', "", c->x_offset + 294, c->y_offset + 60, FONT_NORMAL_BLACK);
    // health
    lang_text_draw(138, 24, c->x_offset + 100, c->y_offset + 80, FONT_NORMAL_BLACK);
    int formation_damage = 0;
    int formation_max_damage = 0;
    for (int i = 0; i < m->num_figures; i++) {
        struct figure_t *f = &figures[m->figures[i]];
        if (!figure_is_dead(f)) {
            formation_damage += f->damage;
            formation_max_damage += figure_properties[f->type].max_damage;
        }
    }
    int formation_damage_perc = calc_percentage(formation_damage, formation_max_damage);
    if (formation_damage_perc <= 0) {
        text_id = 26;
    } else if (formation_damage_perc <= 20) {
        text_id = 27;
    } else if (formation_damage_perc <= 40) {
        text_id = 28;
    } else if (formation_damage_perc <= 55) {
        text_id = 29;
    } else if (formation_damage_perc <= 70) {
        text_id = 30;
    } else if (formation_damage_perc <= 90) {
        text_id = 31;
    } else {
        text_id = 32;
    }
    lang_text_draw(138, text_id, c->x_offset + 300, c->y_offset + 80, FONT_NORMAL_BLACK);
    // military training
    lang_text_draw(138, 25, c->x_offset + 100, c->y_offset + 100, FONT_NORMAL_BLACK);
    lang_text_draw(18, m->has_military_training, c->x_offset + 300, c->y_offset + 100, FONT_NORMAL_BLACK);
    // morale
    if (m->cursed_by_mars) {
        lang_text_draw(138, 59, c->x_offset + 100, c->y_offset + 120, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw(138, 36, c->x_offset + 100, c->y_offset + 120, FONT_NORMAL_BLACK);
        lang_text_draw(138, 37 + m->morale / 5, c->x_offset + 300, c->y_offset + 120, FONT_NORMAL_BLACK);
    }
    if (m->num_figures) {
        // layout
        static const int OFFSETS_LEGIONARY[2][5] = {
            {0, 0, 2, 3, 4},
            {0, 0, 3, 2, 4},
        };
        static const int OFFSETS_OTHER[2][5] = {
            {5, 6, 2, 3, 4},
            {6, 5, 3, 2, 4},
        };
        const int *offsets;
        int rotation_index = 0;
        if (city_view_orientation() == DIR_6_LEFT || city_view_orientation() == DIR_2_RIGHT) {
            rotation_index = 1;
        }
        if (m->figure_type == FIGURE_FORT_LEGIONARY) {
            offsets = OFFSETS_LEGIONARY[rotation_index];
        } else {
            offsets = OFFSETS_OTHER[rotation_index];
        }
        for (int i = 0; i < 5; i++) {
            // for legionaries, draw tortoise formation in first position if academy trained, skip second position
            if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                if ((i == 0 && !m->has_military_training) || i == 1) {
                    continue;
                }
            }
            image_draw(image_group(GROUP_FORT_FORMATIONS) + offsets[i], c->x_offset + 21 + 85 * i, c->y_offset + 141);
        }
    } else {
        // no soldiers
        int group_id;
        if (m->cursed_by_mars) {
            group_id = 89;
            text_id = 1;
        } else if (building_count_active(BUILDING_BARRACKS)) {
            group_id = 138;
            text_id = 10;
        } else {
            group_id = 138;
            text_id = 11;
        }
        window_building_draw_description_at(c, 172, group_id, text_id);
    }
}

void window_building_draw_legion_info_foreground(building_info_context *c)
{
    struct formation_t *m = &legion_formations[c->formation_id];
    if (!m->num_figures) {
        return;
    }
    for (int i = 0; i < 5; i++) {
        // for legionaries, draw tortoise formation border in first position if academy trained, skip second position
        if (m->figure_type == FIGURE_FORT_LEGIONARY) {
            if ((i == 0 && !m->has_military_training) || i == 1) {
                continue;
            }
        }
        button_border_draw(c->x_offset + 19 + 85 * i, c->y_offset + 139, 84, 84, data.focus_button_id == i + 1);
    }

    inner_panel_draw(c->x_offset + 16, c->y_offset + 230, c->width_blocks - 2, 4);

    int title_id;
    int text_id;
    switch (data.focus_button_id) {
        case 1: // tortoise formation for legionaries, single line for aux
            if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                title_id = 12;
                text_id = m->has_military_training ? 18 : 17;
            } else {
                title_id = 16;
                text_id = 22;
            }
            break;
        case 2: // skip for legionaries, single line for aux
            if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                goto default_label;
            } else {
                title_id = 16;
                text_id = 22;
            }
            break;
        case 3: // double line
        case 4: // double line
            title_id = 14;
            text_id = 20;
            break;
        case 5: // mop up
            title_id = 15;
            text_id = 21;
            break;
        default_label:
        default:
            // no button selected: go for formation layout
            switch (m->layout) {
                case FORMATION_TORTOISE:
                    title_id = 12;
                    text_id = 18;
                    break;
                case FORMATION_SINGLE_LINE_1:
                case FORMATION_SINGLE_LINE_2:
                    title_id = 16;
                    text_id = 22;
                    break;
                case FORMATION_DOUBLE_LINE_1:
                case FORMATION_DOUBLE_LINE_2:
                    title_id = 14;
                    text_id = 20;
                    break;
                case FORMATION_MOP_UP:
                    title_id = 15;
                    text_id = 21;
                    break;
                default:
                    title_id = 16;
                    text_id = 22;
                    log_info("Unknown formation", 0, m->layout);
                    break;
            }
            break;
    }
    // draw formation info
    lang_text_draw(138, title_id, c->x_offset + 24, c->y_offset + 236, FONT_NORMAL_WHITE);
    lang_text_draw_multiline(138, text_id, c->x_offset + 24, c->y_offset + 252, BLOCK_SIZE * (c->width_blocks - 4), FONT_NORMAL_GREEN);

    // Return to fort
    if (!m->is_at_rest && !m->in_distant_battle) {
        button_border_draw(c->x_offset + BLOCK_SIZE * (c->width_blocks - 18) / 2, c->y_offset + BLOCK_SIZE * c->height_blocks - 48, 288, 32, data.return_button_id == 1);
        lang_text_draw_centered(138, 58, c->x_offset + BLOCK_SIZE * (c->width_blocks - 18) / 2, c->y_offset + BLOCK_SIZE * c->height_blocks - 39, 288, FONT_NORMAL_BLACK);
    }
}

int window_building_handle_mouse_legion_info(const mouse *m, building_info_context *c)
{
    data.context_for_callback = c;
    int handled = generic_buttons_handle_mouse(m, c->x_offset, c->y_offset, layout_buttons, sizeof(layout_buttons) / sizeof(generic_button), &data.focus_button_id);
    if (legion_formations[c->formation_id].figure_type == FIGURE_FORT_LEGIONARY) {
        if (data.focus_button_id == 2 || (data.focus_button_id == 1 && c->formation_types == 3)) {
            data.focus_button_id = 0;
        }
    }
    if (!handled) {
        handled = generic_buttons_handle_mouse(m, c->x_offset + BLOCK_SIZE * (c->width_blocks - 18) / 2,
            c->y_offset + BLOCK_SIZE * c->height_blocks - 48, return_button, 1, &data.return_button_id);
    }
    data.context_for_callback = 0;
    return handled;
}

static void button_return_to_fort(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    struct formation_t *m = &legion_formations[data.context_for_callback->formation_id];
    if (!m->in_distant_battle && !m->is_at_rest) {
        return_legion_formation_home(m);
        window_city_show();
    }
}

static void button_layout(int index, __attribute__((unused)) int param2)
{
    struct formation_t *m = &legion_formations[data.context_for_callback->formation_id];
    if (m->in_distant_battle || m->cursed_by_mars) {
        return;
    }

    if (m->figure_type == FIGURE_FORT_LEGIONARY) {
        switch (index) {
            case 0:
                if (m->has_military_training) {
                    m->layout = FORMATION_TORTOISE; break;
                } else {
                    return;
                }
            case 1: return;
            case 2: m->layout = FORMATION_DOUBLE_LINE_1; break;
            case 3: m->layout = FORMATION_DOUBLE_LINE_2; break;
            default: break;
        }
    } else {
        switch (index) {
            case 0: m->layout = FORMATION_SINGLE_LINE_1; break;
            case 1: m->layout = FORMATION_SINGLE_LINE_2; break;
            case 2: m->layout = FORMATION_DOUBLE_LINE_1; break;
            case 3: m->layout = FORMATION_DOUBLE_LINE_2; break;
            default: break;
        }
    }
    if (index == 4) { // mop up
        for (int i = 0; i < m->num_figures; i++) {
            struct figure_t *unit = &figures[m->figures[i]];
            if (unit->action_state != FIGURE_ACTION_CORPSE && unit->action_state != FIGURE_ACTION_FLEEING && unit->action_state != FIGURE_ACTION_ATTACK) {
                unit->action_state = FIGURE_ACTION_SOLDIER_MOPPING_UP;
            }
        }
    }

    switch (index) {
        case 0: sound_speech_play_file("wavs/cohort1.wav"); break;
        case 1: sound_speech_play_file("wavs/cohort2.wav"); break;
        case 2: sound_speech_play_file("wavs/cohort3.wav"); break;
        case 3: sound_speech_play_file("wavs/cohort4.wav"); break;
        case 4: sound_speech_play_file("wavs/cohort5.wav"); break;
    }
    window_city_military_show(data.context_for_callback->formation_id);
}
