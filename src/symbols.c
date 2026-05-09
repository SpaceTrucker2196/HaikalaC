/* Token → glyph vocabulary, ported from the Python upstream. Keeps the
 * artistic mix of three layers: emoji as bright subjects, Unicode
 * dingbats/geometric symbols as essences, and box-drawing/dot
 * characters as connective negative space.
 *
 * Token order in each entry mirrors the upstream: the first glyph is
 * the strongest "subject" form (often emoji), later glyphs decay into
 * quieter symbols. The mandala's center always uses index [0]. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "haikala.h"

/* ---- glyph tuples --------------------------------------------------- */

#define G(...) (const char *const[]){__VA_ARGS__}
#define GN(...) (sizeof((const char *const[]){__VA_ARGS__}) / sizeof(const char *))

typedef struct {
    const char        *token;
    const char *const *glyphs;
    size_t             n_glyphs;
} symbol_entry;

/* Static glyph arrays. Defining each as a separate static-storage array
 * avoids relying on compound literal lifetime rules. */

static const char *const g_pond[]      = { "💧", "◯", "○", "～" };
static const char *const g_water[]     = { "💧", "～", "≈", "∽" };
static const char *const g_splash[]    = { "💦", "✦", "✧", "·" };
static const char *const g_well[]      = { "💧", "◉", "◎", "○" };
static const char *const g_sea[]       = { "🌊", "≈", "～", "∿" };
static const char *const g_wave[]      = { "🌊", "～", "∽", "∿" };
static const char *const g_river[]     = { "🌊", "～", "∽", "∿" };

static const char *const g_frog[]      = { "🐸", "🪷", "✿", "·" };
static const char *const g_crow[]      = { "🦅", "🐦", "✦", "⬢" };
static const char *const g_heron[]     = { "🦅", "✦", "○", "·" };

static const char *const g_branch[]    = { "🌿", "─", "│", "┄" };
static const char *const g_grass[]     = { "🌱", "❀", "✿", "·" };
static const char *const g_blossom[]   = { "🌸", "✿", "❀", "·" };
static const char *const g_morning_glory[] = { "🌺", "❀", "✿", "·" };

static const char *const g_evening[]   = { "🌆", "☾", "✦", "·" };
static const char *const g_night[]     = { "🌙", "☾", "✦", "·" };
static const char *const g_moon[]      = { "🌕", "☾", "◯", "·" };
static const char *const g_star[]      = { "✦", "✧", "·", "˙" };

static const char *const g_lightning[] = { "⚡", "✦", "✧", "·" };
static const char *const g_snow[]      = { "❄", "❅", "❆", "·" };
static const char *const g_bell[]      = { "🔔", "◉", "○", "·" };
static const char *const g_temple[]    = { "⛩", "△", "◇", "·" };
static const char *const g_road[]      = { "┄", "─", "·", "˙" };
static const char *const g_bridge[]    = { "═", "─", "┄", "·" };

static const char *const g_warriors[]  = { "🗡", "⚔", "✦", "·" };

static const char *const g_stillness[] = { "·", "˙", "░", "▒" };
static const char *const g_silence[]   = { "·", "˙", "░", "▒" };
static const char *const g_dream[]     = { "☁", "✦", "˙", "·" };
static const char *const g_memory[]    = { "◯", "·", "✦", "˙" };
static const char *const g_first[]     = { "✦", "✧", "·", "˙" };
static const char *const g_echo[]      = { "◌", "·", "˙", "○" };
static const char *const g_awakening[] = { "✦", "✧", "✩", "·" };
static const char *const g_loneliness[]= { "·", "˙", "░", "▒" };
static const char *const g_longing[]   = { "✧", "·", "˙", "○" };
static const char *const g_joy[]       = { "✦", "❀", "✧", "·" };
static const char *const g_vastness[]  = { "≈", "·", "˙", "∽" };

#define E(t, arr) { t, arr, sizeof(arr) / sizeof((arr)[0]) }

static const symbol_entry SYMBOLS[] = {
    /* water */
    E("pond",      g_pond),
    E("water",     g_water),
    E("splash",    g_splash),
    E("well",      g_well),
    E("sea",       g_sea),
    E("wave",      g_wave),
    E("river",     g_river),
    /* creatures */
    E("frog",      g_frog),
    E("crow",      g_crow),
    E("heron",     g_heron),
    /* plants */
    E("branch",    g_branch),
    E("grass",     g_grass),
    E("blossom",   g_blossom),
    E("morning_glory", g_morning_glory),
    /* time / sky */
    E("evening",   g_evening),
    E("night",     g_night),
    E("moon",      g_moon),
    E("star",      g_star),
    E("lightning", g_lightning),
    E("snow",      g_snow),
    /* objects / structures */
    E("bell",      g_bell),
    E("temple",    g_temple),
    E("road",      g_road),
    E("bridge",    g_bridge),
    E("warriors",  g_warriors),
    /* atmosphere / sensation */
    E("stillness", g_stillness),
    E("silence",   g_silence),
    E("dream",     g_dream),
    E("memory",    g_memory),
    E("first",     g_first),
    E("echo",      g_echo),
    E("awakening", g_awakening),
    E("loneliness", g_loneliness),
    E("longing",   g_longing),
    E("joy",       g_joy),
    E("vastness",  g_vastness),
};

static const size_t N_SYMBOLS = sizeof(SYMBOLS) / sizeof(SYMBOLS[0]);

const char *const *hk_glyphs_for(const char *token, size_t *out_n)
{
    if (token == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < N_SYMBOLS; ++i) {
        if (strcmp(SYMBOLS[i].token, token) == 0) {
            if (out_n) {
                *out_n = SYMBOLS[i].n_glyphs;
            }
            return SYMBOLS[i].glyphs;
        }
    }
    return NULL;
}
