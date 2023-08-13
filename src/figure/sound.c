#include "sound.h"

#include "city/data.h"
#include "sound/sound.h"

void figure_play_die_sound(const struct figure_t *f)
{
    int is_combatant = 0;
    int is_citizen = 0;
    switch (f->type) {
        case FIGURE_WOLF:
            play_sound_effect(SOUND_EFFECT_WOLF_DIE);
            break;
        case FIGURE_SHEEP:
            play_sound_effect(SOUND_EFFECT_SHEEP_DIE);
            break;
        case FIGURE_ZEBRA:
            play_sound_effect(SOUND_EFFECT_ZEBRA_DIE);
            break;
        case FIGURE_LION_TAMER:
            play_sound_effect(SOUND_EFFECT_LION_DIE);
            break;
        case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
            play_sound_effect(SOUND_EFFECT_ELEPHANT_DIE);
            break;
        case FIGURE_ENEMY_BRITON_CHARIOT:
        case FIGURE_ENEMY_CELT_CHARIOT:
        case FIGURE_ENEMY_PICT_CHARIOT:
        case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
        case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
        case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
            play_sound_effect(SOUND_EFFECT_HORSE2);
            break;
        case FIGURE_ENEMY_EGYPTIAN_CAMEL:
            play_sound_effect(SOUND_EFFECT_CAMEL);
            break;
        case FIGURE_NATIVE_TRADER:
        case FIGURE_TRADE_CARAVAN:
        case FIGURE_TRADE_CARAVAN_DONKEY:
            break;
        case FIGURE_PREFECT:
        case FIGURE_FORT_JAVELIN:
        case FIGURE_FORT_MOUNTED:
        case FIGURE_FORT_LEGIONARY:
        case FIGURE_GLADIATOR:
        case FIGURE_INDIGENOUS_NATIVE:
        case FIGURE_TOWER_SENTRY:
        case FIGURE_ENEMY_GLADIATOR:
        case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
        case FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN:
        case FIGURE_ENEMY_BRITON_SWORDSMAN:
        case FIGURE_ENEMY_CELT_SWORDSMAN:
        case FIGURE_ENEMY_PICT_SWORDSMAN:
        case FIGURE_ENEMY_EGYPTIAN_SWORDSMAN:
        case FIGURE_ENEMY_ETRUSCAN_SWORDSMAN:
        case FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER:
        case FIGURE_ENEMY_SAMNITE_SWORDSMAN:
        case FIGURE_ENEMY_SAMNITE_SPEAR_THROWER:
        case FIGURE_ENEMY_GAUL_SWORDSMAN:
        case FIGURE_ENEMY_GAUL_AXEMAN:
        case FIGURE_ENEMY_HELVETIUS_SWORDSMAN:
        case FIGURE_ENEMY_HELVETIUS_AXEMAN:
        case FIGURE_ENEMY_HUN_SWORDSMAN:
        case FIGURE_ENEMY_GOTH_SWORDSMAN:
        case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
        case FIGURE_ENEMY_GREEK_SWORDSMAN:
        case FIGURE_ENEMY_GREEK_SPEAR_THROWER:
        case FIGURE_ENEMY_MACEDONIAN_SWORDSMAN:
        case FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
        case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_PERGAMUM_SWORDSMAN:
        case FIGURE_ENEMY_PERGAMUM_ARCHER:
        case FIGURE_ENEMY_IBERIAN_SWORDSMAN:
        case FIGURE_ENEMY_IBERIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_JUDEAN_SWORDSMAN:
        case FIGURE_ENEMY_JUDEAN_SPEAR_THROWER:
        case FIGURE_ENEMY_SELEUCID_SWORDSMAN:
        case FIGURE_ENEMY_SELEUCID_SPEAR_THROWER:
        case FIGURE_ENEMY_CAESAR_JAVELIN:
        case FIGURE_ENEMY_CAESAR_MOUNTED:
        case FIGURE_ENEMY_CAESAR_LEGIONARY:
            is_combatant = 1;
            break;
        default:
            is_citizen = 1;
            break;
    }
    city_data.sound.die_citizen++;


    if (is_combatant) {
        city_data.sound.die_soldier++;
        if (city_data.sound.die_soldier >= 4) {
            city_data.sound.die_soldier = 0;
        }
        play_sound_effect(SOUND_EFFECT_SOLDIER_DIE + city_data.sound.die_soldier);
    } else if (is_citizen) {
        if (city_data.sound.die_citizen >= 4) {
            city_data.sound.die_citizen = 0;
        }
        play_sound_effect(SOUND_EFFECT_CITIZEN_DIE + city_data.sound.die_citizen);
    }
}

void figure_play_hit_sound(int type)
{
    switch (type) {
        case FIGURE_LION_TAMER:
            play_sound_effect(SOUND_EFFECT_LION_ATTACK);
            break;
        case FIGURE_WOLF:
            play_sound_effect(SOUND_EFFECT_WOLF_ATTACK);
            break;
        case FIGURE_FORT_LEGIONARY:
        case FIGURE_ENEMY_CAESAR_LEGIONARY:
            play_sound_effect(SOUND_EFFECT_SWORD);
            break;
        case FIGURE_FORT_JAVELIN:
        case FIGURE_FORT_MOUNTED:
        case FIGURE_ENEMY_GLADIATOR:
        case FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN:
        case FIGURE_ENEMY_ETRUSCAN_SWORDSMAN:
        case FIGURE_ENEMY_SAMNITE_SWORDSMAN:
        case FIGURE_ENEMY_GREEK_SWORDSMAN:
        case FIGURE_ENEMY_MACEDONIAN_SWORDSMAN:
        case FIGURE_ENEMY_BRITON_CHARIOT:
        case FIGURE_ENEMY_CELT_CHARIOT:
        case FIGURE_ENEMY_PICT_CHARIOT:
        case FIGURE_ENEMY_BRITON_SWORDSMAN:
        case FIGURE_ENEMY_CELT_SWORDSMAN:
        case FIGURE_ENEMY_PICT_SWORDSMAN:
        case FIGURE_ENEMY_GAUL_SWORDSMAN:
        case FIGURE_ENEMY_HELVETIUS_SWORDSMAN:
        case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
        case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
        case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
            play_sound_effect(SOUND_EFFECT_LIGHT_SWORD);
            break;
        case FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER:
        case FIGURE_ENEMY_SAMNITE_SPEAR_THROWER:
        case FIGURE_ENEMY_GREEK_SPEAR_THROWER:
        case FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_IBERIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_JUDEAN_SPEAR_THROWER:
        case FIGURE_ENEMY_SELEUCID_SPEAR_THROWER:
            play_sound_effect(SOUND_EFFECT_SPEAR);
            break;
        case FIGURE_ENEMY_EGYPTIAN_SWORDSMAN:
        case FIGURE_ENEMY_PERGAMUM_SWORDSMAN:
        case FIGURE_ENEMY_IBERIAN_SWORDSMAN:
        case FIGURE_ENEMY_JUDEAN_SWORDSMAN:
        case FIGURE_ENEMY_SELEUCID_SWORDSMAN:
        case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
        case FIGURE_ENEMY_HUN_SWORDSMAN:
        case FIGURE_ENEMY_GOTH_SWORDSMAN:
        case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
        case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
            play_sound_effect(SOUND_EFFECT_CLUB);
            break;
        case FIGURE_ENEMY_GAUL_AXEMAN:
        case FIGURE_ENEMY_HELVETIUS_AXEMAN:
            play_sound_effect(SOUND_EFFECT_AXE);
            break;
        case FIGURE_ENEMY_EGYPTIAN_CAMEL:
            play_sound_effect(SOUND_EFFECT_CAMEL);
            break;
        case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
            city_data.sound.hit_elephant = !city_data.sound.hit_elephant;
            if (city_data.sound.hit_elephant) {
                play_sound_effect(SOUND_EFFECT_ELEPHANT);
            } else {
                play_sound_effect(SOUND_EFFECT_ELEPHANT_HIT);
            }
            break;
        default:
            break;
    }
}
