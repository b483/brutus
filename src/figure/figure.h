#ifndef FIGURE_FIGURE_H
#define FIGURE_FIGURE_H

#include "core/buffer.h"
#include "game/orientation.h"

#define MAX_FIGURES 1000
#define MAX_RANGED_TARGETERS_PER_UNIT 4
#define MAX_MELEE_TARGETERS_PER_UNIT 4
#define MAX_MELEE_COMBATANTS_PER_UNIT 4

struct figure_t {
    int id;
    uint8_t in_use;
    uint8_t is_targetable;
    uint8_t is_corpse;
    uint8_t type;
    uint8_t action_state;
    uint8_t is_fleeing;
    uint8_t formation_id;
    uint8_t index_in_formation;
    uint8_t damage;
    uint8_t is_military_trained;
    uint8_t mounted_charge_ticks;
    uint8_t mounted_charge_ticks_max;
    uint16_t target_figure_id;
    uint16_t melee_targeter_ids[MAX_MELEE_TARGETERS_PER_UNIT];
    uint16_t melee_combatant_ids[MAX_MELEE_COMBATANTS_PER_UNIT];
    uint8_t num_melee_combatants;
    uint16_t ranged_targeter_ids[MAX_RANGED_TARGETERS_PER_UNIT];
    uint8_t prefect_recent_guard_duty;
    int8_t attack_direction;
    uint8_t engaged_in_combat;
    int8_t source_x;
    int8_t source_y;
    uint16_t routing_path_id;
    uint16_t routing_path_current_tile;
    uint16_t routing_path_length;
    uint8_t terrain_usage;
    uint8_t speed_multiplier;
    int8_t previous_tile_direction;
    int8_t previous_tile_x;
    int8_t previous_tile_y;
    int8_t direction;
    uint8_t progress_on_tile;
    int8_t x;
    int8_t y;
    uint16_t grid_offset;
    int8_t destination_x;
    int8_t destination_y;
    uint16_t destination_grid_offset; // only used for soldiers
    uint8_t destination_building_id;
    uint8_t figure_is_halted;
    uint8_t use_cross_country;
    uint8_t cc_direction; // 1 = x, 2 = y
    uint16_t cross_country_x; // position = 15 * x + offset on tile
    uint16_t cross_country_y; // position = 15 * y + offset on tile
    int16_t cc_delta_x;
    int16_t cc_delta_y;
    int16_t cc_delta_xy;
    uint16_t cc_destination_x;
    uint16_t cc_destination_y;
    uint8_t missile_offset;
    uint16_t roam_length;
    uint8_t roam_choose_destination;
    uint8_t roam_random_counter;
    int8_t roam_turn_direction;
    int8_t roam_ticks_until_next_turn;
    uint8_t in_building_wait_ticks;
    uint8_t height_adjusted_ticks;
    uint8_t current_height;
    uint8_t target_height;
    uint16_t next_figure_id_on_same_tile;
    uint16_t image_id;
    uint8_t image_offset;
    uint8_t attack_image_offset;
    uint8_t is_shooting;
    uint16_t cart_image_id;
    int8_t x_offset_cart;
    int8_t y_offset_cart;
    uint8_t enemy_image_group;
    int16_t wait_ticks;
    uint8_t wait_ticks_missile;
    uint16_t name_id;
    uint8_t is_invisible;
    uint16_t building_id; // for missiles: building_id contains the figure that shot it
    uint16_t immigrant_building_id;
    uint8_t migrant_num_people;
    uint8_t min_max_seen;
    uint8_t phrase_sequence_exact;
    int8_t phrase_id;
    uint8_t phrase_sequence_city;
    uint8_t empire_city_id;
    uint8_t resource_id;
    uint8_t collecting_item_id; // NOT a resource ID for cartpushers! IS a resource ID for warehousemen
    uint8_t trader_id;
    uint16_t leading_figure_id;
    uint8_t trader_amount_bought;
    uint8_t loads_sold_or_carrying;
    uint8_t trade_ship_failed_dock_attempts;
    uint8_t flotsam_visible;
};

extern struct figure_t figures[MAX_FIGURES];

enum {
    FIGURE_NONE = 0,
    FIGURE_IMMIGRANT = 1,
    FIGURE_EMIGRANT = 2,
    FIGURE_HOMELESS = 3,
    FIGURE_PATRICIAN = 4,
    FIGURE_CART_PUSHER = 5,
    FIGURE_LABOR_SEEKER = 6,
    FIGURE_BARBER = 7,
    FIGURE_BATHHOUSE_WORKER = 8,
    FIGURE_DOCTOR = 9,
    FIGURE_SURGEON = 10,
    FIGURE_PRIEST = 11,
    FIGURE_SCHOOL_CHILD = 12,
    FIGURE_TEACHER = 13,
    FIGURE_LIBRARIAN = 14,
    FIGURE_MISSIONARY = 15,
    FIGURE_ACTOR = 16,
    FIGURE_GLADIATOR = 17,
    FIGURE_LION_TAMER = 18,
    FIGURE_CHARIOTEER = 19,
    FIGURE_HIPPODROME_HORSES = 20,
    FIGURE_TAX_COLLECTOR = 21,
    FIGURE_ENGINEER = 22,
    FIGURE_FISHING_BOAT = 23,
    FIGURE_FISH_GULLS = 24,
    FIGURE_SHIPWRECK = 25,
    FIGURE_DOCKER = 26,
    FIGURE_FLOTSAM = 27,
    FIGURE_BALLISTA = 28,
    FIGURE_BOLT = 29,
    FIGURE_TOWER_SENTRY = 30,
    FIGURE_JAVELIN = 31,
    FIGURE_PREFECT = 32,
    FIGURE_FORT_STANDARD = 33,
    FIGURE_FORT_JAVELIN = 34,
    FIGURE_FORT_MOUNTED = 35,
    FIGURE_FORT_LEGIONARY = 36,
    FIGURE_MARKET_BUYER = 37,
    FIGURE_MARKET_TRADER = 38,
    FIGURE_DELIVERY_BOY = 39,
    FIGURE_WAREHOUSEMAN = 40,
    FIGURE_PROTESTER = 41,
    FIGURE_CRIMINAL = 42,
    FIGURE_RIOTER = 43,
    FIGURE_TRADE_CARAVAN = 44,
    FIGURE_TRADE_CARAVAN_DONKEY = 45,
    FIGURE_TRADE_SHIP = 46,
    FIGURE_INDIGENOUS_NATIVE = 47,
    FIGURE_NATIVE_TRADER = 48,
    FIGURE_WOLF = 49,
    FIGURE_SHEEP = 50,
    FIGURE_ZEBRA = 51,
    FIGURE_ENEMY_GLADIATOR = 52,
    FIGURE_ENEMY_BARBARIAN_SWORDSMAN = 53,
    FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN = 54,
    FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT = 55,
    FIGURE_ENEMY_BRITON_SWORDSMAN = 56,
    FIGURE_ENEMY_BRITON_CHARIOT = 57,
    FIGURE_ENEMY_CELT_SWORDSMAN = 58,
    FIGURE_ENEMY_CELT_CHARIOT = 59,
    FIGURE_ENEMY_PICT_SWORDSMAN = 60,
    FIGURE_ENEMY_PICT_CHARIOT = 61,
    FIGURE_ENEMY_EGYPTIAN_SWORDSMAN = 62,
    FIGURE_ENEMY_EGYPTIAN_CAMEL = 63,
    FIGURE_ENEMY_ETRUSCAN_SWORDSMAN = 64,
    FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER = 65,
    FIGURE_ENEMY_SAMNITE_SWORDSMAN = 66,
    FIGURE_ENEMY_SAMNITE_SPEAR_THROWER = 67,
    FIGURE_ENEMY_GAUL_SWORDSMAN = 68,
    FIGURE_ENEMY_GAUL_AXEMAN = 69,
    FIGURE_ENEMY_HELVETIUS_SWORDSMAN = 70,
    FIGURE_ENEMY_HELVETIUS_AXEMAN = 71,
    FIGURE_ENEMY_HUN_SWORDSMAN = 72,
    FIGURE_ENEMY_HUN_MOUNTED_ARCHER = 73,
    FIGURE_ENEMY_GOTH_SWORDSMAN = 74,
    FIGURE_ENEMY_GOTH_MOUNTED_ARCHER = 75,
    FIGURE_ENEMY_VISIGOTH_SWORDSMAN = 76,
    FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER = 77,
    FIGURE_ENEMY_GREEK_SWORDSMAN = 78,
    FIGURE_ENEMY_GREEK_SPEAR_THROWER = 79,
    FIGURE_ENEMY_MACEDONIAN_SWORDSMAN = 80,
    FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER = 81,
    FIGURE_ENEMY_NUMIDIAN_SWORDSMAN = 82,
    FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER = 83,
    FIGURE_ENEMY_PERGAMUM_SWORDSMAN = 84,
    FIGURE_ENEMY_PERGAMUM_ARCHER = 85,
    FIGURE_ENEMY_IBERIAN_SWORDSMAN = 86,
    FIGURE_ENEMY_IBERIAN_SPEAR_THROWER = 87,
    FIGURE_ENEMY_JUDEAN_SWORDSMAN = 88,
    FIGURE_ENEMY_JUDEAN_SPEAR_THROWER = 89,
    FIGURE_ENEMY_SELEUCID_SWORDSMAN = 90,
    FIGURE_ENEMY_SELEUCID_SPEAR_THROWER = 91,
    FIGURE_ENEMY_CAESAR_JAVELIN = 92,
    FIGURE_ENEMY_CAESAR_MOUNTED = 93,
    FIGURE_ENEMY_CAESAR_LEGIONARY = 94,
    FIGURE_ARROW = 95,
    FIGURE_MAP_FLAG = 96,
    FIGURE_EXPLOSION = 97,
    FIGURE_TYPE_MAX = 98,
};

struct figure_properties_t {
    uint8_t is_unarmed_civilian_unit;
    uint8_t is_friendly_armed_unit; // excl. player legions
    uint8_t is_player_legion_unit;
    uint8_t is_criminal_unit;
    uint8_t is_empire_trader;
    uint8_t is_native_unit;
    uint8_t is_herd_animal;
    uint8_t is_enemy_unit; // excl. Caesar legions
    uint8_t is_caesar_legion_unit;
    uint8_t melee_attack_value;
    uint8_t melee_defense_value;
    uint8_t missile_attack_value;
    uint8_t missile_defense_value;
    uint8_t missile_delay;
    uint8_t missile_type;
    uint8_t max_range;
    uint8_t max_damage;
    uint16_t max_roam_length;
    uint8_t is_boat; // 1 for boat, 2 for flotsam
};

extern struct figure_properties_t figure_properties[FIGURE_TYPE_MAX];

extern const int MISSILE_LAUNCHER_OFFSETS[128];

enum {
    ENEMY_TYPE_BARBARIAN = 0,
    ENEMY_TYPE_CARTHAGINIAN = 1,
    ENEMY_TYPE_BRITON = 2,
    ENEMY_TYPE_CELT = 3,
    ENEMY_TYPE_PICT = 4,
    ENEMY_TYPE_EGYPTIAN = 5,
    ENEMY_TYPE_ETRUSCAN = 6,
    ENEMY_TYPE_SAMNITE = 7,
    ENEMY_TYPE_GAUL = 8,
    ENEMY_TYPE_HELVETIUS = 9,
    ENEMY_TYPE_HUN = 10,
    ENEMY_TYPE_GOTH = 11,
    ENEMY_TYPE_VISIGOTH = 12,
    ENEMY_TYPE_GREEK = 13,
    ENEMY_TYPE_MACEDONIAN = 14,
    ENEMY_TYPE_NUMIDIAN = 15,
    ENEMY_TYPE_PERGAMUM = 16,
    ENEMY_TYPE_IBERIAN = 17,
    ENEMY_TYPE_JUDEAN = 18,
    ENEMY_TYPE_SELEUCID = 19,
    ENEMY_TYPE_CAESAR = 20
};

enum {
    ENEMY_IMG_TYPE_BARBARIAN = 0,
    ENEMY_IMG_TYPE_CARTHAGINIAN = 1,
    ENEMY_IMG_TYPE_CELT = 2,
    ENEMY_IMG_TYPE_EGYPTIAN = 3,
    ENEMY_IMG_TYPE_ETRUSCAN = 4,
    ENEMY_IMG_TYPE_GAUL = 5,
    ENEMY_IMG_TYPE_GOTH = 6,
    ENEMY_IMG_TYPE_GREEK = 7,
    ENEMY_IMG_TYPE_NORTH_AFRICAN = 8,
    ENEMY_IMG_TYPE_PERSIAN = 9,
    ENEMY_IMG_TYPE_PHOENICIAN = 10,
    ENEMY_IMG_TYPE_CAESAR = 11
};

enum {
    FIGURE_ACTION_IMMIGRANT_CREATED = 1,
    FIGURE_ACTION_IMMIGRANT_ARRIVING = 2,
    FIGURE_ACTION_IMMIGRANT_ENTERING_HOUSE = 3,
    FIGURE_ACTION_EMIGRANT_CREATED = 4,
    FIGURE_ACTION_EMIGRANT_EXITING_HOUSE = 5,
    FIGURE_ACTION_EMIGRANT_LEAVING = 6,
    FIGURE_ACTION_HOMELESS_CREATED = 7,
    FIGURE_ACTION_HOMELESS_GOING_TO_HOUSE = 8,
    FIGURE_ACTION_HOMELESS_ENTERING_HOUSE = 9,
    FIGURE_ACTION_HOMELESS_LEAVING = 10,
    FIGURE_ACTION_CARTPUSHER_INITIAL = 11,
    FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE = 12,
    FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY = 13,
    FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WORKSHOP = 14,
    FIGURE_ACTION_CARTPUSHER_AT_WAREHOUSE = 15,
    FIGURE_ACTION_CARTPUSHER_AT_GRANARY = 16,
    FIGURE_ACTION_CARTPUSHER_AT_WORKSHOP = 17,
    FIGURE_ACTION_CARTPUSHER_RETURNING = 18,
    FIGURE_ACTION_ROAMING = 19,
    FIGURE_ACTION_ROAMER_RETURNING = 20,
    FIGURE_ACTION_ENTERTAINER_AT_SCHOOL_CREATED = 21,
    FIGURE_ACTION_ENTERTAINER_EXITING_SCHOOL = 22,
    FIGURE_ACTION_ENTERTAINER_GOING_TO_VENUE = 23,
    FIGURE_ACTION_ENTERTAINER_ROAMING = 24,
    FIGURE_ACTION_ENTERTAINER_RETURNING = 25,
    FIGURE_ACTION_TAX_COLLECTOR_CREATED = 26,
    FIGURE_ACTION_TAX_COLLECTOR_ENTERING_EXITING = 27,
    FIGURE_ACTION_TAX_COLLECTOR_ROAMING = 28,
    FIGURE_ACTION_TAX_COLLECTOR_RETURNING = 29,
    FIGURE_ACTION_ENGINEER_CREATED = 30,
    FIGURE_ACTION_ENGINEER_ENTERING_EXITING = 31,
    FIGURE_ACTION_ENGINEER_ROAMING = 32,
    FIGURE_ACTION_ENGINEER_RETURNING = 33,
    FIGURE_ACTION_DOCKER_IDLING = 34,
    FIGURE_ACTION_DOCKER_IMPORT_QUEUE = 35,
    FIGURE_ACTION_DOCKER_EXPORT_QUEUE = 36,
    FIGURE_ACTION_DOCKER_IMPORT_GOING_TO_WAREHOUSE = 37,
    FIGURE_ACTION_DOCKER_EXPORT_GOING_TO_WAREHOUSE = 38,
    FIGURE_ACTION_DOCKER_EXPORT_RETURNING = 39,
    FIGURE_ACTION_DOCKER_IMPORT_RETURNING = 40,
    FIGURE_ACTION_DOCKER_IMPORT_AT_WAREHOUSE = 41,
    FIGURE_ACTION_DOCKER_EXPORT_AT_WAREHOUSE = 42,
    FIGURE_ACTION_PREFECT_CREATED = 43,
    FIGURE_ACTION_PREFECT_ENTERING_EXITING = 44,
    FIGURE_ACTION_PREFECT_ROAMING = 45,
    FIGURE_ACTION_PREFECT_RETURNING = 46,
    FIGURE_ACTION_PREFECT_GOING_TO_FIRE = 47,
    FIGURE_ACTION_PREFECT_AT_FIRE = 48,
    FIGURE_ACTION_SOLDIER_AT_REST = 49,
    FIGURE_ACTION_SOLDIER_GOING_TO_FORT = 50,
    FIGURE_ACTION_SOLDIER_RETURNING_TO_BARRACKS = 51,
    FIGURE_ACTION_SOLDIER_GOING_TO_STANDARD = 52,
    FIGURE_ACTION_SOLDIER_AT_STANDARD = 53,
    FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY = 54,
    FIGURE_ACTION_SOLDIER_MOPPING_UP = 55,
    FIGURE_ACTION_SOLDIER_GOING_TO_DISTANT_BATTLE = 56,
    FIGURE_ACTION_SOLDIER_RETURNING_FROM_DISTANT_BATTLE = 57,
    FIGURE_ACTION_TOWER_SENTRY_AT_REST = 58,
    FIGURE_ACTION_TOWER_SENTRY_PATROLLING = 59,
    FIGURE_ACTION_TOWER_SENTRY_RETURNING = 60,
    FIGURE_ACTION_TOWER_SENTRY_GOING_TO_TOWER = 61,
    FIGURE_ACTION_MARKET_BUYER_GOING_TO_STORAGE = 62,
    FIGURE_ACTION_MARKET_BUYER_RETURNING = 63,
    FIGURE_ACTION_WAREHOUSEMAN_CREATED = 64,
    FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE = 65,
    FIGURE_ACTION_WAREHOUSEMAN_AT_DELIVERY_BUILDING = 66,
    FIGURE_ACTION_WAREHOUSEMAN_RETURNING_EMPTY = 67,
    FIGURE_ACTION_WAREHOUSEMAN_GETTING_FOOD = 68,
    FIGURE_ACTION_WAREHOUSEMAN_AT_GRANARY = 69,
    FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_FOOD = 70,
    FIGURE_ACTION_WAREHOUSEMAN_GETTING_RESOURCE = 71,
    FIGURE_ACTION_WAREHOUSEMAN_AT_WAREHOUSE = 72,
    FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_RESOURCE = 73,
    FIGURE_ACTION_TRADE_CARAVAN_CREATED = 74,
    FIGURE_ACTION_TRADE_CARAVAN_ARRIVING = 75,
    FIGURE_ACTION_TRADE_CARAVAN_TRADING = 76,
    FIGURE_ACTION_TRADE_CARAVAN_LEAVING = 77,
    FIGURE_ACTION_TRADE_SHIP_CREATED = 78,
    FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK = 79,
    FIGURE_ACTION_TRADE_SHIP_MOORED = 80,
    FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK_QUEUE = 81,
    FIGURE_ACTION_TRADE_SHIP_ANCHORED = 82,
    FIGURE_ACTION_TRADE_SHIP_LEAVING = 83,
    FIGURE_ACTION_RIOTER_CREATED = 84,
    FIGURE_ACTION_RIOTER_MOVING = 85,
    FIGURE_ACTION_ENEMY_SPAWNING = 86,
    FIGURE_ACTION_ENEMY_REGROUPING = 87,
    FIGURE_ACTION_ENEMY_ADVANCING = 88,
    FIGURE_ACTION_ENEMY_ENGAGED = 89,
    FIGURE_ACTION_NATIVE_GOING_TO_MEETING_CENTER = 90,
    FIGURE_ACTION_NATIVE_RETURNING_FROM_MEETING = 91,
    FIGURE_ACTION_NATIVE_CREATED = 92,
    FIGURE_ACTION_NATIVE_ATTACKING = 93,
    FIGURE_ACTION_NATIVE_TRADER_GOING_TO_WAREHOUSE = 94,
    FIGURE_ACTION_NATIVE_TRADER_RETURNING = 95,
    FIGURE_ACTION_NATIVE_TRADER_CREATED = 96,
    FIGURE_ACTION_NATIVE_TRADER_AT_WAREHOUSE = 97,
    FIGURE_ACTION_BALLISTA_READY = 98,
    FIGURE_ACTION_FISHING_BOAT_CREATED = 99,
    FIGURE_ACTION_FISHING_BOAT_GOING_TO_FISH = 100,
    FIGURE_ACTION_FISHING_BOAT_FISHING = 101,
    FIGURE_ACTION_FISHING_BOAT_GOING_TO_WHARF = 102,
    FIGURE_ACTION_FISHING_BOAT_AT_WHARF = 103,
    FIGURE_ACTION_FISHING_BOAT_RETURNING_WITH_FISH = 104,
    FIGURE_ACTION_HERD_ANIMAL_AT_REST = 105,
    FIGURE_ACTION_HERD_ANIMAL_MOVING = 106,
    FIGURE_ACTION_HIPPODROME_HORSE_CREATED = 107,
    FIGURE_ACTION_HIPPODROME_HORSE_RACING = 108,
    FIGURE_ACTION_HIPPODROME_HORSE_DONE = 109,
    FIGURE_ACTION_FLOTSAM_CREATED = 110,
    FIGURE_ACTION_FLOTSAM_FLOATING = 111,
    FIGURE_ACTION_FLOTSAM_OFF_MAP = 112,
};

enum {
    TERRAIN_USAGE_ANY = 0,
    TERRAIN_USAGE_ROADS = 1,
    TERRAIN_USAGE_ENEMY = 2,
    TERRAIN_USAGE_PREFER_ROADS = 3,
    TERRAIN_USAGE_WALLS = 4,
    TERRAIN_USAGE_ANIMAL = 5,
};

void figure_init_scenario(void);

struct figure_t *figure_create(int type, int x, int y, int dir);

void figure_delete(struct figure_t *f);

void figure_image_increase_offset(struct figure_t *f, int max);

void figure_image_set_cart_offset(struct figure_t *f, int direction);

int figure_image_direction(struct figure_t *f);

int figure_image_normalize_direction(int direction);

int get_direction(struct figure_t *f);

int figure_is_alive(const struct figure_t *f);

void figure_handle_corpse(struct figure_t *f);

int city_figures_total_invading_enemies(void);

void rout_unit(struct figure_t *f);

void figure_save_state(struct buffer_t *list);

void figure_load_state(struct buffer_t *list);

#endif // FIGURE_FIGURE_H
