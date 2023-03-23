#include "sound.h"

#include "city/data_private.h"
#include "sound/effect.h"
#include "sound/speech.h"

void figure_play_die_sound(const figure *f)
{
    int is_combatant = 0;
    int is_citizen = 0;
    switch (f->type) {
        case FIGURE_WOLF:
            sound_effect_play(SOUND_EFFECT_WOLF_DIE);
            break;
        case FIGURE_SHEEP:
            sound_effect_play(SOUND_EFFECT_SHEEP_DIE);
            break;
        case FIGURE_ZEBRA:
            sound_effect_play(SOUND_EFFECT_ZEBRA_DIE);
            break;
        case FIGURE_LION_TAMER:
            sound_effect_play(SOUND_EFFECT_LION_DIE);
            break;
        case FIGURE_ENEMY_CHARIOT:
        case FIGURE_ENEMY_MOUNTED_ARCHER:
            sound_effect_play(SOUND_EFFECT_HORSE2);
            break;
        case FIGURE_ENEMY_CAMEL:
            sound_effect_play(SOUND_EFFECT_CAMEL);
            break;
        case FIGURE_ENEMY_ELEPHANT:
            sound_effect_play(SOUND_EFFECT_ELEPHANT_DIE);
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
        case FIGURE_ENEMY_RANGED_SPEAR_1:
        case FIGURE_ENEMY_SWORD_1:
        case FIGURE_ENEMY_SWORD_2:
        case FIGURE_ENEMY_FAST_SWORD:
        case FIGURE_ENEMY_SWORD_3:
        case FIGURE_ENEMY_RANGED_SPEAR_2:
        case FIGURE_ENEMY_AXE:
        case FIGURE_ENEMY_GLADIATOR:
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
        sound_effect_play(SOUND_EFFECT_SOLDIER_DIE + city_data.sound.die_soldier);
    } else if (is_citizen) {
        if (city_data.sound.die_citizen >= 4) {
            city_data.sound.die_citizen = 0;
        }
        sound_effect_play(SOUND_EFFECT_CITIZEN_DIE + city_data.sound.die_citizen);
    }
    if (f->is_enemy_unit) {
        if (city_data.figure.enemies == 1) {
            sound_speech_play_file("wavs/army_war_cry.wav");
        }
    } else if (f->is_player_legion_unit) {
        if (city_data.figure.soldiers == 1) {
            sound_speech_play_file("wavs/barbarian_war_cry.wav");
        }
    }
}

void figure_play_hit_sound(int type)
{
    switch (type) {
        case FIGURE_FORT_LEGIONARY:
        case FIGURE_ENEMY_CAESAR_LEGIONARY:
            sound_effect_play(SOUND_EFFECT_SWORD);
            break;
        case FIGURE_FORT_MOUNTED:
        case FIGURE_ENEMY_SWORD_2:
        case FIGURE_ENEMY_CHARIOT:
        case FIGURE_ENEMY_SWORD_3:
        case FIGURE_ENEMY_MOUNTED_ARCHER:
        case FIGURE_ENEMY_GLADIATOR:
            sound_effect_play(SOUND_EFFECT_SWORD_SWING);
            break;
        case FIGURE_FORT_JAVELIN:
            sound_effect_play(SOUND_EFFECT_LIGHT_SWORD);
            break;
        case FIGURE_ENEMY_RANGED_SPEAR_1:
        case FIGURE_ENEMY_RANGED_SPEAR_2:
            sound_effect_play(SOUND_EFFECT_SPEAR);
            break;
        case FIGURE_ENEMY_SWORD_1:
        case FIGURE_ENEMY_FAST_SWORD:
            sound_effect_play(SOUND_EFFECT_CLUB);
            break;
        case FIGURE_ENEMY_AXE:
            sound_effect_play(SOUND_EFFECT_AXE);
            break;
        case FIGURE_ENEMY_CAMEL:
            sound_effect_play(SOUND_EFFECT_CAMEL);
            break;
        case FIGURE_ENEMY_ELEPHANT:
            city_data.sound.hit_elephant = !city_data.sound.hit_elephant;
            if (city_data.sound.hit_elephant) {
                sound_effect_play(SOUND_EFFECT_ELEPHANT);
            } else {
                sound_effect_play(SOUND_EFFECT_ELEPHANT_HIT);
            }
            break;
        case FIGURE_LION_TAMER:
            sound_effect_play(SOUND_EFFECT_LION_ATTACK);
            break;
        case FIGURE_WOLF:
            sound_effect_play(SOUND_EFFECT_WOLF_ATTACK);
            break;
        default:
            break;
    }
}
