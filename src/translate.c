/* Haiku → spec translation. Deterministic: same input always yields the
 * same spec. Mirrors the upstream Python algorithm:
 *   tokens[0]   → bindu (center glyph + center color)
 *   tokens[1..] → classified into inner/middle/outer bands
 *   each band  → one ring per token, radii spaced inside the band
 *   plus one fixed lotus throne ring just outside the bindu (Buddhist
 *   mandala detail).
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "haikala.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- season palettes (5 colors each, RGB) -------------------------- */

typedef struct {
    const char *season;
    hk_rgb      colors[5];
} season_palette;

/* Approximations of the upstream Rich color names. Truecolor RGB. */
static const season_palette PALETTES[] = {
    {"spring",  {{0xb8, 0xf0, 0x8a}, {0xff, 0xb0, 0xc8}, {0x88, 0xee, 0xa6},
                 {0xff, 0x90, 0xb0}, {0x66, 0xc8, 0x6e}}},
    {"summer",  {{0x90, 0xc8, 0xff}, {0x40, 0xa0, 0xff}, {0xff, 0xd0, 0x40},
                 {0xff, 0xee, 0x90}, {0x20, 0x90, 0xa8}}},
    {"autumn",  {{0xff, 0x88, 0x30}, {0xd8, 0x70, 0x2a}, {0xc8, 0x90, 0x70},
                 {0xd0, 0xa0, 0x60}, {0xf0, 0xd0, 0x70}}},
    {"winter",  {{0xf6, 0xf6, 0xff}, {0xb0, 0xc8, 0xe0}, {0x68, 0x90, 0xb0},
                 {0xc8, 0xc8, 0xc8}, {0x80, 0x90, 0xa0}}},
    {"new_year",{{0xe8, 0x30, 0x40}, {0xff, 0xc8, 0x40}, {0xff, 0x88, 0x30},
                 {0xff, 0x40, 0x80}, {0xff, 0xff, 0xff}}},
};

static const size_t N_PALETTES = sizeof(PALETTES) / sizeof(PALETTES[0]);

static const hk_rgb *palette_for_season(const char *season)
{
    for (size_t i = 0; i < N_PALETTES; ++i) {
        if (strcmp(PALETTES[i].season, season) == 0) {
            return PALETTES[i].colors;
        }
    }
    return PALETTES[0].colors; /* spring fallback */
}

/* ---- token band classification ------------------------------------- */

static const char *const SUBJECT_TOKENS[] = {
    "frog", "crow", "heron", "moon", "sun", "star", "bell", "temple",
    "blossom", "morning_glory", "warriors", "lightning",
};
static const char *const ACTION_TOKENS[] = {
    "splash", "stillness", "silence", "dream", "memory", "first",
    "echo", "awakening", "longing", "joy",
};

static bool token_in(const char *tok, const char *const *list, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(tok, list[i]) == 0) {
            return true;
        }
    }
    return false;
}

static hk_band band_for(const char *tok)
{
    if (token_in(tok, SUBJECT_TOKENS,
                 sizeof(SUBJECT_TOKENS) / sizeof(SUBJECT_TOKENS[0]))) {
        return HK_BAND_INNER;
    }
    if (token_in(tok, ACTION_TOKENS,
                 sizeof(ACTION_TOKENS) / sizeof(ACTION_TOKENS[0]))) {
        return HK_BAND_MIDDLE;
    }
    return HK_BAND_OUTER;
}

static void radius_band(int grid_radius, hk_band band, double *rmin, double *rmax)
{
    switch (band) {
    case HK_BAND_INNER:
        *rmin = 0.22 * grid_radius;
        *rmax = 0.38 * grid_radius;
        break;
    case HK_BAND_MIDDLE:
        *rmin = 0.46 * grid_radius;
        *rmax = 0.62 * grid_radius;
        break;
    case HK_BAND_OUTER:
    default:
        *rmin = 0.72 * grid_radius;
        *rmax = 0.94 * grid_radius;
        break;
    }
}

static double band_density(hk_band band)
{
    switch (band) {
    case HK_BAND_INNER:  return 1.00;
    case HK_BAND_MIDDLE: return 0.88;
    case HK_BAND_OUTER:
    default:             return 0.74;
    }
}

/* ---- main entry ---------------------------------------------------- */

bool hk_haiku_to_spec(const hk_haiku *h, int fold, int grid_radius, hk_spec *out)
{
    if (!h || !out || h->n_tokens == 0) {
        return false;
    }
    if (fold < 4 || fold > 16 || (fold % 2) != 0) {
        return false;
    }
    if (grid_radius < 5) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->fold        = fold;
    out->grid_radius = grid_radius;
    out->haiku_id    = h->id;

    const hk_rgb *palette = palette_for_season(h->season);
    out->center_color = palette[0];

    /* center glyph from tokens[0][0]. */
    size_t n_glyphs = 0;
    const char *const *gset = hk_glyphs_for(h->tokens[0], &n_glyphs);
    if (!gset || n_glyphs == 0) {
        return false;
    }
    snprintf(out->center_glyph, sizeof(out->center_glyph), "%s", gset[0]);

    /* ----- lotus throne — fixed Buddhist mandala detail ------------- */
    static const char *const lotus_glyphs[] = { "❀" };
    double lotus_r = 0.16 * grid_radius;
    if (lotus_r < 1.4) lotus_r = 1.4;

    out->rings[out->n_rings++] = (hk_ring){
        .radius   = lotus_r,
        .glyphs   = lotus_glyphs,
        .n_glyphs = 1,
        .density  = 1.0,
        .color    = palette[1 % 5],
        .band     = HK_BAND_INNER,
        .shape    = HK_SHAPE_CIRCLE,
        .phase    = M_PI / fold,
    };

    /* ----- content rings, banded ------------------------------------ */
    /* Iterate bands inner→middle→outer; tokens go where their band says. */
    int color_idx = 0;
    int ring_idx  = 0;
    const hk_rgb *cycle = palette + 1; /* 4 entries cycling, then back to 0 */

    for (int b = 0; b < 3; ++b) {
        hk_band band = (hk_band)b;
        /* Collect tokens in this band, in original order. */
        const char *band_tokens[16];
        size_t n_band = 0;
        for (size_t i = 1; i < h->n_tokens && n_band < 16; ++i) {
            if (band_for(h->tokens[i]) == band) {
                band_tokens[n_band++] = h->tokens[i];
            }
        }
        if (n_band == 0) {
            continue;
        }
        double rmin, rmax;
        radius_band(grid_radius, band, &rmin, &rmax);

        for (size_t i = 0; i < n_band && out->n_rings < 16; ++i) {
            const char *tok = band_tokens[i];
            size_t glyph_n = 0;
            const char *const *glyphs = hk_glyphs_for(tok, &glyph_n);
            if (!glyphs || glyph_n == 0) {
                continue;
            }
            double r;
            if (n_band == 1) {
                r = (rmin + rmax) * 0.5;
            } else {
                r = rmin + (rmax - rmin) * (double)i / (double)(n_band - 1);
            }
            hk_rgb color = (color_idx < 4) ? cycle[color_idx % 4] : palette[0];
            color_idx = (color_idx + 1) % 5;

            /* Shape per band: same cycling as upstream — inner polygon-ish,
             * middle petal-ish, outer star-ish. */
            hk_shape shape;
            switch (band) {
            case HK_BAND_INNER:
                shape = (ring_idx % 3 == 0) ? HK_SHAPE_POLYGON
                       : (ring_idx % 3 == 1) ? HK_SHAPE_CIRCLE
                                              : HK_SHAPE_STAR;
                break;
            case HK_BAND_MIDDLE:
                shape = (ring_idx % 3 == 0) ? HK_SHAPE_PETAL
                       : (ring_idx % 3 == 1) ? HK_SHAPE_CIRCLE
                                              : HK_SHAPE_POLYGON;
                break;
            case HK_BAND_OUTER:
            default:
                shape = (ring_idx % 3 == 0) ? HK_SHAPE_STAR
                       : (ring_idx % 3 == 1) ? HK_SHAPE_CIRCLE
                                              : HK_SHAPE_PETAL;
                break;
            }
            double phase = fmod((double)ring_idx * M_PI / fold,
                                2.0 * M_PI / fold);
            ring_idx++;

            out->rings[out->n_rings++] = (hk_ring){
                .radius   = r,
                .glyphs   = glyphs,
                .n_glyphs = glyph_n,
                .density  = band_density(band),
                .color    = color,
                .band     = band,
                .shape    = shape,
                .phase    = phase,
            };
        }
    }

    return true;
}
