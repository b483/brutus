#ifndef CITY_EMPEROR_H
#define CITY_EMPEROR_H

enum {
    GIFT_MODEST = 0,
    GIFT_GENEROUS = 1,
    GIFT_LAVISH = 2
};

struct emperor_gift_t {
    int id;
    int cost;
};

void city_emperor_init_scenario(void);

void update_debt_state(void);

void process_caesar_invasion(void);

void city_emperor_send_gift(void);

int city_emperor_salary_for_rank(int rank);

void city_emperor_set_salary_rank(int player_rank);

#endif // CITY_EMPEROR_H
