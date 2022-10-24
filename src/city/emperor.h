#ifndef CITY_EMPEROR_H
#define CITY_EMPEROR_H

enum {
    GIFT_MODEST = 0,
    GIFT_GENEROUS = 1,
    GIFT_LAVISH = 2
};

typedef struct {
    int id;
    int cost;
} emperor_gift;

void city_emperor_init_scenario(void);

void city_emperor_update(void);

void city_emperor_send_gift(void);

int city_emperor_salary_for_rank(int rank);

void city_emperor_set_salary_rank(int player_rank);

int city_emperor_salary_amount(void);

int city_emperor_personal_savings(void);

void city_emperor_mark_soldier_killed(void);

#endif // CITY_EMPEROR_H
