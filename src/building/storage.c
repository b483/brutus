#include "storage.h"

#include "building/building.h"

#include <string.h>

#define MAX_STORAGES 200

struct data_storage {
    int in_use;
    int building_id;
    struct building_storage_t storage;
};

static struct {
    struct data_storage storages[MAX_STORAGES];
} data;

void building_storage_clear_all(void)
{
    memset(data.storages, 0, MAX_STORAGES * sizeof(struct data_storage));
}

void building_storage_reset_building_ids(void)
{
    for (int i = 1; i < MAX_STORAGES; i++) {
        data.storages[i].building_id = 0;
    }

    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_UNUSED) {
            continue;
        }
        if (b->type == BUILDING_GRANARY || b->type == BUILDING_WAREHOUSE) {
            if (b->storage_id) {
                if (data.storages[b->storage_id].building_id) {
                    // storage is already connected to a building: corrupt, create new
                    b->storage_id = building_storage_create();
                } else {
                    data.storages[b->storage_id].building_id = i;
                }
            }
        }
    }
}

int building_storage_create(void)
{
    for (int i = 1; i < MAX_STORAGES; i++) {
        if (!data.storages[i].in_use) {
            memset(&data.storages[i], 0, sizeof(struct data_storage));
            data.storages[i].in_use = 1;
            return i;
        }
    }
    return 0;
}

int building_storage_restore(int storage_id) 
{
    if (data.storages[storage_id].in_use) {
        return 0;
    }
    data.storages[storage_id].in_use = 1;
    return storage_id;
}

void building_storage_delete(int storage_id)
{
    data.storages[storage_id].in_use = 0;
}

struct building_storage_t *building_storage_get(int storage_id)
{
    return &data.storages[storage_id].storage;
}

void building_storage_toggle_empty_all(int storage_id)
{
    data.storages[storage_id].storage.empty_all = 1 - data.storages[storage_id].storage.empty_all;
}

void building_storage_cycle_resource_state(int storage_id, int resource_id)
{
    int state = data.storages[storage_id].storage.resource_state[resource_id];
    if (state == BUILDING_STORAGE_STATE_ACCEPTING) {
        state = BUILDING_STORAGE_STATE_NOT_ACCEPTING;
    } else if (state == BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
        state = BUILDING_STORAGE_STATE_GETTING;
    } else if (state == BUILDING_STORAGE_STATE_GETTING) {
        state = BUILDING_STORAGE_STATE_ACCEPTING;
    }
    data.storages[storage_id].storage.resource_state[resource_id] = state;
}

void building_storage_accept_none(int storage_id)
{
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        data.storages[storage_id].storage.resource_state[r] = BUILDING_STORAGE_STATE_NOT_ACCEPTING;
    }
}

void building_storage_save_state(struct buffer_t *buf)
{
    for (int i = 0; i < MAX_STORAGES; i++) {
        buffer_write_i32(buf, data.storages[i].building_id);
        buffer_write_u8(buf, (uint8_t) data.storages[i].in_use);
        buffer_write_u8(buf, (uint8_t) data.storages[i].storage.empty_all);
        for (int r = 0; r < RESOURCE_TYPES_MAX; r++) {
            buffer_write_u8(buf, data.storages[i].storage.resource_state[r]);
        }
    }
}

void building_storage_load_state(struct buffer_t *buf)
{
    for (int i = 0; i < MAX_STORAGES; i++) {
        data.storages[i].building_id = buffer_read_i32(buf);
        data.storages[i].in_use = buffer_read_u8(buf);
        data.storages[i].storage.empty_all = buffer_read_u8(buf);
        for (int r = 0; r < RESOURCE_TYPES_MAX; r++) {
            data.storages[i].storage.resource_state[r] = buffer_read_u8(buf);
        }
    }
}
