#ifndef BUILDING_BUILDING_H
#define BUILDING_BUILDING_H

#include "building/type.h"
#include "core/buffer.h"
#include "map/point.h"

#define MAX_BUILDINGS 2000

struct building_t {
    int id;
    unsigned char state;
    unsigned char size;
    unsigned char house_is_merged;
    unsigned char house_size;
    unsigned char x;
    unsigned char y;
    short grid_offset;
    short type;
    union {
        short house_level;
        short warehouse_resource_id;
        short workshop_type;
        short orientation;
        short fort_figure_type;
        short native_meeting_center_id;
    } subtype;
    unsigned char road_network_id;
    unsigned short created_sequence;
    short houses_covered;
    short percentage_houses_covered;
    short house_population;
    short house_population_room;
    short distance_from_entry;
    short house_highest_population;
    short house_unreachable_ticks;
    unsigned char road_access_x;
    unsigned char road_access_y;
    short figure_id;
    short figure_id2; // labor seeker or market buyer
    short immigrant_figure_id;
    short figure_id4; // tower ballista or burning ruin prefect
    unsigned char figure_spawn_delay;
    unsigned char figure_roam_direction;
    unsigned char has_water_access;
    short prev_part_building_id;
    short next_part_building_id;
    short loads_stored;
    unsigned char has_well_access;
    short num_workers;
    unsigned char labor_category;
    unsigned char output_resource_id;
    unsigned char has_road_access;
    unsigned char house_criminal_active;
    short damage_risk;
    short fire_risk;
    short fire_duration;
    unsigned char fire_proof; // cannot catch fire or collapse
    unsigned char house_figure_generation_delay;
    unsigned char house_tax_coverage;
    short formation_id;
    union {
        struct {
            short queued_docker_id;
            unsigned char num_ships;
            signed char orientation;
            short docker_ids[3];
            short trade_ship_id;
        } dock;
        struct {
            short inventory[8];
            short pottery_demand;
            short furniture_demand;
            short oil_demand;
            short wine_demand;
            unsigned char fetch_inventory_id;
        } market;
        struct {
            short resource_stored[16];
        } granary;
        struct {
            short progress;
            unsigned char blessing_days_left;
            unsigned char curse_days_left;
            unsigned char has_raw_materials;
            unsigned char has_fish;
            unsigned char orientation;
            short fishing_boat_id;
        } industry;
        struct {
            unsigned char num_shows;
            unsigned char days1;
            unsigned char days2;
            unsigned char play;
        } entertainment;
        struct {
            short inventory[8];
            unsigned char theater;
            unsigned char amphitheater_actor;
            unsigned char amphitheater_gladiator;
            unsigned char colosseum_gladiator;
            unsigned char colosseum_lion;
            unsigned char hippodrome;
            unsigned char school;
            unsigned char library;
            unsigned char academy;
            unsigned char barber;
            unsigned char clinic;
            unsigned char bathhouse;
            unsigned char hospital;
            unsigned char temple_ceres;
            unsigned char temple_neptune;
            unsigned char temple_mercury;
            unsigned char temple_mars;
            unsigned char temple_venus;
            unsigned char no_space_to_expand;
            unsigned char num_foods;
            unsigned char entertainment;
            unsigned char education;
            unsigned char health;
            unsigned char num_gods;
            unsigned char devolve_delay;
            unsigned char evolve_text_id;
        } house;
    } data;
    int tax_income_or_storage;
    unsigned char house_days_without_food;
    unsigned char ruin_has_plague;
    signed char desirability;
    unsigned char is_deleted;
    unsigned char storage_id;
    union {
        signed char house_happiness;
        signed char native_anger;
    } sentiment;
    unsigned char show_on_problem_overlay;
};

extern struct building_t all_buildings[MAX_BUILDINGS];

struct building_properties_t {
    uint8_t size;
    uint8_t fire_proof;
    uint8_t image_group;
    uint8_t image_offset;
    uint16_t cost;
    int8_t desirability_value;
    uint8_t desirability_step;
    int8_t desirability_step_size;
    uint8_t desirability_range;
    uint8_t laborers;
};

extern struct building_properties_t building_properties[BUILDING_TYPE_MAX];

struct house_properties_t {
    int8_t devolve_desirability; /**< Desirability at which the house devolves */
    int8_t evolve_desirability; /**< Desirability at which the house evolves */
    uint8_t entertainment; /**< Entertainment points required */
    uint8_t water; /**< Water required: 1 = well, 2 = fountain */
    uint8_t religion; /**< Number of gods required */
    uint8_t education; /**< Education required: 1 = school or library, 2 = school and library, 3 = school, library and academy */
    uint8_t barber; /**< Barber required (boolean) */
    uint8_t bathhouse; /**< Bathhouse required (boolean) */
    uint8_t health; /**< Health required: 1 = doctor or hospital, 2 = doctor and hospital */
    uint8_t food_types; /**< Number of food types required */
    uint8_t pottery; /**< Pottery required */
    uint8_t oil; /**< Oil required */
    uint8_t furniture; /**< Furniture required */
    uint8_t wine; /**< Wine types required: 1 = any wine, 2 = two types of wine */
    uint16_t prosperity; /**< Prosperity contribution */
    uint8_t max_people; /**< Maximum people per tile (medium insula and lower) or per house (large insula and up) */
    uint8_t tax_multiplier; /**< Tax rate multiplier */
};

extern struct house_properties_t house_properties[MAX_HOUSE_TYPES];

struct building_t *building_main(struct building_t *b);

struct building_t *building_next(struct building_t *b);

struct building_t *building_create(building_type type, int x, int y);

void building_clear_related_data(struct building_t *b);

void building_update_state(void);

void building_update_desirability(void);

int building_is_house(building_type type);

int building_is_fort(building_type type);

int building_get_highest_id(void);

void building_update_highest_id(void);

void set_destination__closest_building_of_type(int closest_to__building_id, int closest_building_of_type__type, map_point *closest_building_of_type__road_tile);

void building_totals_add_corrupted_house(int unfixable);

void building_clear_all(void);

void building_save_state(buffer *buf, buffer *highest_id, buffer *highest_id_ever,
                         buffer *sequence, buffer *corrupt_houses);

void building_load_state(buffer *buf, buffer *highest_id, buffer *highest_id_ever,
                         buffer *sequence, buffer *corrupt_houses);

#endif // BUILDING_BUILDING_H
