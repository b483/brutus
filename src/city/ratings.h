#ifndef CITY_RATINGS_H
#define CITY_RATINGS_H

enum {
    SELECTED_RATING_NONE = 0,
    SELECTED_RATING_CULTURE = 1,
    SELECTED_RATING_PROSPERITY = 2,
    SELECTED_RATING_PEACE = 3,
    SELECTED_RATING_FAVOR = 4
};

int city_rating_selected_explanation(void);

void city_ratings_reduce_prosperity_after_bailout(void);

void city_ratings_peace_building_destroyed(int type);

void city_ratings_peace_record_criminal(void);

void city_ratings_peace_record_rioter(void);

void city_ratings_update_favor_explanation(void);

void city_ratings_update_explanations(void);

void city_ratings_update(int is_yearly_update);

#endif // CITY_RATINGS_H
