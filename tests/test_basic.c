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

static void test_haiku_to_spec_basic(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    ASSERT(h != NULL);
    hk_spec spec;
    bool ok = hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, &spec);
    ASSERT(ok);
    ASSERT(spec.fold == 8);
    ASSERT(spec.grid_radius == HK_SIZE_MEDIUM);
    /* lotus throne + content rings == 1 + (n_tokens - 1) */
    ASSERT(spec.n_rings == h->n_tokens);
    ASSERT(spec.center_glyph[0] != '\0');
}

static void test_haiku_to_spec_rejects_invalid_fold(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_spec spec;
    ASSERT(!hk_haiku_to_spec(h, 3, HK_SIZE_MEDIUM, &spec));   /* odd */
    ASSERT(!hk_haiku_to_spec(h, 18, HK_SIZE_MEDIUM, &spec));  /* > 16 */
    ASSERT(!hk_haiku_to_spec(h, 8, 1, &spec));                /* radius too small */
}

static void test_render_centers_bindu(void)
{
    const hk_haiku *h = hk_haiku_get("old_pond");
    hk_spec spec;
    ASSERT(hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, &spec));
    hk_grid *g = hk_grid_new(HK_SIZE_MEDIUM);
    ASSERT(g != NULL);
    hk_render_spec(&spec, 0.0, g);
    int cy = g->height / 2, cx = g->width / 2;
    ASSERT(g->cells[cy * g->width + cx].glyph[0] != '\0');
    /* Center color should be set. */
    ASSERT(g->cells[cy * g->width + cx].has_fg == true);
    hk_grid_free(g);
}

static void test_render_for_every_haiku(void)
{
    for (size_t i = 0; i < hk_haiku_count; ++i) {
        const hk_haiku *h = &hk_haiku_table[i];
        hk_spec spec;
        if (!hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, &spec)) {
            fprintf(stderr, "FAIL: spec build for %s\n", h->id);
            ++failures;
            continue;
        }
        hk_grid *g = hk_grid_new(HK_SIZE_MEDIUM);
        if (!g) { ++failures; continue; }
        hk_render_spec(&spec, 0.0, g);
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
    ASSERT(hk_haiku_to_spec(h, 8, HK_SIZE_MEDIUM, &spec));
    hk_grid *g = hk_grid_new(HK_SIZE_MEDIUM);
    ASSERT(g != NULL);

    int counts[3] = {0, 0, 0};
    double breaths[3] = {-1.0, 0.0, 1.0};
    for (int b = 0; b < 3; ++b) {
        hk_render_spec(&spec, breaths[b], g);
        int n = 0;
        for (int i = 0; i < g->width * g->height; ++i) {
            if (g->cells[i].glyph[0] != '\0' && !g->cells[i].covered) ++n;
        }
        counts[b] = n;
    }
    /* Inhale should not place fewer glyphs than exhale. */
    ASSERT(counts[2] >= counts[0]);
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
    test_render_centers_bindu();
    test_render_for_every_haiku();
    test_breath_modulates_glyph_count();
    test_backdrop_dimensions();

    if (failures) {
        printf("FAILED: %d failures\n", failures);
        return 1;
    }
    printf("OK: all tests passed\n");
    return 0;
}
