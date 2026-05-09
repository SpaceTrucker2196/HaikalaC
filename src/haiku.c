/* Haiku table — 100 curated public-domain classics, ported 1:1 from
 * the Python upstream. Poets: Bashō (30), Buson (25), Issa (25),
 * Chiyo-ni (8), Shiki (5), Ryōkan (3), Onitsura (1), Kikaku (1),
 * Moritake (1), Sōkan (1).
 *
 * Each token array is in static storage so we can return stable
 * pointers from the spec builder. */

#include <stdio.h>
#include <string.h>

#include "haikala.h"

#define T(...) (const char *const[]){__VA_ARGS__}

/* Per-haiku token arrays. */
#define TOKS(name, ...) \
    static const char *const name[] = { __VA_ARGS__ }
#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* ===================== Bashō (30) ================================== */
TOKS(t_old_pond,            "pond","frog","splash","water","stillness");
TOKS(t_withered_branch,     "crow","branch","evening","stillness");
TOKS(t_summer_grass,        "grass","warriors","dream","memory");
TOKS(t_first_snow_bridge,   "snow","bridge","stillness","first");
TOKS(t_basho_journey_dreams,"dream","journey","field","withered","sickness");
TOKS(t_basho_cicada_rock,   "cicada","voice","stone","silence","summer");
TOKS(t_basho_lonely_road,   "road","traveler","evening","loneliness","autumn");
TOKS(t_basho_sea_duck_voice,"sea","duck","voice","dusk","white");
TOKS(t_basho_temple_bell_blossom,"bell","blossom","fragrance","lingering","evening");
TOKS(t_basho_octopus_pot,   "octopus","pot","dream","moon","summer");
TOKS(t_basho_wild_sea_stars,"sea","milky_way","star","island","wind");
TOKS(t_basho_cuckoo_clouds, "cuckoo","cloud","voice","vanishing","summer");
TOKS(t_basho_moon_treetops, "moon","tree","rain","stillness","night");
TOKS(t_basho_monkey_mask,   "monkey","mask","year","repetition","new_year");
TOKS(t_basho_winter_solitude,"winter","color","wind","loneliness","sound");
TOKS(t_basho_skylark_moor,  "skylark","song","field","freedom","spring");
TOKS(t_basho_spring_departs,"spring","sparrow","fish","tears","parting");
TOKS(t_basho_snowy_morning, "snow","morning","fish","loneliness","winter");
TOKS(t_basho_cherry_petals, "cherry","blossom","drift","wind","spring");
TOKS(t_basho_lightning,     "lightning","heron","voice","dark","summer");
TOKS(t_basho_plum_dawn,     "plum","sun","mountain","path","fragrance");
TOKS(t_basho_harvest_moon_gate,"moon","tide","gate","autumn","water");
TOKS(t_basho_windy_pine,    "pine","wind","sound","autumn","echo");
TOKS(t_basho_wisteria_evening,"wisteria","evening","white","fragrance","breeze");
TOKS(t_basho_spider_dew,    "spider","dew","morning","garden","stillness");
TOKS(t_basho_autumn_window, "wind","autumn","window","lantern","longing");
TOKS(t_basho_snow_crow,     "crow","snow","morning","silence","winter");
TOKS(t_basho_snowy_field_traveler,"snow","field","traveler","path","winter");
TOKS(t_basho_butterfly_dream,"butterfly","dream","drift","awakening","spring");
TOKS(t_basho_temple_bell_flowers,"bell","blossom","lingering","sound","temple");

/* ===================== Buson (25) ================================== */
TOKS(t_candle_flame,        "candle","flame","transfer","twilight");
TOKS(t_autumn_moon,         "moon","worm","chestnut","silence");
TOKS(t_buson_peony_cart,    "peony","cart","summer","sound","opening");
TOKS(t_buson_spring_rain_cat,"rain","cat","dance","woman","spring");
TOKS(t_buson_summer_river_horse,"river","horse","moon","summer","crossing");
TOKS(t_buson_heron_breeze,  "heron","breeze","water","evening","summer");
TOKS(t_buson_willow_pool,   "willow","pool","mud","stillness","spring");
TOKS(t_buson_spring_road_basket,"basket","road","rain","spring","mud");
TOKS(t_buson_wife_comb,     "comb","woman","regret","longing","autumn");
TOKS(t_buson_pawnshop_trees,"tree","village","withered","winter","loneliness");
TOKS(t_buson_morning_haze,  "mist","village","bridge","morning","stillness");
TOKS(t_buson_short_night_peony,"peony","night","summer","opening","blossom");
TOKS(t_buson_white_plum_dawn,"plum","dawn","color","spring","blossom");
TOKS(t_buson_old_well_fish, "well","fish","sound","dark","autumn");
TOKS(t_buson_moonlit_frog_clouds,"frog","moon","cloud","drift","summer");
TOKS(t_buson_blossoms_drift,"blossom","cloud","drift","parting","spring");
TOKS(t_buson_winter_stable, "stable","horse","breath","snow","winter");
TOKS(t_buson_butterfly_bell,"butterfly","bell","sleep","temple","stillness");
TOKS(t_buson_moonlit_wisteria,"wisteria","moon","scent","night","fragrance");
TOKS(t_buson_cherry_path_wind,"cherry","path","wind","breeze","spring");
TOKS(t_buson_summer_river_stars,"river","star","night","sound","summer");
TOKS(t_buson_spring_breeze_pipe,"pipe","grass","breeze","smoke","spring");
TOKS(t_buson_mustard_field_sea,"mustard","field","sea","evening","color");
TOKS(t_buson_thunder_plum,  "lightning","plum","blossom","spring","sudden");
TOKS(t_buson_boat_river_evening,"boat","lantern","river","evening","autumn");

/* ===================== Issa (25) =================================== */
TOKS(t_snow_melts,          "snow","village","flood","children","joy");
TOKS(t_firefly_drift,       "firefly","drift","night","passing");
TOKS(t_plum_path,           "plum","blossom","fragrance","path","messenger");
TOKS(t_issa_world_of_dew,   "dew","world","parting","gratitude","autumn");
TOKS(t_issa_snail_fuji,     "snail","mountain","climbing","patience","summer");
TOKS(t_issa_cherry_strange, "cherry","blossom","world","awakening","spring");
TOKS(t_issa_autumn_old_age, "autumn","year","longing","sparrow","cloud");
TOKS(t_issa_spider_kindness,"spider","hut","kindness","stillness","summer");
TOKS(t_issa_mosquito_monk,  "mosquito","prayer","monk","irony","summer");
TOKS(t_issa_cat_journey,    "cat","journey","stretching","awakening","autumn");
TOKS(t_issa_cuckoo_mountain,"cuckoo","mountain","voice","song","summer");
TOKS(t_issa_radish_road,    "radish","road","traveler","kindness","autumn");
TOKS(t_issa_flies_temple,   "fly","buddha","world","summer","irony");
TOKS(t_issa_kite_beggar,    "kite","beggar","hut","joy","spring");
TOKS(t_issa_moon_request,   "moon","child","longing","tears","autumn");
TOKS(t_issa_autumn_wind_child,"wind","autumn","child","sickness","frailty");
TOKS(t_issa_cricket_singing,"cricket","song","voice","night","autumn");
TOKS(t_issa_first_dream,    "dream","year","secret","smile","new_year");
TOKS(t_issa_spring_rain_yawn,"rain","woman","sleep","spring","awakening");
TOKS(t_issa_mosquito_journey,"mosquito","journey","joy","summer","irony");
TOKS(t_issa_moon_plum_passing,"moon","plum","passing","year","spring");
TOKS(t_issa_cicada_first,   "cicada","first","voice","summer","awakening");
TOKS(t_issa_tiny_frog_mountain,"frog","mountain","stillness","spring","awakening");
TOKS(t_issa_world_walk_hell,"blossom","world","hell","passing","spring");
TOKS(t_issa_spring_thaw_river,"river","thaw","spring","memory","current");

/* ===================== Chiyo-ni (8) ================================ */
TOKS(t_morning_glory,       "morning_glory","well","vine","water","kindness");
TOKS(t_chiyo_winter_parting,"winter","parting","woman","road","gratitude");
TOKS(t_chiyo_moon_farewell, "moon","gratitude","world","parting","autumn");
TOKS(t_chiyo_dragonfly_hunter,"dragonfly","child","journey","longing","summer");
TOKS(t_chiyo_women_field,   "woman","field","rice","spring","song");
TOKS(t_chiyo_butterflies_stones,"butterfly","stone","path","drift","spring");
TOKS(t_chiyo_willow_woman,  "willow","longing","wind","spring","parting");
TOKS(t_chiyo_new_year_face, "mirror","face","year","joy","new_year");

/* ===================== Other classical (12) ======================== */
TOKS(t_shiki_persimmon,     "persimmon","bell","sound","evening","autumn");
TOKS(t_shiki_softly,        "fly","sleep","kindness","longing","summer");
TOKS(t_shiki_spider_lone,   "spider","loneliness","night","regret","autumn");
TOKS(t_shiki_horse_river,   "horse","river","bridge","water","summer");
TOKS(t_shiki_snowy_village, "village","snow","mountain","water","sound");
TOKS(t_ryokan_thief_moon,   "moon","thief","window","kindness","autumn");
TOKS(t_ryokan_leaves_boat,  "leaf","boat","withered","gratitude","autumn");
TOKS(t_ryokan_no_talent,    "monk","year","patience","loneliness","winter");
TOKS(t_onitsura_trout_clouds,"fish","cloud","river","current","summer");
TOKS(t_kikaku_firefly_wind, "firefly","wind","vanishing","night","summer");
TOKS(t_moritake_butterfly_branch,"butterfly","blossom","branch","return","spring");
TOKS(t_sokan_moon_fan,      "moon","fan","joy","summer","freedom");

/* ---- the table ----------------------------------------------------- */

#define H(_id, _l1, _l2, _l3, _author, _season, _toks) \
    { .id=_id, .line1=_l1, .line2=_l2, .line3=_l3, \
      .author=_author, .season=_season, \
      .tokens=_toks, .n_tokens=COUNT(_toks) }

const hk_haiku hk_haiku_table[] = {
    /* Bashō */
    H("old_pond", "old pond—", "a frog leaps in", "the sound of water",
      "Matsuo Bashō", "spring", t_old_pond),
    H("withered_branch", "on a withered branch", "a crow has settled—",
      "autumn nightfall", "Matsuo Bashō", "autumn", t_withered_branch),
    H("summer_grass", "summer grasses—", "all that remains",
      "of warriors' dreams", "Matsuo Bashō", "summer", t_summer_grass),
    H("first_snow_bridge", "first snow", "falling on the half-finished",
      "bridge", "Matsuo Bashō", "winter", t_first_snow_bridge),
    H("basho_journey_dreams", "sick on a journey", "my dreams wander",
      "the withered fields", "Matsuo Bashō", "winter", t_basho_journey_dreams),
    H("basho_cicada_rock", "such stillness—", "the cicada's voice",
      "soaks into the rocks", "Matsuo Bashō", "summer", t_basho_cicada_rock),
    H("basho_lonely_road", "this road—", "no one travels it",
      "autumn evening", "Matsuo Bashō", "autumn", t_basho_lonely_road),
    H("basho_sea_duck_voice", "the sea darkens—", "a wild duck's call",
      "faintly white", "Matsuo Bashō", "winter", t_basho_sea_duck_voice),
    H("basho_temple_bell_blossom", "the bell fades",
      "the scent of blossoms ringing—", "evening",
      "Matsuo Bashō", "spring", t_basho_temple_bell_blossom),
    H("basho_octopus_pot", "octopus traps—", "fleeting dreams",
      "under the summer moon", "Matsuo Bashō", "summer", t_basho_octopus_pot),
    H("basho_wild_sea_stars", "a wild sea—", "and stretched across to Sado",
      "the river of stars", "Matsuo Bashō", "autumn", t_basho_wild_sea_stars),
    H("basho_cuckoo_clouds", "a cuckoo cries—", "and through the clouds",
      "vanishes", "Matsuo Bashō", "summer", t_basho_cuckoo_clouds),
    H("basho_moon_treetops", "the moon swift overhead—", "treetops still",
      "wet with rain", "Matsuo Bashō", "autumn", t_basho_moon_treetops),
    H("basho_monkey_mask", "year after year", "on the monkey's face",
      "a monkey's mask", "Matsuo Bashō", "new_year", t_basho_monkey_mask),
    H("basho_winter_solitude", "winter solitude—", "in a world of one color",
      "the sound of wind", "Matsuo Bashō", "winter", t_basho_winter_solitude),
    H("basho_skylark_moor", "a skylark singing—", "all day long",
      "and never enough", "Matsuo Bashō", "spring", t_basho_skylark_moor),
    H("basho_spring_departs", "spring departing—",
      "birds cry, and in the eyes", "of fishes are tears",
      "Matsuo Bashō", "spring", t_basho_spring_departs),
    H("basho_snowy_morning", "a snowy morning—", "by myself",
      "chewing on dried salmon", "Matsuo Bashō", "winter",
      t_basho_snowy_morning),
    H("basho_cherry_petals", "from all four sides",
      "cherry petals blowing in—", "Lake Biwa",
      "Matsuo Bashō", "spring", t_basho_cherry_petals),
    H("basho_lightning", "lightning flash—", "into the darkness",
      "a night-heron's cry", "Matsuo Bashō", "summer", t_basho_lightning),
    H("basho_plum_dawn", "the scent of plum—", "and the sun rises",
      "over the mountain path", "Matsuo Bashō", "spring", t_basho_plum_dawn),
    H("basho_harvest_moon_gate", "the harvest moon—", "the tide rises",
      "to my gate", "Matsuo Bashō", "autumn", t_basho_harvest_moon_gate),
    H("basho_windy_pine", "a wind in the pines—", "and somewhere, far off",
      "an axe falling", "Matsuo Bashō", "autumn", t_basho_windy_pine),
    H("basho_wisteria_evening", "wisteria flowers—", "in the evening light",
      "the crane's white", "Matsuo Bashō", "spring",
      t_basho_wisteria_evening),
    H("basho_spider_dew", "a spider in the doorway—", "morning dew",
      "trembles", "Matsuo Bashō", "autumn", t_basho_spider_dew),
    H("basho_autumn_window", "autumn wind—", "through an open window",
      "the lamp wavers", "Matsuo Bashō", "autumn", t_basho_autumn_window),
    H("basho_snow_crow", "morning snow—", "a single crow",
      "without a sound", "Matsuo Bashō", "winter", t_basho_snow_crow),
    H("basho_snowy_field_traveler", "snowy field—",
      "a single traveler's prints", "and then nothing",
      "Matsuo Bashō", "winter", t_basho_snowy_field_traveler),
    H("basho_butterfly_dream", "am I a butterfly",
      "dreaming I am Bashō—", "spring afternoon",
      "Matsuo Bashō", "spring", t_basho_butterfly_dream),
    H("basho_temple_bell_flowers", "the temple bell stops—",
      "but the sound keeps coming", "out of the flowers",
      "Matsuo Bashō", "spring", t_basho_temple_bell_flowers),

    /* Buson */
    H("candle_flame", "the light of a candle", "is transferred to another—",
      "spring twilight", "Yosa Buson", "spring", t_candle_flame),
    H("autumn_moon", "autumn moon—", "a worm digs silently",
      "into a chestnut", "Yosa Buson", "autumn", t_autumn_moon),
    H("buson_peony_cart", "a heavy cart rumbles by—", "and the peonies",
      "tremble", "Yosa Buson", "summer", t_buson_peony_cart),
    H("buson_spring_rain_cat", "spring rain—", "a girl is teaching",
      "the cat to dance", "Yosa Buson", "spring", t_buson_spring_rain_cat),
    H("buson_summer_river_horse", "a summer river—", "horses crossing one by one,",
      "each with a moon", "Yosa Buson", "summer", t_buson_summer_river_horse),
    H("buson_heron_breeze", "evening breeze—", "water laps",
      "against the heron's legs", "Yosa Buson", "summer",
      t_buson_heron_breeze),
    H("buson_willow_pool", "the willow tree", "has thinned out",
      "the muddy pool", "Yosa Buson", "spring", t_buson_willow_pool),
    H("buson_spring_road_basket", "along the muddy road",
      "in a passing basket—", "young cockles",
      "Yosa Buson", "spring", t_buson_spring_road_basket),
    H("buson_wife_comb", "a sudden chill—", "in our room, my dead wife's comb",
      "under my heel", "Yosa Buson", "autumn", t_buson_wife_comb),
    H("buson_pawnshop_trees", "two villages", "one pawnshop—",
      "leafless trees", "Yosa Buson", "winter", t_buson_pawnshop_trees),
    H("buson_morning_haze", "morning haze—", "the village still asleep,",
      "and a bridge", "Yosa Buson", "spring", t_buson_morning_haze),
    H("buson_short_night_peony", "the short night—",
      "by the riverbank a peony", "blooms wide",
      "Yosa Buson", "summer", t_buson_short_night_peony),
    H("buson_white_plum_dawn", "white plum blossoms—", "the dawn approaches",
      "yellow", "Yosa Buson", "spring", t_buson_white_plum_dawn),
    H("buson_old_well_fish", "an old well—", "a fish leaps",
      "to a dark sound", "Yosa Buson", "autumn", t_buson_old_well_fish),
    H("buson_moonlit_frog_clouds", "moonlit night—", "a frog swims",
      "through the clouds", "Yosa Buson", "summer",
      t_buson_moonlit_frog_clouds),
    H("buson_blossoms_drift", "blossoms fallen—", "pieces of cloud",
      "drift away", "Yosa Buson", "spring", t_buson_blossoms_drift),
    H("buson_winter_stable", "snow on snow—", "in the dark stable",
      "the horses' breath", "Yosa Buson", "winter", t_buson_winter_stable),
    H("buson_butterfly_bell", "a butterfly perches", "on the temple bell—",
      "sleeping", "Yosa Buson", "spring", t_buson_butterfly_bell),
    H("buson_moonlit_wisteria", "in pale moonlight", "the wisteria's scent",
      "comes from far away", "Yosa Buson", "spring",
      t_buson_moonlit_wisteria),
    H("buson_cherry_path_wind", "a narrow path—",
      "cherry trees on both sides,", "the wind",
      "Yosa Buson", "spring", t_buson_cherry_path_wind),
    H("buson_summer_river_stars", "summer night—", "footsteps cross the river,",
      "starlight", "Yosa Buson", "summer", t_buson_summer_river_stars),
    H("buson_spring_breeze_pipe", "spring breeze—", "on the grasses",
      "a pipe smolders", "Yosa Buson", "spring", t_buson_spring_breeze_pipe),
    H("buson_mustard_field_sea", "a field of mustard flowers—",
      "no whales in sight,", "the sea darkens",
      "Yosa Buson", "spring", t_buson_mustard_field_sea),
    H("buson_thunder_plum", "a sudden thunderclap—", "and the plum blossoms",
      "bow", "Yosa Buson", "spring", t_buson_thunder_plum),
    H("buson_boat_river_evening", "an evening boat—",
      "lanterns lit, and far ahead", "another lamp",
      "Yosa Buson", "autumn", t_buson_boat_river_evening),

    /* Issa */
    H("snow_melts", "the snow is melting",
      "and the village is flooded", "with children",
      "Kobayashi Issa", "spring", t_snow_melts),
    H("firefly_drift", "a giant firefly:", "that way, this way, that way—",
      "and it passes by", "Kobayashi Issa", "summer", t_firefly_drift),
    H("plum_path", "plum blossoms—", "their fragrance the path",
      "for a messenger", "Kobayashi Issa", "spring", t_plum_path),
    H("issa_world_of_dew", "the world of dew", "is the world of dew—",
      "and yet, and yet", "Kobayashi Issa", "autumn", t_issa_world_of_dew),
    H("issa_snail_fuji", "o snail—", "climb Mount Fuji,",
      "but slowly, slowly", "Kobayashi Issa", "summer", t_issa_snail_fuji),
    H("issa_cherry_strange", "what a strange thing—", "to be alive",
      "beneath cherry blossoms",
      "Kobayashi Issa", "spring", t_issa_cherry_strange),
    H("issa_autumn_old_age", "this autumn—", "how old I feel,",
      "a bird in the clouds",
      "Kobayashi Issa", "autumn", t_issa_autumn_old_age),
    H("issa_spider_kindness", "don't worry, spiders—", "I keep house",
      "casually", "Kobayashi Issa", "summer", t_issa_spider_kindness),
    H("issa_mosquito_monk", "all the time I pray to Buddha—", "I keep on",
      "killing mosquitoes",
      "Kobayashi Issa", "summer", t_issa_mosquito_monk),
    H("issa_cat_journey", "the cat,", "having stretched—",
      "picks up his journey",
      "Kobayashi Issa", "autumn", t_issa_cat_journey),
    H("issa_cuckoo_mountain", "a cuckoo sings—", "to me, to the mountain,",
      "to me, to the mountain",
      "Kobayashi Issa", "summer", t_issa_cuckoo_mountain),
    H("issa_radish_road", "the man pulling radishes—", "pointed the way",
      "with a radish", "Kobayashi Issa", "autumn", t_issa_radish_road),
    H("issa_flies_temple", "where there are humans", "there are flies—",
      "and Buddhas", "Kobayashi Issa", "summer", t_issa_flies_temple),
    H("issa_kite_beggar", "a beautiful kite", "rose up",
      "from a beggar's hut", "Kobayashi Issa", "spring", t_issa_kite_beggar),
    H("issa_moon_request", "a child cries—", "asks for the moon",
      "tonight", "Kobayashi Issa", "autumn", t_issa_moon_request),
    H("issa_autumn_wind_child", "the autumn wind—", "blows the curls",
      "of a sick child",
      "Kobayashi Issa", "autumn", t_issa_autumn_wind_child),
    H("issa_cricket_singing", "a cricket sings—", "and the night",
      "is taller for it",
      "Kobayashi Issa", "autumn", t_issa_cricket_singing),
    H("issa_first_dream", "first dream of the year—", "I keep it secret,",
      "smiling", "Kobayashi Issa", "new_year", t_issa_first_dream),
    H("issa_spring_rain_yawn", "spring rain—", "a pretty girl,",
      "yawning", "Kobayashi Issa", "spring", t_issa_spring_rain_yawn),
    H("issa_mosquito_journey", "what good luck—", "even the mosquitoes",
      "find me here",
      "Kobayashi Issa", "summer", t_issa_mosquito_journey),
    H("issa_moon_plum_passing", "moon, plum blossoms—", "this, that,",
      "the day passes",
      "Kobayashi Issa", "spring", t_issa_moon_plum_passing),
    H("issa_cicada_first", "the first cicada—", "life is",
      "cruel, cruel, cruel",
      "Kobayashi Issa", "summer", t_issa_cicada_first),
    H("issa_tiny_frog_mountain", "with bland serenity",
      "gazing at the far mountains—", "a tiny frog",
      "Kobayashi Issa", "spring", t_issa_tiny_frog_mountain),
    H("issa_world_walk_hell", "in this world",
      "we walk on the roof of hell—", "gazing at flowers",
      "Kobayashi Issa", "spring", t_issa_world_walk_hell),
    H("issa_spring_thaw_river", "the spring thaw—", "an old river",
      "remembers itself",
      "Kobayashi Issa", "spring", t_issa_spring_thaw_river),

    /* Chiyo-ni */
    H("morning_glory", "morning glory—", "the well bucket entangled,",
      "I ask for water",
      "Fukuda Chiyo-ni", "summer", t_morning_glory),
    H("chiyo_winter_parting", "after a long winter—",
      "giving each other nothing,", "we part",
      "Fukuda Chiyo-ni", "winter", t_chiyo_winter_parting),
    H("chiyo_moon_farewell", "having seen the moon—", "I bid farewell",
      "with a grateful heart",
      "Fukuda Chiyo-ni", "autumn", t_chiyo_moon_farewell),
    H("chiyo_dragonfly_hunter", "the dragonfly hunter—", "today, how far",
      "has he wandered?",
      "Fukuda Chiyo-ni", "summer", t_chiyo_dragonfly_hunter),
    H("chiyo_women_field", "again the women", "come out to the field—",
      "young rice",
      "Fukuda Chiyo-ni", "spring", t_chiyo_women_field),
    H("chiyo_butterflies_stones", "all entangled—", "butterfly to butterfly,",
      "on the stone path",
      "Fukuda Chiyo-ni", "spring", t_chiyo_butterflies_stones),
    H("chiyo_willow_woman", "to the willow—",
      "all the longing of my heart,", "and the wind",
      "Fukuda Chiyo-ni", "spring", t_chiyo_willow_woman),
    H("chiyo_new_year_face", "even my plain face", "is worth seeing—",
      "first day of the year",
      "Fukuda Chiyo-ni", "new_year", t_chiyo_new_year_face),

    /* Other classical */
    H("shiki_persimmon", "I bite into a persimmon—", "a temple bell sounds",
      "from far away", "Masaoka Shiki", "autumn", t_shiki_persimmon),
    H("shiki_softly", "I want to sleep—", "swat the flies",
      "softly, please", "Masaoka Shiki", "summer", t_shiki_softly),
    H("shiki_spider_lone", "after killing a spider—", "how lonely I feel",
      "in the cold of night",
      "Masaoka Shiki", "autumn", t_shiki_spider_lone),
    H("shiki_horse_river", "a summer river—", "there's a bridge,",
      "but my horse goes through the water",
      "Masaoka Shiki", "summer", t_shiki_horse_river),
    H("shiki_snowy_village", "a mountain village—",
      "under the piled-up snow,", "the sound of water",
      "Masaoka Shiki", "winter", t_shiki_snowy_village),
    H("ryokan_thief_moon", "the thief left it behind—", "the moon",
      "at my window", "Ryōkan Taigu", "autumn", t_ryokan_thief_moon),
    H("ryokan_leaves_boat", "if I gathered them all—", "the fallen leaves",
      "would fill the boat", "Ryōkan Taigu", "autumn", t_ryokan_leaves_boat),
    H("ryokan_no_talent", "no talent—", "I just keep alive",
      "through the seasons", "Ryōkan Taigu", "winter", t_ryokan_no_talent),
    H("onitsura_trout_clouds", "the trout leap up—", "and clouds",
      "move on the bottom of the brook",
      "Uejima Onitsura", "summer", t_onitsura_trout_clouds),
    H("kikaku_firefly_wind", "the first firefly—", "has flown away,",
      "the wind", "Takarai Kikaku", "summer", t_kikaku_firefly_wind),
    H("moritake_butterfly_branch", "a fallen petal—",
      "returning to the branch?", "no, a butterfly",
      "Arakida Moritake", "spring", t_moritake_butterfly_branch),
    H("sokan_moon_fan", "if only we could",
      "add a handle to the moon—", "what a fan!",
      "Yamazaki Sōkan", "summer", t_sokan_moon_fan),
};

const size_t hk_haiku_count = sizeof(hk_haiku_table) / sizeof(hk_haiku_table[0]);

const hk_haiku *hk_haiku_get(const char *id)
{
    if (id == NULL) return NULL;
    for (size_t i = 0; i < hk_haiku_count; ++i) {
        if (strcmp(hk_haiku_table[i].id, id) == 0) {
            return &hk_haiku_table[i];
        }
    }
    return NULL;
}
