#include "figure/figure.h"

#include "building/building.h"
#include "city/data_private.h"
#include "city/emperor.h"
#include "core/random.h"
#include "empire/object.h"
#include "figure/formation.h"
#include "figure/name.h"
#include "figure/route.h"
#include "figure/trader.h"
#include "map/figure.h"
#include "map/grid.h"

#include <string.h>

struct figure_t figures[MAX_FIGURES];

struct figure_t *figure_create(int type, int x, int y, direction_type dir)
{
    int id = 0;
    for (int i = 1; i < MAX_FIGURES; i++) {
        if (!figures[i].state) {
            id = i;
            break;
        }
    }
    if (!id) {
        return &figures[0];
    }

    struct figure_t *f = &figures[id];
    f->state = FIGURE_STATE_ALIVE;
    f->type = type;
    f->use_cross_country = 0;
    f->speed_multiplier = 1;
    f->direction = dir;
    f->source_x = f->destination_x = f->previous_tile_x = f->x = x;
    f->source_y = f->destination_y = f->previous_tile_y = f->y = y;
    f->grid_offset = map_grid_offset(x, y);
    f->cross_country_x = 15 * x;
    f->cross_country_y = 15 * y;
    f->progress_on_tile = 15;
    f->phrase_sequence_city = f->phrase_sequence_exact = random_byte() & 3;
    f->name_id = get_figure_name_id(f);
    map_figure_add(f);
    if (type == FIGURE_TRADE_CARAVAN || type == FIGURE_TRADE_SHIP) {
        f->trader_id = trader_create();
    }

    switch (f->type) {
        case FIGURE_IMMIGRANT:
        case FIGURE_EMIGRANT:
        case FIGURE_HOMELESS:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 20;
            break;
        case FIGURE_PATRICIAN:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 10;
            break;
        case FIGURE_CART_PUSHER:
        case FIGURE_LABOR_SEEKER:
        case FIGURE_BARBER:
        case FIGURE_BATHHOUSE_WORKER:
        case FIGURE_DOCTOR:
        case FIGURE_SURGEON:
        case FIGURE_PRIEST:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 20;
            break;
        case FIGURE_SCHOOL_CHILD:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 10;
            break;
        case FIGURE_TEACHER:
        case FIGURE_LIBRARIAN:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 20;
            break;
        case FIGURE_MISSIONARY:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 100;
            break;
        case FIGURE_ACTOR:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 20;
            break;
        case FIGURE_GLADIATOR:
            f->is_targetable = 1;
            f->is_friendly_armed_unit = 1;
            f->max_damage = 100;
            f->melee_attack_value = 9;
            f->melee_defense_value = 2;
            break;
        case FIGURE_LION_TAMER:
            f->is_targetable = 1;
            f->is_friendly_armed_unit = 1;
            f->max_damage = 100;
            f->melee_attack_value = 15;
            break;
        case FIGURE_CHARIOTEER:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 20;
            break;
        case FIGURE_TAX_COLLECTOR:
        case FIGURE_ENGINEER:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 20;
            break;
        case FIGURE_DOCKER:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 20;
            break;
        case FIGURE_FLOTSAM:
            break;
        case FIGURE_BALLISTA:
            f->is_friendly_armed_unit = 1;
            f->missile_attack_value = 8;
            f->missile_delay = 200;
            f->missile_type = FIGURE_BOLT;
            f->max_range = 15;
            break;
        case FIGURE_BOLT:
            f->missile_attack_value = 200;
            break;
        case FIGURE_TOWER_SENTRY:
            f->is_friendly_armed_unit = 1;
            f->max_damage = 50;
            f->melee_attack_value = 6;
            f->missile_attack_value = 6;
            f->missile_delay = 50;
            f->missile_type = FIGURE_JAVELIN;
            f->max_range = 12;
            break;
        case FIGURE_JAVELIN:
            f->missile_attack_value = 15;
            break;
        case FIGURE_PREFECT:
            f->is_targetable = 1;
            f->is_friendly_armed_unit = 1;
            f->max_damage = 50;
            f->melee_attack_value = 5;
            break;
        case FIGURE_FORT_JAVELIN:
            f->is_targetable = 1;
            f->is_player_legion_unit = 1;
            f->max_damage = 70;
            f->melee_attack_value = 4;
            f->missile_attack_value = 4;
            f->missile_delay = 100;
            f->missile_type = FIGURE_JAVELIN;
            f->speed_multiplier = 2;
            f->max_range = 10;
            break;
        case FIGURE_FORT_MOUNTED:
            f->is_targetable = 1;
            f->is_player_legion_unit = 1;
            f->max_damage = 110;
            f->melee_attack_value = 6;
            f->speed_multiplier = 3;
            break;
        case FIGURE_FORT_LEGIONARY:
            f->is_targetable = 1;
            f->is_player_legion_unit = 1;
            f->max_damage = 150;
            f->melee_attack_value = 10;
            f->melee_defense_value = 3;
            f->missile_defense_value = 6;
            break;
        case FIGURE_MARKET_BUYER:
        case FIGURE_MARKET_TRADER:
        case FIGURE_DELIVERY_BOY:
        case FIGURE_WAREHOUSEMAN:
            f->is_targetable = 1;
            f->is_unarmed_civilian_unit = 1;
            f->max_damage = 20;
            break;
        case FIGURE_PROTESTER:
        case FIGURE_CRIMINAL:
        case FIGURE_RIOTER:
            f->is_targetable = 1;
            f->is_criminal_unit = 1;
            f->max_damage = 30;
            break;
        case FIGURE_TRADE_CARAVAN:
        case FIGURE_TRADE_CARAVAN_DONKEY:
            f->is_targetable = 1;
            f->is_empire_trader = 1;
            f->max_damage = 10;
            break;
        case FIGURE_TRADE_SHIP:
            f->is_empire_trader = 1;
            break;
        case FIGURE_INDIGENOUS_NATIVE:
            f->is_targetable = 1;
            f->is_native_unit = 1;
            f->max_damage = 40;
            f->melee_attack_value = 6;
            break;
        case FIGURE_NATIVE_TRADER:
            f->is_targetable = 1;
            f->is_native_unit = 1;
            f->max_damage = 40;
            break;
        case FIGURE_WOLF:
            f->is_targetable = 1;
            f->is_herd_animal = 1;
            f->max_damage = 80;
            f->melee_attack_value = 8;
            break;
        case FIGURE_SHEEP:
            f->is_targetable = 1;
            f->is_herd_animal = 1;
            f->max_damage = 10;
            break;
        case FIGURE_ZEBRA:
            f->is_targetable = 1;
            f->is_herd_animal = 1;
            f->max_damage = 20;
            break;
        case FIGURE_ENEMY_GLADIATOR:
            f->is_targetable = 1;
            f->is_enemy_unit = 1;
            f->max_damage = 100;
            f->melee_attack_value = 9;
            f->melee_defense_value = 2;
            break;
        case FIGURE_ARROW:
            f->missile_attack_value = 5;
            break;
        default:
            break;
    }

    return f;
}

void figure_delete(struct figure_t *f)
{
    building *b = building_get(f->building_id);
    switch (f->type) {
        case FIGURE_LABOR_SEEKER:
        case FIGURE_MARKET_BUYER:
            if (f->building_id) {
                b->figure_id2 = 0;
            }
            break;
        case FIGURE_BALLISTA:
            b->figure_id4 = 0;
            break;
        case FIGURE_DOCKER:
            for (int i = 0; i < 3; i++) {
                if (b->data.dock.docker_ids[i] == f->id) {
                    b->data.dock.docker_ids[i] = 0;
                }
            }
            break;
        case FIGURE_ENEMY_CAESAR_LEGIONARY:
            city_data.emperor.invasion.soldiers_killed++;
            break;
        case FIGURE_EXPLOSION:
        case FIGURE_FORT_STANDARD:
        case FIGURE_ARROW:
        case FIGURE_JAVELIN:
        case FIGURE_BOLT:
        case FIGURE_FISH_GULLS:
        case FIGURE_SHEEP:
        case FIGURE_WOLF:
        case FIGURE_ZEBRA:
        case FIGURE_DELIVERY_BOY:
        case FIGURE_PATRICIAN:
            // nothing to do here
            break;
        default:
            if (f->building_id) {
                b->figure_id = 0;
            }
            break;
    }
    if (f->empire_city_id) {
        // remove trader
        for (int i = 0; i < 3; i++) {
            if (empire_objects[f->empire_city_id].trader_figure_ids[i] == f->id) {
                empire_objects[f->empire_city_id].trader_figure_ids[i] = 0;
            }
        }
    }
    if (f->immigrant_building_id) {
        b->immigrant_figure_id = 0;
    }
    figure_route_remove(f);
    map_figure_delete(f);

    int figure_id = f->id;
    memset(f, 0, sizeof(struct figure_t));
    f->id = figure_id;
}

int figure_is_dead(const struct figure_t *f)
{
    return f->state != FIGURE_STATE_ALIVE || f->action_state == FIGURE_ACTION_CORPSE;
}

void figure_handle_corpse(struct figure_t *f)
{
    if (f->wait_ticks < 0) {
        f->wait_ticks = 0;
    }
    f->wait_ticks++;
    if (f->wait_ticks >= 128) {
        f->wait_ticks = 127;
        f->state = FIGURE_STATE_DEAD;
    }
}

int city_figures_total_invading_enemies(void)
{
    return city_data.figure.imperial_soldiers + city_data.figure.enemies;
}

void figure_init_scenario(void)
{
    for (int i = 0; i < MAX_FIGURES; i++) {
        memset(&figures[i], 0, sizeof(struct figure_t));
        figures[i].id = i;
    }
}

static void figure_save(buffer *buf, const struct figure_t *f)
{
    buffer_write_u8(buf, f->is_targetable);
    buffer_write_u8(buf, f->is_unarmed_civilian_unit);
    buffer_write_u8(buf, f->is_friendly_armed_unit);
    buffer_write_u8(buf, f->is_player_legion_unit);
    buffer_write_u8(buf, f->is_criminal_unit);
    buffer_write_u8(buf, f->is_empire_trader);
    buffer_write_u8(buf, f->is_native_unit);
    buffer_write_u8(buf, f->is_herd_animal);
    buffer_write_u8(buf, f->is_enemy_unit);
    buffer_write_u8(buf, f->is_caesar_legion_unit);
    buffer_write_u8(buf, f->type);
    buffer_write_u8(buf, f->state);
    buffer_write_u8(buf, f->action_state);
    buffer_write_u8(buf, f->action_state_before_attack);
    buffer_write_u8(buf, f->formation_id);
    buffer_write_u8(buf, f->index_in_formation);
    buffer_write_u8(buf, f->damage);
    buffer_write_u8(buf, f->max_damage);
    buffer_write_u8(buf, f->melee_attack_value);
    buffer_write_u8(buf, f->melee_defense_value);
    buffer_write_u8(buf, f->missile_attack_value);
    buffer_write_u8(buf, f->missile_defense_value);
    buffer_write_u8(buf, f->missile_delay);
    buffer_write_u8(buf, f->missile_type);
    buffer_write_u8(buf, f->max_range);
    buffer_write_u8(buf, f->is_military_trained);
    buffer_write_u8(buf, f->mounted_charge_ticks);
    buffer_write_u8(buf, f->mounted_charge_ticks_max);
    buffer_write_u16(buf, f->target_figure_id);
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        buffer_write_u16(buf, f->melee_targeter_ids[i]);
    }
    for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
        buffer_write_u16(buf, f->melee_combatant_ids[i]);
    }
    buffer_write_u8(buf, f->num_melee_combatants);
    for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
        buffer_write_u16(buf, f->ranged_targeter_ids[i]);
    }
    buffer_write_u8(buf, f->prefect_recent_guard_duty);
    buffer_write_i8(buf, f->attack_direction);
    buffer_write_u8(buf, f->source_x);
    buffer_write_u8(buf, f->source_y);
    buffer_write_u16(buf, f->routing_path_id);
    buffer_write_u16(buf, f->routing_path_current_tile);
    buffer_write_u16(buf, f->routing_path_length);
    buffer_write_u8(buf, f->terrain_usage);
    buffer_write_u8(buf, f->speed_multiplier);
    buffer_write_i8(buf, f->previous_tile_direction);
    buffer_write_u8(buf, f->previous_tile_x);
    buffer_write_u8(buf, f->previous_tile_y);
    buffer_write_i8(buf, f->direction);
    buffer_write_u8(buf, f->progress_on_tile);
    buffer_write_u8(buf, f->x);
    buffer_write_u8(buf, f->y);
    buffer_write_u16(buf, f->grid_offset);
    buffer_write_u8(buf, f->destination_x);
    buffer_write_u8(buf, f->destination_y);
    buffer_write_u16(buf, f->destination_grid_offset);
    buffer_write_u16(buf, f->destination_building_id);
    buffer_write_u8(buf, f->figure_is_halted);
    buffer_write_u8(buf, f->use_cross_country);
    buffer_write_u8(buf, f->cc_direction);
    buffer_write_u16(buf, f->cross_country_x);
    buffer_write_u16(buf, f->cross_country_y);
    buffer_write_i16(buf, f->cc_delta_x);
    buffer_write_i16(buf, f->cc_delta_y);
    buffer_write_i16(buf, f->cc_delta_xy);
    buffer_write_u16(buf, f->cc_destination_x);
    buffer_write_u16(buf, f->cc_destination_y);
    buffer_write_u8(buf, f->missile_offset);
    buffer_write_u16(buf, f->roam_length);
    buffer_write_u16(buf, f->max_roam_length);
    buffer_write_u8(buf, f->roam_choose_destination);
    buffer_write_u8(buf, f->roam_random_counter);
    buffer_write_i8(buf, f->roam_turn_direction);
    buffer_write_i8(buf, f->roam_ticks_until_next_turn);
    buffer_write_u8(buf, f->in_building_wait_ticks);
    buffer_write_u8(buf, f->height_adjusted_ticks);
    buffer_write_u8(buf, f->current_height);
    buffer_write_u8(buf, f->target_height);
    buffer_write_u8(buf, f->is_boat);
    buffer_write_u16(buf, f->next_figure_id_on_same_tile);
    buffer_write_u16(buf, f->image_id);
    buffer_write_u8(buf, f->image_offset);
    buffer_write_u8(buf, f->attack_image_offset);
    buffer_write_u16(buf, f->cart_image_id);
    buffer_write_i8(buf, f->x_offset_cart);
    buffer_write_i8(buf, f->y_offset_cart);
    buffer_write_u8(buf, f->enemy_image_group);
    buffer_write_u8(buf, f->enemy_type);
    buffer_write_i16(buf, f->wait_ticks);
    buffer_write_u8(buf, f->wait_ticks_missile);
    buffer_write_u16(buf, f->name_id);
    buffer_write_u8(buf, f->is_ghost);
    buffer_write_u16(buf, f->building_id);
    buffer_write_u16(buf, f->immigrant_building_id);
    buffer_write_u8(buf, f->migrant_num_people);
    buffer_write_u8(buf, f->min_max_seen);
    buffer_write_u8(buf, f->phrase_sequence_exact);
    buffer_write_i8(buf, f->phrase_id);
    buffer_write_u8(buf, f->phrase_sequence_city);
    buffer_write_u8(buf, f->empire_city_id);
    buffer_write_u8(buf, f->resource_id);
    buffer_write_u8(buf, f->collecting_item_id);
    buffer_write_u8(buf, f->trader_id);
    buffer_write_u16(buf, f->leading_figure_id);
    buffer_write_u8(buf, f->trader_amount_bought);
    buffer_write_u8(buf, f->loads_sold_or_carrying);
    buffer_write_u8(buf, f->trade_ship_failed_dock_attempts);
    buffer_write_u8(buf, f->flotsam_visible);
}

static void figure_load(buffer *buf, struct figure_t *f)
{
    f->is_targetable = buffer_read_u8(buf);
    f->is_unarmed_civilian_unit = buffer_read_u8(buf);
    f->is_friendly_armed_unit = buffer_read_u8(buf);
    f->is_player_legion_unit = buffer_read_u8(buf);
    f->is_criminal_unit = buffer_read_u8(buf);
    f->is_empire_trader = buffer_read_u8(buf);
    f->is_native_unit = buffer_read_u8(buf);
    f->is_herd_animal = buffer_read_u8(buf);
    f->is_enemy_unit = buffer_read_u8(buf);
    f->is_caesar_legion_unit = buffer_read_u8(buf);
    f->type = buffer_read_u8(buf);
    f->state = buffer_read_u8(buf);
    f->action_state = buffer_read_u8(buf);
    f->action_state_before_attack = buffer_read_u8(buf);
    f->formation_id = buffer_read_u8(buf);
    f->index_in_formation = buffer_read_u8(buf);
    f->damage = buffer_read_u8(buf);
    f->max_damage = buffer_read_u8(buf);
    f->melee_attack_value = buffer_read_u8(buf);
    f->melee_defense_value = buffer_read_u8(buf);
    f->missile_attack_value = buffer_read_u8(buf);
    f->missile_defense_value = buffer_read_u8(buf);
    f->missile_delay = buffer_read_u8(buf);
    f->missile_type = buffer_read_u8(buf);
    f->max_range = buffer_read_u8(buf);
    f->is_military_trained = buffer_read_u8(buf);
    f->mounted_charge_ticks = buffer_read_u8(buf);
    f->mounted_charge_ticks_max = buffer_read_u8(buf);
    f->target_figure_id = buffer_read_u16(buf);
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        f->melee_targeter_ids[i] = buffer_read_u16(buf);
    }
    for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
        f->melee_combatant_ids[i] = buffer_read_u16(buf);
    }
    f->num_melee_combatants = buffer_read_u8(buf);
    for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
        f->ranged_targeter_ids[i] = buffer_read_u16(buf);
    }
    f->prefect_recent_guard_duty = buffer_read_u8(buf);
    f->attack_direction = buffer_read_i8(buf);
    f->source_x = buffer_read_u8(buf);
    f->source_y = buffer_read_u8(buf);
    f->routing_path_id = buffer_read_u16(buf);
    f->routing_path_current_tile = buffer_read_u16(buf);
    f->routing_path_length = buffer_read_u16(buf);
    f->terrain_usage = buffer_read_u8(buf);
    f->speed_multiplier = buffer_read_u8(buf);
    f->previous_tile_direction = buffer_read_i8(buf);
    f->previous_tile_x = buffer_read_u8(buf);
    f->previous_tile_y = buffer_read_u8(buf);
    f->direction = buffer_read_i8(buf);
    f->progress_on_tile = buffer_read_u8(buf);
    f->x = buffer_read_u8(buf);
    f->y = buffer_read_u8(buf);
    f->grid_offset = buffer_read_u16(buf);
    f->destination_x = buffer_read_u8(buf);
    f->destination_y = buffer_read_u8(buf);
    f->destination_grid_offset = buffer_read_u16(buf);
    f->destination_building_id = buffer_read_u16(buf);
    f->figure_is_halted = buffer_read_u8(buf);
    f->use_cross_country = buffer_read_u8(buf);
    f->cc_direction = buffer_read_u8(buf);
    f->cross_country_x = buffer_read_u16(buf);
    f->cross_country_y = buffer_read_u16(buf);
    f->cc_delta_x = buffer_read_i16(buf);
    f->cc_delta_y = buffer_read_i16(buf);
    f->cc_delta_xy = buffer_read_i16(buf);
    f->cc_destination_x = buffer_read_u16(buf);
    f->cc_destination_y = buffer_read_u16(buf);
    f->missile_offset = buffer_read_u8(buf);
    f->roam_length = buffer_read_u16(buf);
    f->max_roam_length = buffer_read_u16(buf);
    f->roam_choose_destination = buffer_read_u8(buf);
    f->roam_random_counter = buffer_read_u8(buf);
    f->roam_turn_direction = buffer_read_i8(buf);
    f->roam_ticks_until_next_turn = buffer_read_i8(buf);
    f->in_building_wait_ticks = buffer_read_u8(buf);
    f->height_adjusted_ticks = buffer_read_u8(buf);
    f->current_height = buffer_read_u8(buf);
    f->target_height = buffer_read_u8(buf);
    f->is_boat = buffer_read_u8(buf);
    f->next_figure_id_on_same_tile = buffer_read_u16(buf);
    f->image_id = buffer_read_u16(buf);
    f->image_offset = buffer_read_u8(buf);
    f->attack_image_offset = buffer_read_u8(buf);
    f->cart_image_id = buffer_read_u16(buf);
    f->x_offset_cart = buffer_read_i8(buf);
    f->y_offset_cart = buffer_read_i8(buf);
    f->enemy_image_group = buffer_read_u8(buf);
    f->enemy_type = buffer_read_u8(buf);
    f->wait_ticks = buffer_read_i16(buf);
    f->wait_ticks_missile = buffer_read_u8(buf);
    f->name_id = buffer_read_u16(buf);
    f->is_ghost = buffer_read_u8(buf);
    f->building_id = buffer_read_u16(buf);
    f->immigrant_building_id = buffer_read_u16(buf);
    f->migrant_num_people = buffer_read_u8(buf);
    f->min_max_seen = buffer_read_u8(buf);
    f->phrase_sequence_exact = buffer_read_u8(buf);
    f->phrase_id = buffer_read_i8(buf);
    f->phrase_sequence_city = buffer_read_u8(buf);
    f->empire_city_id = buffer_read_u8(buf);
    f->resource_id = buffer_read_u8(buf);
    f->collecting_item_id = buffer_read_u8(buf);
    f->trader_id = buffer_read_u8(buf);
    f->leading_figure_id = buffer_read_u16(buf);
    f->trader_amount_bought = buffer_read_u8(buf);
    f->loads_sold_or_carrying = buffer_read_u8(buf);
    f->trade_ship_failed_dock_attempts = buffer_read_u8(buf);
    f->flotsam_visible = buffer_read_u8(buf);
}

void figure_save_state(buffer *list)
{
    for (int i = 0; i < MAX_FIGURES; i++) {
        figure_save(list, &figures[i]);
    }
}

void figure_load_state(buffer *list)
{
    for (int i = 0; i < MAX_FIGURES; i++) {
        figure_load(list, &figures[i]);
        figures[i].id = i;
    }
}
