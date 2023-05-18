#include "system.h"

#include "core/dir.h"
#include "game/settings.h"
#include "sound/channel.h"
#include "sound/city.h"
#include "sound/device.h"
#include "sound/effect.h"
#include "sound/music.h"
#include "sound/speech.h"

#include <string.h>

static char channel_filenames[SOUND_CHANNEL_MAX][CHANNEL_FILENAME_MAX] = {
    "", // speech channel
    "wavs/panel1.wav",
    "wavs/panel3.wav",
    "wavs/icon1.wav",
    "wavs/build1.wav",
    "wavs/explod1.wav",
    "wavs/fanfare.wav",
    "wavs/fanfare2.wav",
    "wavs/arrow.wav",
    "wavs/arrow_hit.wav",
    "wavs/axe.wav",
    "wavs/ballista.wav",
    "wavs/ballista_hit_ground.wav",
    "wavs/ballista_hit_person.wav",
    "wavs/club.wav",
    "wavs/camel1.wav",
    "wavs/elephant.wav",
    "wavs/elephant_hit.wav",
    "wavs/elephant_die.wav",
    "wavs/horse.wav",
    "wavs/horse2.wav",
    "wavs/horse_mov.wav",
    "wavs/javelin.wav",
    "wavs/lion_attack.wav",
    "wavs/lion_die.wav",
    "wavs/horn3.wav",
    "wavs/sword.wav",
    "wavs/sword_swing.wav",
    "wavs/sword_light.wav",
    "wavs/spear_attack.wav",
    "wavs/wolf_attack.wav",
    "wavs/wolf_attack2.wav",
    "wavs/wolf_die.wav",
    "wavs/die1.wav",
    "wavs/die2.wav",
    "wavs/die4.wav",
    "wavs/die10.wav",
    "wavs/die3.wav",
    "wavs/die5.wav",
    "wavs/die8.wav",
    "wavs/die9.wav",
    "wavs/sheep_die.wav",
    "wavs/zebra_die.wav",
    "wavs/wolf_howl.wav",
    "wavs/fire_splash.wav",
    "wavs/formation_shield.wav",
    // city sounds
    "wavs/house_slum1.wav",
    "wavs/house_slum2.wav",
    "wavs/house_slum3.wav",
    "wavs/house_slum4.wav",
    "wavs/house_poor1.wav",
    "wavs/house_poor2.wav",
    "wavs/house_poor3.wav",
    "wavs/house_poor4.wav",
    "wavs/house_mid1.wav",
    "wavs/house_mid2.wav",
    "wavs/house_mid3.wav",
    "wavs/house_mid4.wav",
    "wavs/house_good1.wav",
    "wavs/house_good2.wav",
    "wavs/house_good3.wav",
    "wavs/house_good4.wav",
    "wavs/house_posh1.wav",
    "wavs/house_posh2.wav",
    "wavs/house_posh3.wav",
    "wavs/house_posh4.wav",
    "wavs/empty_land1.wav",
    "wavs/resevoir.wav",
    "wavs/aquaduct.wav", // same as river.wav
    "wavs/fountain.wav",
    "wavs/well.wav",
    "wavs/barber.wav",
    "wavs/baths.wav",
    "wavs/clinic.wav",
    "wavs/hospital.wav",
    "wavs/temp_farm.wav",
    "wavs/temp_ship.wav",
    "wavs/temple_ship.wav",
    "wavs/temp_comm.wav",
    "wavs/temp_war.wav",
    "wavs/temple_war.wav",
    "wavs/temp_love.wav",
    "wavs/oracle.wav",
    "wavs/school.wav",
    "wavs/academy.wav",
    "wavs/library.wav",
    "wavs/theatre.wav",
    "wavs/ampitheatre.wav",
    "wavs/colloseum.wav",
    "wavs/hippodrome.wav",
    "wavs/glad_pit.wav",
    "wavs/lion_pit.wav",
    "wavs/art_pit.wav",
    "wavs/char_pit.wav",
    "wavs/forum.wav",
    "wavs/senate.wav",
    "wavs/palace.wav",
    "wavs/statue.wav",
    "wavs/gardens1.wav", // same as emptyland1.wav
    "wavs/gardens2.wav", // same as emptyland2.wav
    "wavs/gardens3.wav", // same as emptyland3.wav
    "wavs/gardens4.wav", // same as emptyland4.wav
    "wavs/shipyard.wav",
    "wavs/shipyard1.wav",
    "wavs/shipyard2.wav",
    "wavs/dock.wav",
    "wavs/dock1.wav",
    "wavs/dock2.wav",
    "wavs/wharf.wav",
    "wavs/wharf1.wav",
    "wavs/wharf2.wav",
    "wavs/tower1.wav",
    "wavs/tower2.wav",
    "wavs/tower3.wav",
    "wavs/tower4.wav",
    "wavs/fort1.wav",
    "wavs/fort2.wav",
    "wavs/fort3.wav",
    "wavs/fort4.wav",
    "wavs/mil_acad.wav",
    "wavs/barracks.wav",
    "wavs/wheat.wav",
    "wavs/wheat_farm.wav",
    "wavs/veg_farm.wav",
    "wavs/figs_farm.wav",
    "wavs/olives_farm.wav",
    "wavs/vines_farm.wav",
    "wavs/meat_farm.wav",
    "wavs/clay_pit.wav",
    "wavs/quarry.wav",
    "wavs/mine.wav",
    "wavs/lumber_mill.wav",
    "wavs/wine_workshop.wav",
    "wavs/oil_workshop.wav",
    "wavs/weap_workshop.wav",
    "wavs/weapons_workshop.wav",
    "wavs/furn_workshop.wav",
    "wavs/furniture_workshop.wav",
    "wavs/pott_workshop.wav",
    "wavs/pottery_workshop.wav",
    "wavs/market1.wav",
    "wavs/market2.wav",
    "wavs/market3.wav",
    "wavs/market4.wav",
    "wavs/granary.wav",
    "wavs/granary1.wav",
    "wavs/granary2.wav",
    "wavs/warehouse.wav",
    "wavs/warehouse1.wav",
    "wavs/warehouse2.wav",
    "wavs/burning_ruin.wav",
};

void sound_system_init(void)
{
    sound_device_open();
    sound_device_init_channels(SOUND_CHANNEL_MAX, channel_filenames);

    sound_city_set_volume(setting_sound(SOUND_CITY)->volume);
    sound_effect_set_volume(setting_sound(SOUND_EFFECTS)->volume);
    sound_music_set_volume(setting_sound(SOUND_MUSIC)->volume);
    sound_speech_set_volume(setting_sound(SOUND_SPEECH)->volume);
}

void sound_system_shutdown(void)
{
    sound_device_close();
}
