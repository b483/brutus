#ifndef SCENARIO_EDITOR_EVENTS_H
#define SCENARIO_EDITOR_EVENTS_H

#include "core/buffer.h"

#define MAX_INVASION_WARNINGS 101

typedef enum {
    REQUEST_STATE_NORMAL = 0,
    REQUEST_STATE_OVERDUE = 1,
    REQUEST_STATE_DISPATCHED = 2,
    REQUEST_STATE_DISPATCHED_LATE = 3,
    REQUEST_STATE_IGNORED = 4,
    REQUEST_STATE_RECEIVED = 5
} scenario_request_state;

struct invasion_warning_t {
    int in_use;
    int handled;
    int invasion_path_id;
    int warning_years;
    int x;
    int y;
    int image_id;
    int empire_object_id;
    int year_notified;
    int month_notified;
    int months_to_go;
    int invasion_id;
};

extern struct invasion_warning_t invasion_warnings[MAX_INVASION_WARNINGS];

void scenario_empire_process_expansion(void);

int scenario_building_allowed(int building_type);

void scenario_gladiator_revolt_process(void);

void scenario_earthquake_init(void);

void scenario_earthquake_process(void);

int scenario_earthquake_is_in_progress(void);

void scenario_earthquake_save_state(buffer *buf);

void scenario_earthquake_load_state(buffer *buf);

void scenario_random_event_process(void);

void scenario_request_process(void);

void scenario_request_dispatch(int id);

void scenario_custom_messages_process(void);

void scenario_price_change_process(void);

void scenario_demand_change_process(void);

void scenario_invasion_clear(void);
void scenario_invasion_init(void);

int scenario_invasion_exists_upcoming(void);

void scenario_invasion_start_from_mars(void);

int scenario_invasion_start_from_caesar(int size);

void scenario_invasion_start_from_cheat(void);

void scenario_invasion_process(void);

void scenario_invasion_save_state(buffer *warnings);

void scenario_invasion_load_state(buffer *warnings);

void scenario_distant_battle_process(void);

#endif // SCENARIO_EDITOR_EVENTS_H
