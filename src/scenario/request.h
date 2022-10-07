#ifndef SCENARIO_REQUEST_H
#define SCENARIO_REQUEST_H

typedef enum {
    REQUEST_STATE_NORMAL= 0,
    REQUEST_STATE_OVERDUE = 1,
    REQUEST_STATE_DISPATCHED = 2,
    REQUEST_STATE_DISPATCHED_LATE = 3,
    REQUEST_STATE_IGNORED = 4,
    REQUEST_STATE_RECEIVED = 5
} scenario_request_state;

void scenario_request_process(void);

void scenario_request_dispatch(int id);

#endif // SCENARIO_REQUEST_H
