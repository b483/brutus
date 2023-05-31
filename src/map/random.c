#include "random.h"

#include "core/random.h"
#include "map/grid.h"

static struct grid_u8_t random;

void map_random_clear(void)
{
    map_grid_clear_u8(random.items);
}

void map_random_init(void)
{
    int grid_offset = 0;
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++, grid_offset++) {
            random_generate_next();
            random.items[grid_offset] = (uint8_t) random_short();
        }
    }
}

int map_random_get(int grid_offset)
{
    return random.items[grid_offset];
}

void map_random_save_state(struct buffer_t *buf)
{
    map_grid_save_state_u8(random.items, buf);
}

void map_random_load_state(struct buffer_t *buf)
{
    map_grid_load_state_u8(random.items, buf);
}
