/* Token → glyph vocabulary, ported 1:1 from the Python upstream.
 *
 * Three glyph layers, deliberately mixed:
 *   - emoji (🐸 🦋 🐌): bright, semantic, the "subjects" of the poem
 *   - dingbat / geometric (☾ ✦ ⬢ ❀): alchemical / floral essences
 *   - box-drawing & dot characters (─ ┄ · ˙): negative-space tissue
 *
 * Token order in each entry matches the upstream — the first glyph is
 * the "strongest" form (often emoji); later glyphs decay into quieter
 * symbols. The mandala's bindu uses index [0]; in --no-emoji mode the
 * filter walks the tuple and picks the first non-emoji entry.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "haikala.h"

typedef struct {
    const char        *token;
    const char *const *glyphs;
    size_t             n_glyphs;
} symbol_entry;

#define G_(name, ...) static const char *const name[] = { __VA_ARGS__ }

/* --- water ----------------------------------------------------------- */
G_(g_pond,         "💧", "◯", "○", "～");
G_(g_water,        "💧", "～", "≈", "∽");
G_(g_splash,       "💦", "✦", "✧", "·");
G_(g_well,         "💧", "◉", "◎", "○");
G_(g_flood,        "🌊", "～", "≈", "∿");
G_(g_sea,          "🌊", "≈", "～", "∿");
G_(g_ocean,        "🌊", "≈", "～", "∿");
G_(g_river,        "🌊", "～", "∽", "∿");
G_(g_tide,         "🌊", "～", "∿", "≈");
G_(g_ice,          "🧊", "◇", "·", "˙");
G_(g_dew,          "💧", "·", "˙", "◦");
G_(g_frost,        "🧊", "·", "˙", "❅");
G_(g_rain,         "🌧", "·", "│", "ı");
G_(g_mist,         "🌫", "·", "˙", "˜");
G_(g_cloud,        "☁\xef\xb8\x8f", "◌", "○", "·");   /* ☁ + VS16 */
G_(g_pool,         "💧", "◯", "○", "·");
G_(g_shore,        "🌊", "─", "·", "˙");
G_(g_thaw,         "💧", "·", "～", "˙");
G_(g_ford,         "🌊", "～", "·", "˙");

/* --- creatures ------------------------------------------------------- */
G_(g_frog,         "🐸", "🪷", "✿", "·");
G_(g_crow,         "🦅", "🐦", "✦", "⬢");
G_(g_worm,         "🐛", "～", "˜", "·");
G_(g_firefly,      "🌟", "✺", "✦", "˚");
G_(g_cicada,       "🦗", "⊛", "·", "˙");
G_(g_cuckoo,       "🐦", "∽", "·", "˙");
G_(g_dragonfly,    "🐝", "✺", "✦", "·");
G_(g_butterfly,    "🦋", "🌸", "✤", "·");
G_(g_cricket,      "🦗", "⊛", "·", "˙");
G_(g_sparrow,      "🐦", "✦", "·", "˙");
G_(g_goose,        "🪿", "∨", "·", "˙");
G_(g_duck,         "🦆", "⌒", "·", "˙");
G_(g_heron,        "🪶", "│", "·", "˙");
G_(g_snail,        "🐌", "◉", "○", "·");
G_(g_fish,         "🐟", "🐠", "≈", "˜");
G_(g_octopus,      "🐙", "✱", "·", "˙");
G_(g_mosquito,     "🦟", "·", "˙", "·");
G_(g_spider,       "🕷", "🕸", "·", "˙");
G_(g_deer,         "🦌", "⌬", "·", "˙");
G_(g_horse,        "🐎", "⊏", "·", "˙");
G_(g_cat,          "🐈", "🐾", "◑", "·");
G_(g_skylark,      "🐦", "✦", "·", "˙");
G_(g_monkey,       "🐒", "◔", "·", "˙");
G_(g_fly,          "🪰", "·", "˙", "·");

/* --- plants ---------------------------------------------------------- */
G_(g_branch,       "🌿", "┄", "─", "╴");
G_(g_grass,        "🌾", "│", "┊", "·");
G_(g_morning_glory,"🌺", "✿", "❀", "❁");
G_(g_plum,         "🌸", "✿", "❁", "❀");
G_(g_blossom,      "🌸", "🌼", "✿", "❀");
G_(g_petal,        "🌸", "✿", "·", "˙");
G_(g_vine,         "🌿", "∿", "～", "·");
G_(g_chestnut,     "🌰", "●", "◉", "•");
G_(g_cherry,       "🌸", "🍒", "✿", "❀");
G_(g_willow,       "🌿", "┊", "│", "·");
G_(g_peony,        "🌺", "❀", "✿", "·");
G_(g_wisteria,     "🌸", "❁", "·", "˙");
G_(g_leaf,         "🍃", "🍂", "⌒", "·");
G_(g_persimmon,    "🍑", "●", "◉", "·");
G_(g_lotus,        "🪷", "❀", "○", "·");
G_(g_bamboo,       "🎋", "│", "┊", "·");
G_(g_pine,         "🌲", "⋆", "·", "˙");
G_(g_iris,         "🌷", "❁", "·", "˙");
G_(g_chrysanthemum,"🌼", "✿", "❀", "·");
G_(g_mustard,      "🌼", "·", "˙", "✿");
G_(g_radish,       "🌱", "│", "·", "˙");
G_(g_tree,         "🌳", "│", "┊", "·");
G_(g_rose,         "🌹", "❀", "·", "˙");

/* --- weather / sky --------------------------------------------------- */
G_(g_snow,         "🌨", "❄\xef\xb8\x8f", "❅", "·");
G_(g_moon,         "🌙", "🌕", "☾", "○");
G_(g_star,         "⭐\xef\xb8\x8f", "🌟", "✦", "✧");
G_(g_milky_way,    "🌌", "✨", "·", "˙");
G_(g_sun,          "☀\xef\xb8\x8f", "🌞", "○", "·");
G_(g_dawn,         "🌅", "○", "◌", "·");
G_(g_dusk,         "🌆", "·", "˙", "◦");
G_(g_evening,      "🌆", "·", "˙", "◦");
G_(g_twilight,     "🌆", "·", "˙", "◦");
G_(g_night,        "🌙", "·", "˙", "✦");
G_(g_morning,      "🌅", "○", "◌", "·");
G_(g_horizon,      "🌅", "─", "━", "┄");
G_(g_mountain,     "🏔", "⛰\xef\xb8\x8f", "▲", "△");
G_(g_wind,         "🌬", "∽", "～", "·");
G_(g_breeze,       "🌬", "∽", "·", "˙");
G_(g_lightning,    "⚡\xef\xb8\x8f", "🌩", "⌇", "✦");
G_(g_color,        "🌈", "·", "˙", "◦");
G_(g_white,        " ", "·", "˙");
G_(g_dark,         "·", "˙", "•");
G_(g_smoke,        "∽", "·", "˙");
G_(g_breath,       "🌬", "∽", "·", "˙");
G_(g_warmth,       "🌞", "☼", "·", "˙");
G_(g_valley,       "🏞", "∨", "·", "˙");
G_(g_hill,         "⛰", "△", "·", "˙");

/* --- fire / light ---------------------------------------------------- */
G_(g_candle,       "🕯", "✦", "·", "˙");
G_(g_flame,        "🔥", "✦", "✧", "·");
G_(g_lantern,      "🏮", "◯", "·", "˙");

/* --- people ---------------------------------------------------------- */
G_(g_village,      "🏘", "⌂", "·", "◦");
G_(g_children,     "🧒", "·", "˙", "✦");
G_(g_child,        "🧒", "·", "˙", "✦");
G_(g_warriors,     "⚔\xef\xb8\x8f", "┼", "╋", "╳");
G_(g_messenger,    "🏃", "→", "·", "˙");
G_(g_monk,         "🧘", "○", "·", "˙");
G_(g_traveler,     "🚶", "→", "·", "˙");
G_(g_beggar,       "·", "˙", "○");
G_(g_woman,        "👩", "○", "·", "˙");
G_(g_man,          "👨", "○", "·", "˙");
G_(g_pilgrim,      "🚶", "→", "·", "˙");
G_(g_thief,        "·", "˙", "˚");
G_(g_bell,         "🔔", "☉", "◉", "○");
G_(g_stone,        "🪨", "◉", "●", "○");

/* --- places / objects ------------------------------------------------ */
G_(g_temple,       "⛩\xef\xb8\x8f", "🛕", "⌂", "△");
G_(g_gate,         "⛩\xef\xb8\x8f", "∏", "·", "˙");
G_(g_road,         "🛣", "┄", "─", "·");
G_(g_roof,         "🏠", "∧", "·", "˙");
G_(g_hut,          "🛖", "⌂", "·", "˙");
G_(g_boat,         "⛵\xef\xb8\x8f", "⌒", "·", "˙");
G_(g_sail,         "⛵\xef\xb8\x8f", "△", "·", "˙");
G_(g_mask,         "🎭", "○", "·", "˙");
G_(g_fan,          "🪭", "⌒", "·", "˙");
G_(g_garden,       "🌷", "🌼", "✿", "·");
G_(g_field,        "🌾", "│", "·", "˙");
G_(g_window,       "🪟", "□", "·", "˙");
G_(g_basket,       "🧺", "⌣", "·", "˙");
G_(g_pipe,         "∼", "·", "˙");
G_(g_comb,         "│", "·", "˙");
G_(g_mirror,       "🪞", "◯", "○", "·");
G_(g_cart,         "🛒", "◉", "·", "˙");
G_(g_pot,          "🪴", "⌒", "·", "˙");
G_(g_island,       "🏝", "◬", "·", "˙");
G_(g_stable,       "🏠", "⌂", "·", "˙");
G_(g_rock,         "🪨", "◉", "●", "○");
G_(g_kite,         "🪁", "◇", "·", "˙");
G_(g_flag,         "🚩", "△", "·", "˙");
G_(g_bay,          "🌊", "⌒", "·", "˙");
G_(g_sword,        "🗡", "│", "·", "˙");

/* --- actions / abstractions ----------------------------------------- */
G_(g_stillness,    " ", "·", "˙");
G_(g_silence,      " ", "·", "˙");
G_(g_joy,          "✦", "✧", "·");
G_(g_kindness,     "✿", "·", "˙");
G_(g_dream,        "∽", "·", "˙");
G_(g_memory,       "˙", "·", "∙");
G_(g_transfer,     "→", "·", "˙");
G_(g_fragrance,    "∽", "～", "·");
G_(g_drift,        "∿", "～", "·");
G_(g_passing,      "·", "˙", "◦");
G_(g_echo,         "∙", "·", "˙");
G_(g_first,        "◉", "○", "·");
G_(g_beginning,    "◌", "○", "·");
G_(g_loneliness,   "·", "˙", " ");
G_(g_journey,      "→", "·", "˙");
G_(g_year,         "○", "◌", "·");
G_(g_sickness,     "·", "˙", " ");
G_(g_prayer,       "·", "˙", "○");
G_(g_parting,      "·", "˙", "→");
G_(g_song,         "∽", "·", "˙");
G_(g_voice,        "∽", "·", "˙");
G_(g_scent,        "∽", "·", "˙");
G_(g_sound,        "∽", "·", "˙");
G_(g_longing,      "·", "˙", "◦");
G_(g_gratitude,    "✿", "·", "˙");
G_(g_sleep,        "·", "˙", " ");
G_(g_dance,        "✦", "·", "˙");
G_(g_climbing,     "↑", "·", "˙");
G_(g_vanishing,    "·", "˙", " ");
G_(g_withered,     "╴", "·", "˙");
G_(g_world,        "○", "◌", "·");
G_(g_tears,        "·", "˙", "·");
G_(g_coolness,     "❅", "·", "˙");
G_(g_opening,      "○", "◌", "·");
G_(g_awakening,    "✦", "·", "˙");
G_(g_lingering,    "∽", "·", "˙");
G_(g_current,      "～", "∽", "·");
G_(g_freedom,      "✦", "·", "˙");
G_(g_regret,       "·", "˙", " ");
G_(g_secret,       "·", "˙", " ");
G_(g_buddha,       "○", "◯", "·");
G_(g_hell,         "·", "˙", "•");
G_(g_irony,        "·", "˙", " ");
G_(g_frailty,      "·", "˙", " ");
G_(g_patience,     "·", "˙", " ");
G_(g_slow,         "·", "˙", " ");
G_(g_work,         "│", "·", "˙");
G_(g_planting,     "│", "·", "˙");
G_(g_return,       "←", "·", "˙");
G_(g_stretching,   "→", "·", "˙");
G_(g_sudden,       "✦", "·", "˙");
G_(g_breaking,     "·", "˙", "·");
G_(g_rice,         "│", "·", "˙");
G_(g_mud,          "·", "•", "˙");
G_(g_hair,         "│", "·", "˙");
G_(g_face,         "○", "·", "˙");
G_(g_dust,         "·", "˙", "·");
G_(g_crossing,     "┼", "·", "˙");
G_(g_repetition,   "·", "˙", "·");
G_(g_smile,        "⌣", "·", "˙");
G_(g_ash,          "·", "˙", "·");

/* --- paths / structure ----------------------------------------------- */
G_(g_path,         "┄", "─", "·");
G_(g_bridge,       "═", "─", "┄");

/* --- seasonal -------------------------------------------------------- */
G_(g_spring,       "🌸", "🌷", "✿", "❀");
G_(g_summer,       "🌞", "🌻", "☼", "✦");
G_(g_autumn,       "🍁", "🍂", "⌬", "·");
G_(g_winter,       "🌨", "❄", "❅", "·");
G_(g_new_year,     "🎍", "🎋", "◉", "✦");

#define E(t, arr) { t, arr, sizeof(arr) / sizeof((arr)[0]) }

static const symbol_entry SYMBOLS[] = {
    /* water */
    E("pond", g_pond), E("water", g_water), E("splash", g_splash),
    E("well", g_well), E("flood", g_flood), E("sea", g_sea),
    E("ocean", g_ocean), E("river", g_river), E("tide", g_tide),
    E("ice", g_ice), E("dew", g_dew), E("frost", g_frost),
    E("rain", g_rain), E("mist", g_mist), E("cloud", g_cloud),
    E("pool", g_pool), E("shore", g_shore), E("thaw", g_thaw),
    E("ford", g_ford),
    /* creatures */
    E("frog", g_frog), E("crow", g_crow), E("worm", g_worm),
    E("firefly", g_firefly), E("cicada", g_cicada), E("cuckoo", g_cuckoo),
    E("dragonfly", g_dragonfly), E("butterfly", g_butterfly),
    E("cricket", g_cricket), E("sparrow", g_sparrow), E("goose", g_goose),
    E("duck", g_duck), E("heron", g_heron), E("snail", g_snail),
    E("fish", g_fish), E("octopus", g_octopus), E("mosquito", g_mosquito),
    E("spider", g_spider), E("deer", g_deer), E("horse", g_horse),
    E("cat", g_cat), E("skylark", g_skylark), E("monkey", g_monkey),
    E("fly", g_fly),
    /* plants */
    E("branch", g_branch), E("grass", g_grass),
    E("morning_glory", g_morning_glory), E("plum", g_plum),
    E("blossom", g_blossom), E("petal", g_petal), E("vine", g_vine),
    E("chestnut", g_chestnut), E("cherry", g_cherry),
    E("willow", g_willow), E("peony", g_peony), E("wisteria", g_wisteria),
    E("leaf", g_leaf), E("persimmon", g_persimmon), E("lotus", g_lotus),
    E("bamboo", g_bamboo), E("pine", g_pine), E("iris", g_iris),
    E("chrysanthemum", g_chrysanthemum), E("mustard", g_mustard),
    E("radish", g_radish), E("tree", g_tree), E("rose", g_rose),
    /* weather / sky */
    E("snow", g_snow), E("moon", g_moon), E("star", g_star),
    E("milky_way", g_milky_way), E("sun", g_sun), E("dawn", g_dawn),
    E("dusk", g_dusk), E("evening", g_evening), E("twilight", g_twilight),
    E("night", g_night), E("morning", g_morning), E("horizon", g_horizon),
    E("mountain", g_mountain), E("wind", g_wind), E("breeze", g_breeze),
    E("lightning", g_lightning), E("color", g_color), E("white", g_white),
    E("dark", g_dark), E("smoke", g_smoke), E("breath", g_breath),
    E("warmth", g_warmth), E("valley", g_valley), E("hill", g_hill),
    /* fire / light */
    E("candle", g_candle), E("flame", g_flame), E("lantern", g_lantern),
    /* people */
    E("village", g_village), E("children", g_children), E("child", g_child),
    E("warriors", g_warriors), E("messenger", g_messenger), E("monk", g_monk),
    E("traveler", g_traveler), E("beggar", g_beggar), E("woman", g_woman),
    E("man", g_man), E("pilgrim", g_pilgrim), E("thief", g_thief),
    E("bell", g_bell), E("stone", g_stone),
    /* places / objects */
    E("temple", g_temple), E("gate", g_gate), E("road", g_road),
    E("roof", g_roof), E("hut", g_hut), E("boat", g_boat),
    E("sail", g_sail), E("mask", g_mask), E("fan", g_fan),
    E("garden", g_garden), E("field", g_field), E("window", g_window),
    E("basket", g_basket), E("pipe", g_pipe), E("comb", g_comb),
    E("mirror", g_mirror), E("cart", g_cart), E("pot", g_pot),
    E("island", g_island), E("stable", g_stable), E("rock", g_rock),
    E("kite", g_kite), E("flag", g_flag), E("bay", g_bay), E("sword", g_sword),
    /* actions / abstractions */
    E("stillness", g_stillness), E("silence", g_silence), E("joy", g_joy),
    E("kindness", g_kindness), E("dream", g_dream), E("memory", g_memory),
    E("transfer", g_transfer), E("fragrance", g_fragrance),
    E("drift", g_drift), E("passing", g_passing), E("echo", g_echo),
    E("first", g_first), E("beginning", g_beginning),
    E("loneliness", g_loneliness), E("journey", g_journey),
    E("year", g_year), E("sickness", g_sickness), E("prayer", g_prayer),
    E("parting", g_parting), E("song", g_song), E("voice", g_voice),
    E("scent", g_scent), E("sound", g_sound), E("longing", g_longing),
    E("gratitude", g_gratitude), E("sleep", g_sleep), E("dance", g_dance),
    E("climbing", g_climbing), E("vanishing", g_vanishing),
    E("withered", g_withered), E("world", g_world), E("tears", g_tears),
    E("coolness", g_coolness), E("opening", g_opening),
    E("awakening", g_awakening), E("lingering", g_lingering),
    E("current", g_current), E("freedom", g_freedom), E("regret", g_regret),
    E("secret", g_secret), E("buddha", g_buddha), E("hell", g_hell),
    E("irony", g_irony), E("frailty", g_frailty), E("patience", g_patience),
    E("slow", g_slow), E("work", g_work), E("planting", g_planting),
    E("return", g_return), E("stretching", g_stretching),
    E("sudden", g_sudden), E("breaking", g_breaking), E("rice", g_rice),
    E("mud", g_mud), E("hair", g_hair), E("face", g_face),
    E("dust", g_dust), E("crossing", g_crossing),
    E("repetition", g_repetition), E("smile", g_smile), E("ash", g_ash),
    /* paths */
    E("path", g_path), E("bridge", g_bridge),
    /* seasonal */
    E("spring", g_spring), E("summer", g_summer), E("autumn", g_autumn),
    E("winter", g_winter), E("new_year", g_new_year),
};

static const size_t N_SYMBOLS = sizeof(SYMBOLS) / sizeof(SYMBOLS[0]);

const char *const *hk_glyphs_for(const char *token, size_t *out_n)
{
    if (token == NULL) return NULL;
    for (size_t i = 0; i < N_SYMBOLS; ++i) {
        if (strcmp(SYMBOLS[i].token, token) == 0) {
            if (out_n) *out_n = SYMBOLS[i].n_glyphs;
            return SYMBOLS[i].glyphs;
        }
    }
    return NULL;
}

/* --- text-only filter (cached per token for stable pointers) ------- */

static const char *FALLBACK_TEXT_GLYPHS[] = {"◯"};

typedef struct {
    bool        computed;
    const char *glyphs[8];
    size_t      n;
} filtered_cache;

static filtered_cache TEXT_CACHE[sizeof(SYMBOLS) / sizeof(SYMBOLS[0])];

const char *const *hk_text_glyphs_for(const char *token, size_t *out_n)
{
    if (!token) return NULL;
    for (size_t i = 0; i < N_SYMBOLS; ++i) {
        if (strcmp(SYMBOLS[i].token, token) != 0) continue;
        filtered_cache *cache = &TEXT_CACHE[i];
        if (!cache->computed) {
            size_t out = 0;
            for (size_t j = 0; j < SYMBOLS[i].n_glyphs && out < 8; ++j) {
                if (!hk_is_emoji(SYMBOLS[i].glyphs[j])) {
                    cache->glyphs[out++] = SYMBOLS[i].glyphs[j];
                }
            }
            cache->n = out;
            cache->computed = true;
        }
        if (cache->n == 0) {
            if (out_n) *out_n = 1;
            return FALLBACK_TEXT_GLYPHS;
        }
        if (out_n) *out_n = cache->n;
        return (const char *const *)cache->glyphs;
    }
    return NULL;
}
