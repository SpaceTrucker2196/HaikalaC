/* Smoke tests covering the deterministic parts of the pipeline:
 * haiku lookup, glyph table, spec construction, grid invariants.
 * The animation loop is not tested here — it's interactive. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "haikala.h"

#define ASSERT(cond) do {                               \
    if (!(cond)) {                                      \
        fprintf(stderr, "FAIL %s:%d: %s\n",             \
                __FILE__, __LINE__, #cond);             \
        ++failures;                                     \
    }                                                   \
} while (0)

static int failures = 0;

static void test_haiku_lookup(void)
{
    ASSERT(hk_haiku_count >= 5);
    const hk_haiku *h = hk_haiku_get("old_pond");
    ASSERT(h != NULL);
    ASSERT(strcmp(h->id, "old_pond") == 0);
    ASSERT(strcmp(h->author, "Matsuo Bashō") == 0);
    ASSERT(h->n_tokens >= 3);
    ASSERT(hk_haiku_get("does-not-exist") == NULL);
}

static void test_glyphs_for_known_token(void)
{
    size_t n = 0;
    const char *const *g = hk_glyphs_for("frog", &n);
    ASSERT(g != NULL);
    ASSERT(n >= 1);
    ASSERT(strlen(g[0]) > 0);

    const char *const *missing = hk_glyphs_for("xyzzy", &n);
    ASSERT(missing == NULL);
}

static void test_is_emoji_and_wide(void)
{
    ASSERT(hk_is_emoji("🐸") == true);
    ASSERT(hk_is_emoji("·")  == false);
    ASSERT(hk_is_emoji("❀")  == false);
    ASSERT(hk_is_wide("🐸")  == true);
    ASSERT(hk_is_wide("·")   == false);
}

static void render_at(const hk_spec *spec, double breath, hk_grid *g)
{
    hk_render_params rp;
    hk_render_params_default(&rp);
    rp.breath = breath;
    hk_render_spec(spec, &rp, g);
}

static void test_haiku_to_spec_basic(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    ASSERT(h != NULL);
    hk_spec spec;
    bool ok = hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, false, &spec);
    ASSERT(ok);
    ASSERT(spec.fold == 8);
    ASSERT(spec.grid_radius == HK_SIZE_MEDIUM);
    ASSERT(spec.n_rings == h->n_tokens);
    ASSERT(spec.center_glyph[0] != '\0');
}

static void test_haiku_to_spec_rejects_invalid_fold(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_spec spec;
    ASSERT(!hk_haiku_to_spec(h, 3, HK_SIZE_MEDIUM, false, &spec));
    ASSERT(!hk_haiku_to_spec(h, 18, HK_SIZE_MEDIUM, false, &spec));
    ASSERT(!hk_haiku_to_spec(h, 8, 1, false, &spec));
}

static void test_no_emoji_strips_emoji(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_spec spec;
    ASSERT(hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, true, &spec));
    ASSERT(!hk_is_emoji(spec.center_glyph));
    for (size_t i = 0; i < spec.n_rings; ++i) {
        for (size_t j = 0; j < spec.rings[i].n_glyphs; ++j) {
            ASSERT(!hk_is_emoji(spec.rings[i].glyphs[j]));
        }
    }
    /* In no-emoji mode every ring carries a bg color. */
    for (size_t i = 0; i < spec.n_rings; ++i) {
        ASSERT(spec.rings[i].has_bg);
    }
}

static void test_render_centers_bindu(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_spec spec;
    ASSERT(hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, false, &spec));
    hk_grid *g = hk_grid_new(HK_SIZE_MEDIUM);
    ASSERT(g != NULL);
    render_at(&spec, 0.0, g);
    int cy = g->height / 2, cx = g->width / 2;
    ASSERT(g->cells[cy * g->width + cx].glyph[0] != '\0');
    ASSERT(g->cells[cy * g->width + cx].has_fg == true);
    hk_grid_free(g);
}

static void test_render_for_every_haiku(void)
{
    for (size_t i = 0; i < hk_haiku_count; ++i) {
        const hk_haiku *h = &hk_haiku_table[i];
        hk_spec spec;
        if (!hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, false, &spec)) {
            fprintf(stderr, "FAIL: spec build for %s\n", h->id);
            ++failures;
            continue;
        }
        hk_grid *g = hk_grid_new(HK_SIZE_MEDIUM);
        if (!g) { ++failures; continue; }
        render_at(&spec, 0.0, g);
        int cy = g->height / 2, cx = g->width / 2;
        if (g->cells[cy * g->width + cx].glyph[0] == '\0') {
            fprintf(stderr, "FAIL: %s has empty bindu\n", h->id);
            ++failures;
        }
        hk_grid_free(g);
    }
}

static void test_breath_modulates_glyph_count(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_spec spec;
    ASSERT(hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, false, &spec));
    hk_grid *g = hk_grid_new(HK_SIZE_MEDIUM);
    ASSERT(g != NULL);

    int counts[3] = {0, 0, 0};
    double breaths[3] = {-1.0, 0.0, 1.0};
    for (int b = 0; b < 3; ++b) {
        render_at(&spec, breaths[b], g);
        int n = 0;
        for (int i = 0; i < g->width * g->height; ++i) {
            if (g->cells[i].glyph[0] != '\0' && !g->cells[i].covered) ++n;
        }
        counts[b] = n;
    }
    ASSERT(counts[2] >= counts[0]);
    hk_grid_free(g);
}

static void test_fold_for_haiku_in_range(void)
{
    for (size_t i = 0; i < hk_haiku_count; ++i) {
        int f = hk_fold_for_haiku(&hk_haiku_table[i]);
        ASSERT(f >= 4 && f <= 16);
        ASSERT(f % 2 == 0);
    }
}

static void test_palette_from_haiku(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_rgb pal[HK_PALETTE_STOPS];
    ASSERT(hk_palette_from_haiku(h, pal));
    /* Should be monotonic ascending in luminance. */
    double prev = -1.0;
    for (int i = 0; i < HK_PALETTE_STOPS; ++i) {
        double lum = 0.299 * pal[i].r + 0.587 * pal[i].g + 0.114 * pal[i].b;
        ASSERT(lum + 1e-6 >= prev);
        prev = lum;
    }
}

static void test_named_palette_lookup(void)
{
    ASSERT(hk_palette_id_from_name("aurora") == HK_PAL_AURORA);
    ASSERT(hk_palette_id_from_name("ocean")  == HK_PAL_OCEAN);
    /* unknown name → < 0 (cast to int because enum may be unsigned). */
    ASSERT((int)hk_palette_id_from_name("xyzzy") < 0);
    const hk_rgb *p = hk_palette_named(HK_PAL_AURORA);
    ASSERT(p != NULL);
}

static void test_hue_rotation_round_trip(void)
{
    hk_rgb red = {0xff, 0x00, 0x00};
    hk_rgb same = hk_rotate_hue(red, 0.0);
    ASSERT(same.r == 0xff && same.g == 0x00 && same.b == 0x00);
    hk_rgb full = hk_rotate_hue(red, 360.0);
    /* Allow tiny rounding. */
    ASSERT(full.r >= 0xfd);
    ASSERT(full.b <= 0x02);
}

static void test_weather_classifier_via_palette(void)
{
    /* Each season + condition pair must produce a valid 8-stop ramp.
     * We don't hit the network — just exercise the palette generator. */
    hk_rgb pal[HK_PALETTE_STOPS];
    hk_season seasons[] = {HK_SEASON_SPRING, HK_SEASON_SUMMER,
                           HK_SEASON_AUTUMN, HK_SEASON_WINTER};
    hk_weather_cond conds[] = {
        HK_WEATHER_CLEAR, HK_WEATHER_CLOUDY, HK_WEATHER_RAIN,
        HK_WEATHER_SNOW, HK_WEATHER_STORM, HK_WEATHER_FOG,
        HK_WEATHER_UNKNOWN,
    };
    for (size_t s = 0; s < sizeof(seasons)/sizeof(seasons[0]); ++s) {
        for (size_t c = 0; c < sizeof(conds)/sizeof(conds[0]); ++c) {
            memset(pal, 0xee, sizeof(pal));
            hk_palette_from_weather(seasons[s], conds[c], pal);
            /* Ramp must be monotonic non-decreasing in luminance — that's
             * how we built the season anchors. */
            double prev = -1.0;
            for (int i = 0; i < HK_PALETTE_STOPS; ++i) {
                double lum = 0.299*pal[i].r + 0.587*pal[i].g + 0.114*pal[i].b;
                ASSERT(lum + 0.5 >= prev);
                prev = lum;
            }
        }
    }
    /* Season-from-clock must be one of the four. */
    int s = (int)hk_season_now();
    ASSERT(s >= 0 && s <= 3);
}

static void test_no_emoji_bg_does_not_bleed_across_empty_cells(void)
{
    /* Regression: with bg colors set on adjacent cells separated by an
     * empty one, the emitter must reset SGR before the empty cell so
     * the bg doesn't extend through it as a horizontal bar. */
    hk_grid *g = hk_grid_new(HK_SIZE_SMALL);  /* 15 wide × 7 tall */
    ASSERT(g != NULL);
    /* Place X at column 2 row 3, leave 3 empty, place Y at column 6.
     * Both carry the same bg color. */
    int row = 3;
    hk_cell *cx = &g->cells[row * g->width + 2];
    hk_cell *cy = &g->cells[row * g->width + 6];
    snprintf(cx->glyph, HK_MAX_GLYPH_BYTES, "%s", "X");
    cx->fg = (hk_rgb){0xff, 0xff, 0xff}; cx->has_fg = true;
    cx->bg = (hk_rgb){0x40, 0x80, 0xc0}; cx->has_bg = true;
    snprintf(cy->glyph, HK_MAX_GLYPH_BYTES, "%s", "Y");
    cy->fg = (hk_rgb){0xff, 0xff, 0xff}; cy->has_fg = true;
    cy->bg = (hk_rgb){0x40, 0x80, 0xc0}; cy->has_bg = true;

    char buf[4096];
    hk_grid_to_ansi(g, 0.0, NULL, NULL, buf, sizeof(buf));

    /* The X must be followed by a reset (\x1b[0m) before any bare
     * spaces, otherwise the bg will bleed across to Y. */
    const char *p = strstr(buf, "X");
    ASSERT(p != NULL);
    /* Skip past the X. */
    ++p;
    ASSERT(*p == '\x1b');
    /* Find the next non-space, non-escape character; it should be 'Y'
     * (preceded somewhere by a fresh SGR). */
    /* Easier: confirm the substring "X\x1b[0m" appears. */
    ASSERT(strstr(buf, "X\x1b[0m") != NULL);
    /* Confirm the substring "Y\x1b[0m" appears too (end-of-row reset). */
    ASSERT(strstr(buf, "Y\x1b[0m") != NULL);

    hk_grid_free(g);
}

static void test_render_with_fractal_runs(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_spec spec;
    ASSERT(hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, false, &spec));
    hk_grid *g = hk_grid_new(HK_SIZE_MEDIUM);
    ASSERT(g != NULL);
    hk_render_params rp;
    hk_render_params_default(&rp);
    rp.fractal = true;
    rp.fractal_colors = hk_palette_named(HK_PAL_OCEAN);
    rp.spin = true;
    rp.ripple = true;
    rp.vary_breath = true;
    rp.t = 1.5;
    hk_render_spec(&spec, &rp, g);
    int cy = g->height / 2, cx = g->width / 2;
    ASSERT(g->cells[cy * g->width + cx].glyph[0] != '\0');
    hk_grid_free(g);
}

static void test_backdrop_dimensions(void)
{
    hk_grid *bg = hk_backdrop_new(80, 24, 0xCAFEBABE);
    ASSERT(bg != NULL);
    ASSERT(bg->width == 80);
    ASSERT(bg->height == 24);
    /* At least some non-empty static cells. */
    int non_empty = 0;
    for (int i = 0; i < bg->width * bg->height; ++i) {
        if (bg->cells[i].glyph[0] != '\0') {
            ++non_empty;
            ASSERT(bg->cells[i].is_static == true);
        }
    }
    ASSERT(non_empty > 0);
    hk_grid_free(bg);
}

int main(void)
{
    test_haiku_lookup();
    test_glyphs_for_known_token();
    test_is_emoji_and_wide();
    test_haiku_to_spec_basic();
    test_haiku_to_spec_rejects_invalid_fold();
    test_no_emoji_strips_emoji();
    test_render_centers_bindu();
    test_render_for_every_haiku();
    test_breath_modulates_glyph_count();
    test_backdrop_dimensions();
    test_fold_for_haiku_in_range();
    test_palette_from_haiku();
    test_named_palette_lookup();
    test_hue_rotation_round_trip();
    test_render_with_fractal_runs();
    test_weather_classifier_via_palette();
    test_no_emoji_bg_does_not_bleed_across_empty_cells();

    if (failures) {
        printf("FAILED: %d failures\n", failures);
        return 1;
    }
    printf("OK: all tests passed\n");
    return 0;
}
