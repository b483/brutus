#ifndef CITY_FINANCE_H
#define CITY_FINANCE_H

int city_finance_can_afford(int building_cost);

void city_finance_process_export(int price);

void city_finance_process_donation(int amount);

void city_finance_process_sundry(int cost);

void city_finance_process_construction(int cost);

void city_finance_calculate_totals(void);

void city_finance_estimate_wages(void);

void city_finance_estimate_taxes(void);

void city_finance_handle_month_change(void);

void city_finance_handle_year_change(void);

typedef struct {
    struct {
        int taxes;
        int exports;
        int donated;
        int total;
    } income;
    struct {
        int imports;
        int wages;
        int construction;
        int interest;
        int salary;
        int sundries;
        int tribute;
        int total;
    } expenses;
    int net_in_out;
    int balance;
} finance_overview;

void distribute_treasury(void);

#endif // CITY_FINANCE_H
