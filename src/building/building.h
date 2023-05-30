#ifndef BUILDING_BUILDING_H
#define BUILDING_BUILDING_H

#include "core/buffer.h"
#include "map/point.h"

#define MAX_BUILDINGS 2000
#define MAX_HOUSE_TYPES 20

enum {
    BUILDING_NONE = 0,
    BUILDING_HOUSE_VACANT_LOT = 1,
    BUILDING_HOUSE_SMALL_TENT = 2,
    BUILDING_HOUSE_LARGE_TENT = 3,
    BUILDING_HOUSE_SMALL_SHACK = 4,
    BUILDING_HOUSE_LARGE_SHACK = 5,
    BUILDING_HOUSE_SMALL_HOVEL = 6,
    BUILDING_HOUSE_LARGE_HOVEL = 7,
    BUILDING_HOUSE_SMALL_CASA = 8,
    BUILDING_HOUSE_LARGE_CASA = 9,
    BUILDING_HOUSE_SMALL_INSULA = 10,
    BUILDING_HOUSE_MEDIUM_INSULA = 11,
    BUILDING_HOUSE_LARGE_INSULA = 12,
    BUILDING_HOUSE_GRAND_INSULA = 13,
    BUILDING_HOUSE_SMALL_VILLA = 14,
    BUILDING_HOUSE_MEDIUM_VILLA = 15,
    BUILDING_HOUSE_LARGE_VILLA = 16,
    BUILDING_HOUSE_GRAND_VILLA = 17,
    BUILDING_HOUSE_SMALL_PALACE = 18,
    BUILDING_HOUSE_MEDIUM_PALACE = 19,
    BUILDING_HOUSE_LARGE_PALACE = 20,
    BUILDING_HOUSE_LUXURY_PALACE = 21,
    BUILDING_CLEAR_LAND = 22,
    BUILDING_ROAD = 23,
    BUILDING_RESERVOIR = 24,
    BUILDING_AQUEDUCT = 25,
    BUILDING_FOUNTAIN = 26,
    BUILDING_WELL = 27,
    BUILDING_DOCTOR = 28,
    BUILDING_BATHHOUSE = 29,
    BUILDING_BARBER = 30,
    BUILDING_HOSPITAL = 31,
    BUILDING_SMALL_TEMPLE_CERES = 32,
    BUILDING_SMALL_TEMPLE_NEPTUNE = 33,
    BUILDING_SMALL_TEMPLE_MERCURY = 34,
    BUILDING_SMALL_TEMPLE_MARS = 35,
    BUILDING_SMALL_TEMPLE_VENUS = 36,
    BUILDING_LARGE_TEMPLE_CERES = 37,
    BUILDING_LARGE_TEMPLE_NEPTUNE = 38,
    BUILDING_LARGE_TEMPLE_MERCURY = 39,
    BUILDING_LARGE_TEMPLE_MARS = 40,
    BUILDING_LARGE_TEMPLE_VENUS = 41,
    BUILDING_ORACLE = 42,
    BUILDING_SCHOOL = 43,
    BUILDING_LIBRARY = 44,
    BUILDING_ACADEMY = 45,
    BUILDING_MISSION_POST = 46,
    BUILDING_THEATER = 47,
    BUILDING_ACTOR_COLONY = 48,
    BUILDING_AMPHITHEATER = 49,
    BUILDING_GLADIATOR_SCHOOL = 50,
    BUILDING_LION_HOUSE = 51,
    BUILDING_COLOSSEUM = 52,
    BUILDING_CHARIOT_MAKER = 53,
    BUILDING_HIPPODROME = 54,
    BUILDING_GARDENS = 55,
    BUILDING_PLAZA = 56,
    BUILDING_SMALL_STATUE = 57,
    BUILDING_MEDIUM_STATUE = 58,
    BUILDING_LARGE_STATUE = 59,
    BUILDING_GOVERNORS_HOUSE = 60,
    BUILDING_GOVERNORS_VILLA = 61,
    BUILDING_GOVERNORS_PALACE = 62,
    BUILDING_FORUM = 63,
    BUILDING_SENATE = 64,
    BUILDING_TRIUMPHAL_ARCH = 65,
    BUILDING_ENGINEERS_POST = 66,
    BUILDING_LOW_BRIDGE = 67,
    BUILDING_SHIP_BRIDGE = 68,
    BUILDING_SHIPYARD = 69,
    BUILDING_WHARF = 70,
    BUILDING_DOCK = 71,
    BUILDING_PREFECTURE = 72,
    BUILDING_WALL = 73,
    BUILDING_TOWER = 74,
    BUILDING_GATEHOUSE = 75,
    BUILDING_FORT_LEGIONARIES = 76,
    BUILDING_FORT_JAVELIN = 77,
    BUILDING_FORT_MOUNTED = 78,
    BUILDING_BARRACKS = 79,
    BUILDING_MILITARY_ACADEMY = 80,
    BUILDING_WHEAT_FARM = 81,
    BUILDING_VEGETABLE_FARM = 82,
    BUILDING_FRUIT_FARM = 83,
    BUILDING_PIG_FARM = 84,
    BUILDING_OLIVE_FARM = 85,
    BUILDING_VINES_FARM = 86,
    BUILDING_CLAY_PIT = 87,
    BUILDING_TIMBER_YARD = 88,
    BUILDING_MARBLE_QUARRY = 89,
    BUILDING_IRON_MINE = 90,
    BUILDING_OIL_WORKSHOP = 91,
    BUILDING_WINE_WORKSHOP = 92,
    BUILDING_POTTERY_WORKSHOP = 93,
    BUILDING_FURNITURE_WORKSHOP = 94,
    BUILDING_WEAPONS_WORKSHOP = 95,
    BUILDING_MARKET = 96,
    BUILDING_GRANARY = 97,
    BUILDING_WAREHOUSE = 98,
    BUILDING_WAREHOUSE_SPACE = 99,
    BUILDING_NATIVE_HUT = 100,
    BUILDING_NATIVE_MEETING = 101,
    BUILDING_NATIVE_CROPS = 102,
    BUILDING_FORT_GROUND = 103,
    BUILDING_BURNING_RUIN = 104,
    // helper constants
    BUILDING_TYPE_MAX = 105
};

/**
 * House levels
 */
enum {
    HOUSE_SMALL_TENT = 0,
    HOUSE_LARGE_TENT = 1,
    HOUSE_SMALL_SHACK = 2,
    HOUSE_LARGE_SHACK = 3,
    HOUSE_SMALL_HOVEL = 4,
    HOUSE_LARGE_HOVEL = 5,
    HOUSE_SMALL_CASA = 6,
    HOUSE_LARGE_CASA = 7,
    HOUSE_SMALL_INSULA = 8,
    HOUSE_MEDIUM_INSULA = 9,
    HOUSE_LARGE_INSULA = 10,
    HOUSE_GRAND_INSULA = 11,
    HOUSE_SMALL_VILLA = 12,
    HOUSE_MEDIUM_VILLA = 13,
    HOUSE_LARGE_VILLA = 14,
    HOUSE_GRAND_VILLA = 15,
    HOUSE_SMALL_PALACE = 16,
    HOUSE_MEDIUM_PALACE = 17,
    HOUSE_LARGE_PALACE = 18,
    HOUSE_LUXURY_PALACE = 19,
};

enum {
    BUILDING_STATE_UNUSED = 0,
    BUILDING_STATE_IN_USE = 1,
    BUILDING_STATE_UNDO = 2,
    BUILDING_STATE_CREATED = 3,
    BUILDING_STATE_RUBBLE = 4,
    BUILDING_STATE_DELETED_BY_GAME = 5, // used for earthquakes, fires, house mergers
    BUILDING_STATE_DELETED_BY_PLAYER = 6
};


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
    int8_t labor_category;
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
    uint8_t n_laborers;
    int8_t labor_category;
    uint8_t sound_channel;
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

extern uint8_t all_buildings_strings[][23];

struct building_t *building_main(struct building_t *b);

struct building_t *building_next(struct building_t *b);

struct building_t *building_create(int type, int x, int y);

void building_clear_related_data(struct building_t *b);

void building_update_state(void);

void building_update_desirability(void);

int align_bulding_type_index_to_strings(int building_type_index);

int building_is_house(int type);

int building_is_fort(int type);

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
