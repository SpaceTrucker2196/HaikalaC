/* Color math + named fractal palettes + word-derived auto palette and
 * symmetry. Ports the Python upstream's palette.py and FRACTAL_PALETTES.
 *
 * RGB↔HLS conversion follows the standard hue/lightness/saturation
 * formulation (the same one Python's colorsys uses), so hue rotation
 * here matches what _shift_hue does in animate.py.
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "haikala.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- RGB↔HLS ------------------------------------------------------- */

static double dmin3(double a, double b, double c) {
    double m = a < b ? a : b;
    return m < c ? m : c;
}
static double dmax3(double a, double b, double c) {
    double m = a > b ? a : b;
    return m > c ? m : c;
}

static void rgb_to_hls(double r, double g, double b,
                       double *h, double *l, double *s)
{
    double mx = dmax3(r, g, b);
    double mn = dmin3(r, g, b);
    double L = (mx + mn) * 0.5;
    if (mx == mn) {
        *h = 0.0;
        *l = L;
        *s = 0.0;
        return;
    }
    double d = mx - mn;
    double S = (L > 0.5) ? d / (2.0 - mx - mn) : d / (mx + mn);
    double H;
    if (mx == r)      H = (g - b) / d + (g < b ? 6.0 : 0.0);
    else if (mx == g) H = (b - r) / d + 2.0;
    else              H = (r - g) / d + 4.0;
    H /= 6.0;
    *h = H;
    *l = L;
    *s = S;
}

static double hue_to_channel(double p, double q, double t)
{
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 0.5)       return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}

static void hls_to_rgb(double h, double l, double s,
                       double *r, double *g, double *b)
{
    if (s == 0.0) { *r = *g = *b = l; return; }
    double q = (l < 0.5) ? l * (1.0 + s) : l + s - l * s;
    double p = 2.0 * l - q;
    *r = hue_to_channel(p, q, h + 1.0 / 3.0);
    *g = hue_to_channel(p, q, h);
    *b = hue_to_channel(p, q, h - 1.0 / 3.0);
}

hk_rgb hk_rotate_hue(hk_rgb base, double degrees)
{
    double h, l, s;
    rgb_to_hls(base.r / 255.0, base.g / 255.0, base.b / 255.0, &h, &l, &s);
    h = fmod(h + degrees / 360.0, 1.0);
    if (h < 0) h += 1.0;
    double r, g, b;
    hls_to_rgb(h, l, s, &r, &g, &b);
    hk_rgb out;
    long ir = (long)(r * 255.0 + 0.5);
    long ig = (long)(g * 255.0 + 0.5);
    long ib = (long)(b * 255.0 + 0.5);
    if (ir < 0) ir = 0; if (ir > 255) ir = 255;
    if (ig < 0) ig = 0; if (ig > 255) ig = 255;
    if (ib < 0) ib = 0; if (ib > 255) ib = 255;
    out.r = (uint8_t)ir; out.g = (uint8_t)ig; out.b = (uint8_t)ib;
    return out;
}

/* ---- named fractal palettes (8 stops each, dark→bright) ----------- */

#define HEX(rgb) {0x##rgb >> 16, (0x##rgb >> 8) & 0xFF, 0x##rgb & 0xFF}

/* Each palette: 8 RGB stops. Index 0 = deepest dark, index 7 = brightest tip. */
static const hk_rgb PALETTES[HK_PAL_COUNT][HK_PALETTE_STOPS] = {
    /* aurora */ {
        HEX(000a1a), HEX(022040), HEX(0a4060), HEX(19828c),
        HEX(2cd9a0), HEX(7af56a), HEX(c270ff), HEX(ffaff0),
    },
    /* ember */ {
        HEX(0a0000), HEX(2b0500), HEX(660f00), HEX(a82800),
        HEX(e65800), HEX(ff9d20), HEX(ffd76a), HEX(fff8d0),
    },
    /* ocean */ {
        HEX(000814), HEX(001d3d), HEX(003566), HEX(0077b6),
        HEX(00b4d8), HEX(48cae4), HEX(caf0f8), HEX(ffffff),
    },
    /* forest */ {
        HEX(0a1f0a), HEX(15391a), HEX(2c5a2c), HEX(4c7a2c),
        HEX(7ea848), HEX(b8c870), HEX(e8c878), HEX(fff0c0),
    },
    /* sakura */ {
        HEX(1a0e1a), HEX(3a1f30), HEX(6e3a55), HEX(a55a7a),
        HEX(dc88a8), HEX(f5b6cc), HEX(fcdde6), HEX(ffffff),
    },
    /* twilight */ {
        HEX(080216), HEX(1a0a3a), HEX(36195e), HEX(5a2887),
        HEX(8e3ec0), HEX(c266e0), HEX(ec96f0), HEX(fcd9ff),
    },
    /* lava */ {
        HEX(050505), HEX(1a0a05), HEX(3d1a0a), HEX(7a2a14),
        HEX(c75028), HEX(ee8b3a), HEX(f5cf6a), HEX(fff5d0),
    },
    /* coral */ {
        HEX(001a1a), HEX(024040), HEX(157070), HEX(3aa8a0),
        HEX(ffb88a), HEX(ff8466), HEX(ffd6b0), HEX(ffffff),
    },
};

static const char *PALETTE_NAMES[HK_PAL_COUNT] = {
    "aurora", "ember", "ocean", "forest",
    "sakura", "twilight", "lava", "coral",
};

const hk_rgb *hk_palette_named(hk_palette_id id)
{
    if (id < 0 || id >= HK_PAL_COUNT) {
        return PALETTES[HK_PAL_AURORA];
    }
    return PALETTES[id];
}

hk_palette_id hk_palette_id_from_name(const char *name)
{
    if (!name) return (hk_palette_id)-1;
    for (int i = 0; i < HK_PAL_COUNT; ++i) {
        if (strcmp(name, PALETTE_NAMES[i]) == 0) {
            return (hk_palette_id)i;
        }
    }
    return (hk_palette_id)-1;
}

const char *hk_palette_name(hk_palette_id id)
{
    if (id < 0 || id >= HK_PAL_COUNT) return "?";
    return PALETTE_NAMES[id];
}

/* ---- HAIKU_COLOR_WORDS dictionary --------------------------------- */

typedef struct { const char *word; hk_rgb color; } word_color;

/* Pulled directly from the Python palette.py. Lowercased. */
static const word_color WORDS[] = {
    /* celestial */
    {"moon", HEX(e6e2c0)}, {"moons", HEX(e6e2c0)}, {"moonlight", HEX(e6e2c0)},
    {"sun", HEX(ffd06a)}, {"sunlight", HEX(ffd06a)}, {"sunshine", HEX(ffd06a)},
    {"star", HEX(e8e8ff)}, {"stars", HEX(e8e8ff)},
    /* times of day */
    {"morning", HEX(ffba70)}, {"dawn", HEX(ffb88a)},
    {"noon", HEX(fff0a0)},
    {"evening", HEX(705080)}, {"dusk", HEX(9060a0)}, {"twilight", HEX(7050a0)},
    {"night", HEX(0a1030)}, {"nightfall", HEX(0a1030)}, {"midnight", HEX(000814)},
    /* weather / sky */
    {"rain", HEX(7090b0)}, {"raindrop", HEX(90b0d0)}, {"drop", HEX(90b0d0)}, {"drops", HEX(90b0d0)},
    {"snow", HEX(f0f6ff)}, {"snowflake", HEX(dce8f0)},
    {"frost", HEX(cde4f0)}, {"ice", HEX(b0d8e8)},
    {"fire", HEX(ff4d20)}, {"flame", HEX(ff8030)}, {"ember", HEX(d04020)}, {"embers", HEX(d04020)},
    {"smoke", HEX(a0a0a8)},
    {"mist", HEX(c0c8d0)}, {"fog", HEX(b0b8c0)},
    {"wind", HEX(b0c8d8)}, {"breeze", HEX(b0c8d8)},
    {"lightning", HEX(fff8a0)}, {"storm", HEX(506070)},
    {"rainbow", HEX(c870e0)},
    /* seasons */
    {"spring", HEX(a0d050)}, {"summer", HEX(80c020)},
    {"autumn", HEX(d8702a)}, {"fall", HEX(d8702a)},
    {"winter", HEX(c0d8e8)},
    /* plants */
    {"blossom", HEX(ffb0c8)}, {"blossoms", HEX(ffb0c8)},
    {"cherry", HEX(ff90b0)}, {"plum", HEX(a04060)},
    {"petal", HEX(ffc0d0)}, {"petals", HEX(ffc0d0)},
    {"leaf", HEX(60a040)}, {"leaves", HEX(60a040)},
    {"grass", HEX(7ab050)}, {"grasses", HEX(7ab050)},
    {"moss", HEX(5a8040)}, {"pine", HEX(306030)}, {"bamboo", HEX(80a050)},
    {"willow", HEX(a0c060)}, {"lotus", HEX(f0c0d8)},
    {"lily", HEX(f8f0f8)}, {"iris", HEX(7050a0)}, {"fern", HEX(508040)},
    {"branch", HEX(604030)}, {"branches", HEX(604030)},
    {"tree", HEX(3a5a2a)}, {"trees", HEX(3a5a2a)}, {"wood", HEX(704030)},
    {"rice", HEX(f0e0b0)}, {"wheat", HEX(e0c870)}, {"field", HEX(a08840)}, {"fields", HEX(a08840)},
    {"seed", HEX(a08040)}, {"seeds", HEX(a08040)},
    /* water */
    {"sky", HEX(80b8e0)},
    {"cloud", HEX(e8e8f0)}, {"clouds", HEX(e8e8f0)},
    {"river", HEX(3a7aa8)}, {"stream", HEX(5090c0)},
    {"water", HEX(4080b0)}, {"pond", HEX(3070a0)}, {"lake", HEX(286490)},
    {"sea", HEX(106090)}, {"ocean", HEX(005078)},
    {"wave", HEX(3088b0)}, {"waves", HEX(3088b0)},
    {"splash", HEX(90c8e8)}, {"puddle", HEX(5080a0)},
    {"well", HEX(3a6080)},
    /* creatures */
    {"crow", HEX(101010)}, {"raven", HEX(101010)},
    {"frog", HEX(508040)}, {"frogs", HEX(508040)},
    {"fish", HEX(a0b0c0)},
    {"bird", HEX(a08070)}, {"birds", HEX(a08070)},
    {"swallow", HEX(3a3a48)}, {"heron", HEX(e0e0e0)}, {"crane", HEX(f0f0f0)},
    {"duck", HEX(506050)},
    {"deer", HEX(a07050)}, {"horse", HEX(603020)}, {"monkey", HEX(806040)},
    {"snail", HEX(988868)}, {"spider", HEX(403030)},
    {"bee", HEX(f0c020)}, {"butterfly", HEX(e8b0e0)}, {"dragonfly", HEX(5090b0)},
    {"firefly", HEX(c8ee88)}, {"fireflies", HEX(c8ee88)},
    {"cricket", HEX(60702a)}, {"cicada", HEX(a0a020)},
    /* earth & landscape */
    {"stone", HEX(808080)}, {"stones", HEX(808080)},
    {"rock", HEX(707070)}, {"rocks", HEX(707070)},
    {"earth", HEX(604030)}, {"soil", HEX(604030)},
    {"mountain", HEX(506070)}, {"mountains", HEX(506070)},
    {"valley", HEX(5a7050)},
    {"road", HEX(888070)}, {"path", HEX(888070)},
    {"bridge", HEX(a08868)}, {"house", HEX(806030)},
    {"temple", HEX(a06848)}, {"garden", HEX(7a9a48)},
    /* named colors */
    {"gold", HEX(ffc848)}, {"silver", HEX(c8d0d8)},
    {"red", HEX(d83040)}, {"white", HEX(f8f8f8)}, {"black", HEX(101010)},
    {"green", HEX(40a040)}, {"blue", HEX(3070b0)},
    {"purple", HEX(7040a0)}, {"yellow", HEX(f0d040)},
    {"orange", HEX(ff8830)}, {"pink", HEX(ff90b0)},
    {"rose", HEX(e06080)}, {"scarlet", HEX(d83040)}, {"crimson", HEX(aa1830)},
    /* abstract */
    {"shadow", HEX(404048)}, {"shadows", HEX(404048)},
    {"warriors", HEX(603030)},
    {"dream", HEX(704080)}, {"dreams", HEX(704080)},
    {"memory", HEX(605880)},
    {"stillness", HEX(506070)}, {"silence", HEX(506070)},
};
static const size_t N_WORDS = sizeof(WORDS) / sizeof(WORDS[0]);

static const hk_rgb *lookup_color(const char *word)
{
    for (size_t i = 0; i < N_WORDS; ++i) {
        if (strcmp(WORDS[i].word, word) == 0) {
            return &WORDS[i].color;
        }
    }
    return NULL;
}

/* Lowercase one ASCII word from buf into `out` (max len). */
static void lower_copy(char *dst, size_t cap, const char *src, size_t srclen)
{
    if (cap == 0) return;
    size_t n = (srclen < cap - 1) ? srclen : cap - 1;
    for (size_t i = 0; i < n; ++i) {
        char c = src[i];
        dst[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
    }
    dst[n] = '\0';
}

/* ---- auto fold (haiku → symmetry order) -------------------------- */

static int count_words(const char *line)
{
    int n = 0;
    bool in_word = false;
    if (!line) return 0;
    for (const char *s = line; *s; ++s) {
        bool is_alpha = ((*s >= 'a' && *s <= 'z') ||
                         (*s >= 'A' && *s <= 'Z'));
        if (is_alpha) {
            if (!in_word) { ++n; in_word = true; }
        } else {
            in_word = false;
        }
    }
    return n;
}

int hk_fold_for_haiku(const hk_haiku *h)
{
    if (!h) return 8;
    int n_tokens = (int)h->n_tokens;
    int n_words = count_words(h->line1) + count_words(h->line2) +
                  count_words(h->line3);
    double score = 1.5 * (double)n_tokens + (double)n_words / 3.0;
    int fold = (int)floor(score + 0.5);
    if (fold < 4)  fold = 4;
    if (fold > 16) fold = 16;
    if (fold % 2)  ++fold;
    if (fold > 16) fold = 16;
    return fold;
}

/* ---- auto palette (haiku words → 8-stop ramp) -------------------- */

static double luminance(hk_rgb c)
{
    return 0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
}

static hk_rgb lerp_rgb(hk_rgb a, hk_rgb b, double t)
{
    if (t < 0) t = 0; else if (t > 1) t = 1;
    hk_rgb out;
    out.r = (uint8_t)(a.r + (b.r - a.r) * t + 0.5);
    out.g = (uint8_t)(a.g + (b.g - a.g) * t + 0.5);
    out.b = (uint8_t)(a.b + (b.b - a.b) * t + 0.5);
    return out;
}

static void resample_anchors(const hk_rgb *anchors, int n_anchors,
                             hk_rgb *out, int n_stops)
{
    if (n_anchors == 1) {
        for (int i = 0; i < n_stops; ++i) out[i] = anchors[0];
        return;
    }
    int last = n_anchors - 1;
    for (int i = 0; i < n_stops; ++i) {
        double pos = (double)i / (double)(n_stops - 1) * (double)last;
        int lo = (int)floor(pos);
        if (lo >= last) { out[i] = anchors[last]; continue; }
        double frac = pos - (double)lo;
        out[i] = lerp_rgb(anchors[lo], anchors[lo + 1], frac);
    }
}

static int find_or_append(hk_rgb *list, int n, hk_rgb c)
{
    for (int i = 0; i < n; ++i) {
        if (list[i].r == c.r && list[i].g == c.g && list[i].b == c.b) {
            return n;
        }
    }
    list[n] = c;
    return n + 1;
}

bool hk_palette_from_haiku(const hk_haiku *h, hk_rgb out[HK_PALETTE_STOPS])
{
    if (!h || !out) return false;

    hk_rgb matches[64];
    int n_matches = 0;

    /* Tokens first (curated essence). */
    for (size_t i = 0; i < h->n_tokens && n_matches < 64; ++i) {
        char w[64];
        lower_copy(w, sizeof(w), h->tokens[i], strlen(h->tokens[i]));
        const hk_rgb *c = lookup_color(w);
        if (c) n_matches = find_or_append(matches, n_matches, *c);
    }
    /* Then full-text words from the lines. */
    const char *lines[] = {h->line1, h->line2, h->line3};
    for (int li = 0; li < 3 && n_matches < 64; ++li) {
        const char *s = lines[li];
        const char *p = s;
        while (*p) {
            while (*p && !((*p >= 'a' && *p <= 'z') ||
                           (*p >= 'A' && *p <= 'Z'))) ++p;
            const char *start = p;
            while (*p && ((*p >= 'a' && *p <= 'z') ||
                          (*p >= 'A' && *p <= 'Z'))) ++p;
            if (p == start) break;
            char w[64];
            lower_copy(w, sizeof(w), start, (size_t)(p - start));
            const hk_rgb *c = lookup_color(w);
            if (c && n_matches < 64) {
                n_matches = find_or_append(matches, n_matches, *c);
            }
        }
    }
    if (n_matches == 0) return false;

    /* Sort by luminance ascending (insertion sort — n is tiny). */
    for (int i = 1; i < n_matches; ++i) {
        hk_rgb cur = matches[i];
        double lc = luminance(cur);
        int j = i - 1;
        while (j >= 0 && luminance(matches[j]) > lc) {
            matches[j + 1] = matches[j];
            --j;
        }
        matches[j + 1] = cur;
    }

    /* Build anchors: deep-night → matches → brightened-tip. */
    hk_rgb anchors[66];
    int n_anchors = 0;
    anchors[n_anchors++] = (hk_rgb){0x02, 0x02, 0x08};
    for (int i = 0; i < n_matches; ++i) anchors[n_anchors++] = matches[i];
    /* tip = brightest match blended 30% toward white */
    hk_rgb tip = lerp_rgb(matches[n_matches - 1],
                          (hk_rgb){0xff, 0xff, 0xff}, 0.30);
    anchors[n_anchors++] = tip;

    resample_anchors(anchors, n_anchors, out, HK_PALETTE_STOPS);
    return true;
}

/* ---- weather-derived palettes ------------------------------------- */

/* Four base season ramps used by the --weather mode. Distinct from the
 * Python upstream's `PALETTES` dict (those map seasons → Rich color
 * names for ring fg colors). These are 8-stop fractal-style ramps,
 * dark→bright, designed to read as the *atmosphere* of each season:
 *   spring: damp earth → green shoots → blossom
 *   summer: deep teal water → bright sky → sunlit gold
 *   autumn: charred bark → ember → harvest gold
 *   winter: deep navy → ice blue → snow
 *
 * The condition modifier then nudges these in HLS space. */
static const hk_rgb WEATHER_SEASONS[4][HK_PALETTE_STOPS] = {
    /* SPRING */ {
        HEX(0a1810), HEX(152e1a), HEX(2c5a30), HEX(64a850),
        HEX(b0d878), HEX(ffd0e0), HEX(f6f8e8), HEX(ffffff),
    },
    /* SUMMER */ {
        HEX(001020), HEX(003845), HEX(0a708a), HEX(3da6c0),
        HEX(90d8e0), HEX(ffe888), HEX(fff0c0), HEX(ffffe8),
    },
    /* AUTUMN */ {
        HEX(100804), HEX(2b1208), HEX(5e2c10), HEX(a8501c),
        HEX(e8862a), HEX(ffc060), HEX(ffe098), HEX(fff5d8),
    },
    /* WINTER */ {
        HEX(000814), HEX(0a1a30), HEX(264870), HEX(5c8ab0),
        HEX(a8c8e0), HEX(e0eaf6), HEX(f0f8ff), HEX(ffffff),
    },
};

/* Condition modifier in HLS deltas. Hue is in degrees, L/S in [-1, +1]. */
typedef struct {
    double dH; /* hue rotation, degrees */
    double dL; /* lightness offset */
    double dS; /* saturation scale offset (multiplicative bias toward 0) */
} weather_mod;

static weather_mod modifier_for(hk_weather_cond c)
{
    weather_mod m = {0.0, 0.0, 0.0};
    switch (c) {
    case HK_WEATHER_CLEAR:  m.dL =  0.10;                                  break;
    case HK_WEATHER_CLOUDY: m.dL = -0.05; m.dS = -0.45;                    break;
    case HK_WEATHER_RAIN:   m.dH = +18.0; m.dL = -0.05; m.dS = -0.30;      break;
    case HK_WEATHER_SNOW:   m.dL =  0.20; m.dS = -0.30;                    break;
    case HK_WEATHER_STORM:  m.dH = +25.0; m.dL = -0.18; m.dS = -0.40;      break;
    case HK_WEATHER_FOG:    m.dL =  0.08; m.dS = -0.65;                    break;
    case HK_WEATHER_UNKNOWN:
    default: break;
    }
    return m;
}

static hk_rgb apply_mod(hk_rgb in, weather_mod m)
{
    double h, l, s;
    rgb_to_hls(in.r / 255.0, in.g / 255.0, in.b / 255.0, &h, &l, &s);
    h = fmod(h + m.dH / 360.0, 1.0);
    if (h < 0) h += 1.0;
    l += m.dL;
    if (l < 0) l = 0;
    if (l > 1) l = 1;
    /* dS is additive bias; clamp to [0, 1]. Negative dS pushes toward grey. */
    s += m.dS;
    if (s < 0) s = 0;
    if (s > 1) s = 1;
    double r, g, b;
    hls_to_rgb(h, l, s, &r, &g, &b);
    hk_rgb out;
    long ir = (long)(r * 255.0 + 0.5);
    long ig = (long)(g * 255.0 + 0.5);
    long ib = (long)(b * 255.0 + 0.5);
    if (ir < 0) ir = 0; if (ir > 255) ir = 255;
    if (ig < 0) ig = 0; if (ig > 255) ig = 255;
    if (ib < 0) ib = 0; if (ib > 255) ib = 255;
    out.r = (uint8_t)ir; out.g = (uint8_t)ig; out.b = (uint8_t)ib;
    return out;
}

void hk_palette_from_weather(hk_season s, hk_weather_cond c,
                             hk_rgb out[HK_PALETTE_STOPS])
{
    if (!out) return;
    int si = (int)s;
    if (si < 0 || si > 3) si = 0;
    weather_mod m = modifier_for(c);
    for (int i = 0; i < HK_PALETTE_STOPS; ++i) {
        out[i] = apply_mod(WEATHER_SEASONS[si][i], m);
    }
}
