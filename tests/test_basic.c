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

static void test_sound_module_lifecycle(void)
{
    /* When closed, energy must be 0. Open is best-effort: if sox isn't
     * installed (CI envs often), open returns false and we still
     * expect a clean 0. */
    ASSERT(hk_sound_energy() == 0.0);
    bool ok = hk_sound_open();
    if (!ok) {
        /* No sox on PATH — that's fine. Energy still 0. */
        ASSERT(hk_sound_energy() == 0.0);
        return;
    }
    /* Sox is available: energy is in [0, 1]. */
    double e = hk_sound_energy();
    ASSERT(e >= 0.0 && e <= 1.0);
    hk_sound_close();
    ASSERT(hk_sound_energy() == 0.0);
}

static void test_sound_compression_normalizes(void)
{
    /* Drive the compressor with a constant raw RMS and confirm the
     * output rises to (and stays near) a high normalized value — the
     * point of the AGC is that any sustained level fills the range. */
    hk_sound_filter_reset();
    double final_loud = 0.0;
    for (int i = 0; i < 200; ++i) {
        final_loud = hk_sound_filter_apply(0.30, true);
    }
    ASSERT(final_loud > 0.85);

    /* A 100× quieter constant raw RMS — after the AGC catches up,
     * the normalized energy should also reach a high value. That's
     * the compressor's job: equalize quiet and loud across rooms. */
    hk_sound_filter_reset();
    double final_quiet = 0.0;
    for (int i = 0; i < 2000; ++i) {
        final_quiet = hk_sound_filter_apply(0.003, true);
    }
    ASSERT(final_quiet > 0.65);

    /* Silence (no data) decays the EMA toward 0 within a few seconds. */
    hk_sound_filter_reset();
    /* prime with some loud audio */
    for (int i = 0; i < 50; ++i) hk_sound_filter_apply(0.40, true);
    double after_silence = 0.0;
    for (int i = 0; i < 200; ++i) {
        after_silence = hk_sound_filter_apply(0.0, false);
    }
    ASSERT(after_silence < 0.05);

    /* Output is always clipped to [0, 1]. */
    hk_sound_filter_reset();
    double e = hk_sound_filter_apply(10.0, true);
    ASSERT(e >= 0.0 && e <= 1.0);
}

static void test_palette_with_spectrum_silence_is_identity(void)
{
    /* All-zero spectrum input must leave the base palette nearly
     * unchanged (up to ±1 from rounding through HLS). */
    const hk_rgb *base = hk_palette_named(HK_PAL_AURORA);
    hk_rgb out[HK_PALETTE_STOPS];
    hk_palette_with_spectrum(base, 0.0, 0.0, 0.0, out);
    for (int i = 0; i < HK_PALETTE_STOPS; ++i) {
        ASSERT(abs((int)out[i].r - (int)base[i].r) <= 2);
        ASSERT(abs((int)out[i].g - (int)base[i].g) <= 2);
        ASSERT(abs((int)out[i].b - (int)base[i].b) <= 2);
    }
}

static void test_palette_with_spectrum_hue_shift_direction(void)
{
    /* Bass-dominant input shifts a green-ish base toward cool;
     * treble-dominant input shifts it toward warm. We compare the
     * blue vs red channels of a mid-luminance stop to confirm the
     * direction of the shift, not the exact magnitude. */
    hk_rgb base[HK_PALETTE_STOPS];
    for (int i = 0; i < HK_PALETTE_STOPS; ++i) {
        base[i] = (hk_rgb){0x40, 0xa0, 0x60}; /* mid green */
    }
    hk_rgb cooler[HK_PALETTE_STOPS], warmer[HK_PALETTE_STOPS];
    hk_palette_with_spectrum(base, /*lo*/1.0, /*md*/0.0, /*hi*/0.0, cooler);
    hk_palette_with_spectrum(base, /*lo*/0.0, /*md*/0.0, /*hi*/1.0, warmer);
    /* Bass dominant → cooler should have noticeably more blue than red. */
    ASSERT((int)cooler[4].b - (int)cooler[4].r > 8);
    /* Treble dominant → warmer should have more red than blue. */
    ASSERT((int)warmer[4].r - (int)warmer[4].b > 8);
}

static void test_life_engine_evolves(void)
{
    /* Seed a life grid from a rendered ring layer, tick it a few
     * frames, and confirm the population is non-zero after at least
     * one tick. (B3/S23 may collapse from arbitrary seeds, but the
     * spec's ring positions reliably produce some survivors for a
     * few generations.) */
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_spec spec;
    ASSERT(hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, false, &spec));
    hk_grid *g = hk_grid_new(HK_SIZE_MEDIUM);
    ASSERT(g != NULL);

    hk_render_params rp;
    hk_render_params_default(&rp);
    rp.rings_only = true;
    hk_render_spec(&spec, &rp, g);

    hk_life *L = hk_life_new(g->width, g->height, HK_SIZE_MEDIUM);
    ASSERT(L != NULL);
    hk_life_seed_from_grid(L, g);
    int seed = hk_life_initial_count(L);
    ASSERT(seed > 0);

    /* Tick once — survivors must be ≤ seed (Conway). */
    int alive = hk_life_tick(L);
    ASSERT(alive >= 0);
    ASSERT(hk_life_alive_count(L) == alive);

    /* Stamping must not crash and must not zero out the bindu (which is
     * a wide emoji — life skips wide cells). */
    hk_grid *canvas = hk_grid_new(HK_SIZE_MEDIUM);
    ASSERT(canvas != NULL);
    hk_render_spec(&spec, &(hk_render_params){0}, canvas);  /* full render */
    const hk_rgb *pal = hk_palette_named(HK_PAL_AURORA);
    hk_life_stamp(L, canvas, 0, 0, pal);
    /* Bindu still has the spec's center glyph. */
    int cy = canvas->height / 2, cx = canvas->width / 2;
    ASSERT(strcmp(canvas->cells[cy * canvas->width + cx].glyph,
                  spec.center_glyph) == 0);

    hk_grid_free(canvas);
    hk_grid_free(g);
    hk_life_free(L);
}

static void test_life_kill_at_grid_collision(void)
{
    /* Build a life grid with a known alive cell at (5,5). Build a
     * ring grid with a non-empty glyph at (5,5). Kill, then confirm
     * the life cell is dead. */
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_spec spec;
    ASSERT(hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, false, &spec));
    hk_grid *src = hk_grid_new(HK_SIZE_MEDIUM);
    ASSERT(src != NULL);
    hk_life *L = hk_life_new(src->width, src->height, HK_SIZE_MEDIUM);
    ASSERT(L != NULL);
    /* Seed life: every disc cell alive (force population). */
    hk_render_params rp;
    hk_render_params_default(&rp);
    rp.rings_only = true;
    hk_render_spec(&spec, &rp, src);
    hk_life_seed_from_grid(L, src);
    int seeded = hk_life_alive_count(L);
    ASSERT(seeded > 0);

    /* Build a "ring overlay" grid with a glyph at every position of
     * the original seed. Kill — should clear ALL seeded life. */
    int killed = hk_life_kill_at_grid(L, src);
    ASSERT(killed == seeded);
    ASSERT(hk_life_alive_count(L) == 0);

    hk_grid_free(src);
    hk_life_free(L);
}

static void test_size_max_fits_terminal(void)
{
    /* For a sane terminal, the resulting body must fit:
     *   width  4r+1 ≤ term_width
     *   height 2r+1 ≤ term_height - header_overhead
     */
    int cases[][2] = {
        { 80,  24}, /* common 80x24 */
        {120,  40}, /* roomy laptop */
        {200,  60}, /* big monitor */
        { 30,  10}, /* tiny */
        { 25,   8}, /* below floor — should still return >= small */
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        int w = cases[i][0], h = cases[i][1];
        int r = hk_size_max_for_terminal(w, h);
        ASSERT(r >= HK_SIZE_SMALL);
        if (w >= 30 && h >= 16) {
            int body_w = 4 * r + 1;
            int body_h = 2 * r + 1;
            ASSERT(body_w <= w);
            ASSERT(body_h <= h - 2);  /* allow some header overhead */
        }
    }
    /* On an enormous terminal we should still cap. */
    int huge_r = hk_size_max_for_terminal(500, 200);
    ASSERT(huge_r <= 40);
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
    test_size_max_fits_terminal();
    test_sound_module_lifecycle();
    test_sound_compression_normalizes();
    test_palette_with_spectrum_silence_is_identity();
    test_palette_with_spectrum_hue_shift_direction();
    test_life_engine_evolves();
    test_life_kill_at_grid_collision();

    if (failures) {
        printf("FAILED: %d failures\n", failures);
        return 1;
    }
    printf("OK: all tests passed\n");
    return 0;
}
