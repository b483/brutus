#include "house_evolution.h"

#include "building/house.h"
#include "city/houses.h"
#include "city/resource.h"
#include "core/calc.h"
#include "game/resource.h"
#include "game/time.h"
#include "game/undo.h"
#include "map/building.h"
#include "map/grid.h"
#include "map/routing_terrain.h"
#include "map/tiles.h"

typedef enum {
    EVOLVE = 1,
    NONE = 0,
    DEVOLVE = -1
} evolve_status;

static int check_evolve_desirability(struct building_t *house)
{
    int level = house->subtype.house_level;
    int evolve_des = house_properties[level].evolve_desirability;
    if (level >= HOUSE_LUXURY_PALACE) {
        evolve_des = 1000;
    }
    int current_des = house->desirability;
    int status;
    if (current_des <= house_properties[level].devolve_desirability) {
        status = DEVOLVE;
    } else if (current_des >= evolve_des) {
        status = EVOLVE;
    } else {
        status = NONE;
    }
    house->data.house.evolve_text_id = status; // BUG? -1 in an unsigned char?
    return status;
}

static int has_required_goods_and_services(struct building_t *house, int for_upgrade, house_demands *demands)
{
    int level = house->subtype.house_level;
    if (for_upgrade) {
        ++level;
    }
    // water
    if (!house->has_water_access) {
        if (house_properties[level].water >= 2) {
            ++demands->missing.fountain;
            return 0;
        }
        if (house_properties[level].water == 1 && !house->has_well_access) {
            ++demands->missing.well;
            return 0;
        }
    }
    // entertainment
    if (house->data.house.entertainment < house_properties[level].entertainment) {
        if (house->data.house.entertainment) {
            ++demands->missing.more_entertainment;
        } else {
            ++demands->missing.entertainment;
        }
        return 0;
    }
    // education
    if (house->data.house.education < house_properties[level].education) {
        if (house->data.house.education) {
            ++demands->missing.more_education;
        } else {
            ++demands->missing.education;
        }
        return 0;
    }
    if (house_properties[level].education == 2) {
        ++demands->requiring.school;
        ++demands->requiring.library;
    } else if (house_properties[level].education == 1) {
        ++demands->requiring.school;
    }
    // religion
    if (house->data.house.num_gods < house_properties[level].religion) {
        if (house_properties[level].religion == 1) {
            ++demands->missing.religion;
            return 0;
        } else if (house_properties[level].religion == 2) {
            ++demands->missing.second_religion;
            return 0;
        } else if (house_properties[level].religion == 3) {
            ++demands->missing.third_religion;
            return 0;
        }
    } else if (house_properties[level].religion > 0) {
        ++demands->requiring.religion;
    }
    // barber
    if (house->data.house.barber < house_properties[level].barber) {
        ++demands->missing.barber;
        return 0;
    }
    if (house_properties[level].barber == 1) {
        ++demands->requiring.barber;
    }
    // bathhouse
    if (house->data.house.bathhouse < house_properties[level].bathhouse) {
        ++demands->missing.bathhouse;
        return 0;
    }
    if (house_properties[level].bathhouse == 1) {
        ++demands->requiring.bathhouse;
    }
    // health
    if (house->data.house.health < house_properties[level].health) {
        if (house_properties[level].health < 2) {
            ++demands->missing.clinic;
        } else {
            ++demands->missing.hospital;
        }
        return 0;
    }
    if (house_properties[level].health >= 1) {
        ++demands->requiring.clinic;
    }
    // food types
    int foodtypes_available = 0;
    for (int i = INVENTORY_MIN_FOOD; i < INVENTORY_MAX_FOOD; i++) {
        if (house->data.house.inventory[i]) {
            foodtypes_available++;
        }
    }
    if (foodtypes_available < house_properties[level].food_types) {
        ++demands->missing.food;
        return 0;
    }
    // goods
    if (house->data.house.inventory[INVENTORY_POTTERY] < house_properties[level].pottery) {
        return 0;
    }
    if (house->data.house.inventory[INVENTORY_OIL] < house_properties[level].oil) {
        return 0;
    }
    if (house->data.house.inventory[INVENTORY_FURNITURE] < house_properties[level].furniture) {
        return 0;
    }
    int wine = house_properties[level].wine;
    if (wine && house->data.house.inventory[INVENTORY_WINE] <= 0) {
        return 0;
    }
    if (wine > 1 && !city_resource_multiple_wine_available()) {
        ++demands->missing.second_wine;
        return 0;
    }
    return 1;
}

static int check_requirements(struct building_t *house, house_demands *demands)
{
    int status = check_evolve_desirability(house);
    if (!has_required_goods_and_services(house, 0, demands)) {
        status = DEVOLVE;
    } else if (status == EVOLVE) {
        status = has_required_goods_and_services(house, 1, demands);
    }
    return status;
}

static int has_devolve_delay(struct building_t *house, evolve_status status)
{
    if (status == DEVOLVE && house->data.house.devolve_delay < 2) {
        house->data.house.devolve_delay++;
        return 1;
    } else {
        house->data.house.devolve_delay = 0;
        return 0;
    }
}

static int evolve_small_tent(struct building_t *house, house_demands *demands)
{
    if (house->house_population > 0) {
        building_house_merge(house);
        evolve_status status = check_requirements(house, demands);
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_TENT);
        }
    }
    return 0;
}

static int evolve_large_tent(struct building_t *house, house_demands *demands)
{
    if (house->house_population > 0) {
        building_house_merge(house);
        evolve_status status = check_requirements(house, demands);
        if (!has_devolve_delay(house, status)) {
            if (status == EVOLVE) {
                building_house_change_to(house, BUILDING_HOUSE_SMALL_SHACK);
            } else if (status == DEVOLVE) {
                building_house_change_to(house, BUILDING_HOUSE_SMALL_TENT);
            }
        }
    }
    return 0;
}

static int evolve_small_shack(struct building_t *house, house_demands *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_SHACK);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_TENT);
        }
    }
    return 0;
}

static int evolve_large_shack(struct building_t *house, house_demands *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_HOVEL);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_SHACK);
        }
    }
    return 0;
}

static int evolve_small_hovel(struct building_t *house, house_demands *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_HOVEL);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_SHACK);
        }
    }
    return 0;
}

static int evolve_large_hovel(struct building_t *house, house_demands *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_CASA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_HOVEL);
        }
    }
    return 0;
}

static int evolve_small_casa(struct building_t *house, house_demands *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_CASA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_HOVEL);
        }
    }
    return 0;
}

static int evolve_large_casa(struct building_t *house, house_demands *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_INSULA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_CASA);
        }
    }
    return 0;
}

static int evolve_small_insula(struct building_t *house, house_demands *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_MEDIUM_INSULA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_CASA);
        }
    }
    return 0;
}

static int evolve_medium_insula(struct building_t *house, house_demands *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            if (building_house_can_expand(house, 4)) {
                game_undo_disable();
                house->house_is_merged = 0;
                building_house_expand_to_large_insula(house);
                map_tiles_update_all_gardens();
                return 1;
            }
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_INSULA);
        }
    }
    return 0;
}

static int evolve_large_insula(struct building_t *house, house_demands *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_INSULA);
        } else if (status == DEVOLVE) {
            game_undo_disable();
            building_house_devolve_from_large_insula(house);
        }
    }
    return 0;
}

static int evolve_grand_insula(struct building_t *house, house_demands *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_VILLA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_INSULA);
        }
    }
    return 0;
}

static int evolve_small_villa(struct building_t *house, house_demands *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_MEDIUM_VILLA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_INSULA);
        }
    }
    return 0;
}

static int evolve_medium_villa(struct building_t *house, house_demands *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            if (building_house_can_expand(house, 9)) {
                game_undo_disable();
                building_house_expand_to_large_villa(house);
                map_tiles_update_all_gardens();
                return 1;
            }
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_VILLA);
        }
    }
    return 0;
}

static int evolve_large_villa(struct building_t *house, house_demands *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_VILLA);
        } else if (status == DEVOLVE) {
            game_undo_disable();
            building_house_devolve_from_large_villa(house);
        }
    }
    return 0;
}

static int evolve_grand_villa(struct building_t *house, house_demands *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_PALACE);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_VILLA);
        }
    }
    return 0;
}

static int evolve_small_palace(struct building_t *house, house_demands *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_MEDIUM_PALACE);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_VILLA);
        }
    }
    return 0;
}

static int evolve_medium_palace(struct building_t *house, house_demands *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            if (building_house_can_expand(house, 16)) {
                game_undo_disable();
                building_house_expand_to_large_palace(house);
                map_tiles_update_all_gardens();
                return 1;
            }
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_PALACE);
        }
    }
    return 0;
}

static int evolve_large_palace(struct building_t *house, house_demands *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LUXURY_PALACE);
        } else if (status == DEVOLVE) {
            game_undo_disable();
            building_house_devolve_from_large_palace(house);
        }
    }
    return 0;
}

static int evolve_luxury_palace(struct building_t *house, house_demands *demands)
{
    int status = check_evolve_desirability(house);
    if (!has_required_goods_and_services(house, 0, demands)) {
        status = DEVOLVE;
    }
    if (!has_devolve_delay(house, status) && status == DEVOLVE) {
        building_house_change_to(house, BUILDING_HOUSE_LARGE_PALACE);
    }
    return 0;
}

static void consume_resource(struct building_t *b, int inventory, int amount)
{
    if (amount > 0) {
        if (amount > b->data.house.inventory[inventory]) {
            b->data.house.inventory[inventory] = 0;
        } else {
            b->data.house.inventory[inventory] -= amount;
        }
    }
}

static void consume_resources(struct building_t *b)
{
    consume_resource(b, INVENTORY_POTTERY, house_properties[b->subtype.house_level].pottery);
    consume_resource(b, INVENTORY_FURNITURE, house_properties[b->subtype.house_level].furniture);
    consume_resource(b, INVENTORY_OIL, house_properties[b->subtype.house_level].oil);
    consume_resource(b, INVENTORY_WINE, house_properties[b->subtype.house_level].wine);
}

static int (*evolve_callback[])(struct building_t *, house_demands *) = {
    evolve_small_tent, evolve_large_tent, evolve_small_shack, evolve_large_shack,
    evolve_small_hovel, evolve_large_hovel, evolve_small_casa, evolve_large_casa,
    evolve_small_insula, evolve_medium_insula, evolve_large_insula, evolve_grand_insula,
    evolve_small_villa, evolve_medium_villa, evolve_large_villa, evolve_grand_villa,
    evolve_small_palace, evolve_medium_palace, evolve_large_palace, evolve_luxury_palace
};

void building_house_process_evolve_and_consume_goods(void)
{
    city_houses_reset_demands();
    house_demands *demands = city_houses_demands();
    int has_expanded = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && building_is_house(b->type) && b->type != BUILDING_HOUSE_VACANT_LOT) {
            building_house_check_for_corruption(b);
            has_expanded |= evolve_callback[b->type - BUILDING_HOUSE_SMALL_TENT](b, demands);
            if (game_time_day() == 0 || game_time_day() == 7) {
                consume_resources(b);
            }
        }
    }
    if (has_expanded) {
        map_routing_update_land();
    }
}

void building_house_determine_evolve_text(struct building_t *house, int worst_desirability_building)
{
    int level = house->subtype.house_level;

    // this house will devolve soon because...

    // desirability
    if (house->desirability <= house_properties[level].devolve_desirability) {
        house->data.house.evolve_text_id = 0;
        return;
    }
    // water
    if (house_properties[level].water == 1 && !house->has_water_access && !house->has_well_access) {
        house->data.house.evolve_text_id = 1;
        return;
    }
    if (house_properties[level].water == 2 && !house->has_water_access) {
        house->data.house.evolve_text_id = 2;
        return;
    }
    // entertainment
    if (house->data.house.entertainment < house_properties[level].entertainment) {
        if (!house->data.house.entertainment) {
            house->data.house.evolve_text_id = 3;
        } else if (house_properties[level].entertainment < 10) {
            house->data.house.evolve_text_id = 4;
        } else if (house_properties[level].entertainment < 25) {
            house->data.house.evolve_text_id = 5;
        } else if (house_properties[level].entertainment < 50) {
            house->data.house.evolve_text_id = 6;
        } else if (house_properties[level].entertainment < 80) {
            house->data.house.evolve_text_id = 7;
        } else {
            house->data.house.evolve_text_id = 8;
        }
        return;
    }
    // food types
    int foodtypes_available = 0;
    for (int i = INVENTORY_MIN_FOOD; i < INVENTORY_MAX_FOOD; i++) {
        if (house->data.house.inventory[i]) {
            foodtypes_available++;
        }
    }
    if (foodtypes_available < house_properties[level].food_types) {
        if (house_properties[level].food_types == 1) {
            house->data.house.evolve_text_id = 9;
            return;
        } else if (house_properties[level].food_types == 2) {
            house->data.house.evolve_text_id = 10;
            return;
        } else if (house_properties[level].food_types == 3) {
            house->data.house.evolve_text_id = 11;
            return;
        }
    }
    // education
    if (house->data.house.education < house_properties[level].education) {
        if (house_properties[level].education == 1) {
            house->data.house.evolve_text_id = 14;
            return;
        } else if (house_properties[level].education == 2) {
            if (house->data.house.school) {
                house->data.house.evolve_text_id = 15;
                return;
            } else if (house->data.house.library) {
                house->data.house.evolve_text_id = 16;
                return;
            }
        } else if (house_properties[level].education == 3) {
            house->data.house.evolve_text_id = 17;
            return;
        }
    }
    // bathhouse
    if (house->data.house.bathhouse < house_properties[level].bathhouse) {
        house->data.house.evolve_text_id = 18;
        return;
    }
    // pottery
    if (house->data.house.inventory[INVENTORY_POTTERY] < house_properties[level].pottery) {
        house->data.house.evolve_text_id = 19;
        return;
    }
    // religion
    if (house->data.house.num_gods < house_properties[level].religion) {
        if (house_properties[level].religion == 1) {
            house->data.house.evolve_text_id = 20;
            return;
        } else if (house_properties[level].religion == 2) {
            house->data.house.evolve_text_id = 21;
            return;
        } else if (house_properties[level].religion == 3) {
            house->data.house.evolve_text_id = 22;
            return;
        }
    }
    // barber
    if (house->data.house.barber < house_properties[level].barber) {
        house->data.house.evolve_text_id = 23;
        return;
    }
    // health
    if (house->data.house.health < house_properties[level].health) {
        if (house_properties[level].health == 1) {
            house->data.house.evolve_text_id = 24;
        } else if (house->data.house.clinic) {
            house->data.house.evolve_text_id = 25;
        } else {
            house->data.house.evolve_text_id = 26;
        }
        return;
    }
    // oil
    if (house->data.house.inventory[INVENTORY_OIL] < house_properties[level].oil) {
        house->data.house.evolve_text_id = 27;
        return;
    }
    // furniture
    if (house->data.house.inventory[INVENTORY_FURNITURE] < house_properties[level].furniture) {
        house->data.house.evolve_text_id = 28;
        return;
    }
    // wine
    if (house->data.house.inventory[INVENTORY_WINE] < house_properties[level].wine) {
        house->data.house.evolve_text_id = 29;
        return;
    }
    if (house_properties[level].wine > 1 && !city_resource_multiple_wine_available()) {
        house->data.house.evolve_text_id = 65;
        return;
    }
    if (level >= HOUSE_LUXURY_PALACE) { // max level!
        house->data.house.evolve_text_id = 60;
        return;
    }

    // this house will evolve if ...

    // desirability
    if (house->desirability < house_properties[level].evolve_desirability) {
        if (worst_desirability_building) {
            house->data.house.evolve_text_id = 62;
        } else {
            house->data.house.evolve_text_id = 30;
        }
        return;
    }
    // water
    if (house_properties[level + 1].water == 1 && !house->has_water_access && !house->has_well_access) {
        house->data.house.evolve_text_id = 31;
        return;
    }
    if (house_properties[level + 1].water == 2 && !house->has_water_access) {
        house->data.house.evolve_text_id = 32;
        return;
    }
    // entertainment
    if (house->data.house.entertainment < house_properties[level + 1].entertainment) {
        if (!house->data.house.entertainment) {
            house->data.house.evolve_text_id = 33;
        } else if (house_properties[level + 1].entertainment < 10) {
            house->data.house.evolve_text_id = 34;
        } else if (house_properties[level + 1].entertainment < 25) {
            house->data.house.evolve_text_id = 35;
        } else if (house_properties[level + 1].entertainment < 50) {
            house->data.house.evolve_text_id = 36;
        } else if (house_properties[level + 1].entertainment < 80) {
            house->data.house.evolve_text_id = 37;
        } else {
            house->data.house.evolve_text_id = 38;
        }
        return;
    }
    // food types
    if (foodtypes_available < house_properties[level + 1].food_types) {
        if (house_properties[level + 1].food_types == 1) {
            house->data.house.evolve_text_id = 39;
            return;
        } else if (house_properties[level + 1].food_types == 2) {
            house->data.house.evolve_text_id = 40;
            return;
        } else if (house_properties[level + 1].food_types == 3) {
            house->data.house.evolve_text_id = 41;
            return;
        }
    }
    // education
    if (house->data.house.education < house_properties[level + 1].education) {
        if (house_properties[level + 1].education == 1) {
            house->data.house.evolve_text_id = 44;
            return;
        } else if (house_properties[level + 1].education == 2) {
            if (house->data.house.school) {
                house->data.house.evolve_text_id = 45;
                return;
            } else if (house->data.house.library) {
                house->data.house.evolve_text_id = 46;
                return;
            }
        } else if (house_properties[level + 1].education == 3) {
            house->data.house.evolve_text_id = 47;
            return;
        }
    }
    // bathhouse
    if (house->data.house.bathhouse < house_properties[level + 1].bathhouse) {
        house->data.house.evolve_text_id = 48;
        return;
    }
    // pottery
    if (house->data.house.inventory[INVENTORY_POTTERY] < house_properties[level + 1].pottery) {
        house->data.house.evolve_text_id = 49;
        return;
    }
    // religion
    if (house->data.house.num_gods < house_properties[level + 1].religion) {
        if (house_properties[level + 1].religion == 1) {
            house->data.house.evolve_text_id = 50;
            return;
        } else if (house_properties[level + 1].religion == 2) {
            house->data.house.evolve_text_id = 51;
            return;
        } else if (house_properties[level + 1].religion == 3) {
            house->data.house.evolve_text_id = 52;
            return;
        }
    }
    // barber
    if (house->data.house.barber < house_properties[level + 1].barber) {
        house->data.house.evolve_text_id = 53;
        return;
    }
    // health
    if (house->data.house.health < house_properties[level + 1].health) {
        if (house_properties[level + 1].health == 1) {
            house->data.house.evolve_text_id = 54;
        } else if (house->data.house.clinic) {
            house->data.house.evolve_text_id = 55;
        } else {
            house->data.house.evolve_text_id = 56;
        }
        return;
    }
    // oil
    if (house->data.house.inventory[INVENTORY_OIL] < house_properties[level + 1].oil) {
        house->data.house.evolve_text_id = 57;
        return;
    }
    // furniture
    if (house->data.house.inventory[INVENTORY_FURNITURE] < house_properties[level + 1].furniture) {
        house->data.house.evolve_text_id = 58;
        return;
    }
    // wine
    if (house->data.house.inventory[INVENTORY_WINE] < house_properties[level + 1].wine) {
        house->data.house.evolve_text_id = 59;
        return;
    }
    if (house_properties[level + 1].wine > 1 && !city_resource_multiple_wine_available()) {
        house->data.house.evolve_text_id = 66;
        return;
    }
    // house is evolving
    house->data.house.evolve_text_id = 61;
    if (house->data.house.no_space_to_expand == 1) {
        // house would like to evolve but can't
        house->data.house.evolve_text_id = 64;
    }
}

int building_house_determine_worst_desirability_building(const struct building_t *house)
{
    int lowest_desirability = 0;
    int lowest_building_id = 0;
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(house->x, house->y, 1, 6, &x_min, &y_min, &x_max, &y_max);

    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int building_id = map_building_at(map_grid_offset(x, y));
            if (building_id <= 0) {
                continue;
            }
            struct building_t *b = &all_buildings[building_id];
            if (b->state != BUILDING_STATE_IN_USE || building_id == house->id) {
                continue;
            }
            if (!b->house_size || b->type < house->type) {
                int des = building_properties[b->type].desirability_value;
                if (des < 0) {
                    // simplified desirability calculation
                    int step_size = building_properties[b->type].desirability_step_size;
                    int range = building_properties[b->type].desirability_range;
                    int dist = calc_maximum_distance(x, y, house->x, house->y);
                    if (dist <= range) {
                        while (--dist > 1) {
                            des += step_size;
                        }
                        if (des < lowest_desirability) {
                            lowest_desirability = des;
                            lowest_building_id = building_id;
                        }
                    }
                }
            }
        }
    }
    return lowest_building_id;
}
