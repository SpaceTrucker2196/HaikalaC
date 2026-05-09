/* Cell grid → ANSI escape sequences. Truecolor (24-bit) only — every
 * mainstream terminal in use today supports it. Hue rotation is
 * applied per cell using a tiny (color, 5°-bucket) cache so position-
 * dependent shifts (--emanate) stay fast.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "haikala.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int append(char *buf, size_t bufsize, size_t pos, const char *s)
{
    if (!s) return 0;
    size_t len = strlen(s);
    if (pos + len + 1 > bufsize) return -1;
    memcpy(buf + pos, s, len);
    buf[pos + len] = '\0';
    return (int)len;
}

/* ---- Cache for rotated colors keyed by (r,g,b,bucket) ------------ */

#define CACHE_BUCKETS 72  /* 5° quantization */
#define CACHE_SLOTS   16  /* per bucket; small linear scan */

typedef struct {
    uint8_t in_r, in_g, in_b;
    uint8_t out_r, out_g, out_b;
    bool    used;
} cache_slot;

typedef struct {
    cache_slot slots[CACHE_BUCKETS][CACHE_SLOTS];
} hue_cache;

static void cache_clear(hue_cache *c)
{
    memset(c, 0, sizeof(*c));
}

static hk_rgb cache_get(hue_cache *c, hk_rgb in, double degrees)
{
    int bucket = ((int)floor(degrees / 5.0 + 0.5)) % CACHE_BUCKETS;
    if (bucket < 0) bucket += CACHE_BUCKETS;
    cache_slot *slots = c->slots[bucket];
    for (int i = 0; i < CACHE_SLOTS; ++i) {
        if (slots[i].used &&
            slots[i].in_r == in.r &&
            slots[i].in_g == in.g &&
            slots[i].in_b == in.b) {
            hk_rgb out = {slots[i].out_r, slots[i].out_g, slots[i].out_b};
            return out;
        }
    }
    hk_rgb out = hk_rotate_hue(in, bucket * 5.0);
    /* find first empty slot, else replace slot 0. */
    int idx = 0;
    for (int i = 0; i < CACHE_SLOTS; ++i) {
        if (!slots[i].used) { idx = i; break; }
    }
    slots[idx].used = true;
    slots[idx].in_r = in.r; slots[idx].in_g = in.g; slots[idx].in_b = in.b;
    slots[idx].out_r = out.r; slots[idx].out_g = out.g; slots[idx].out_b = out.b;
    return out;
}

/* ---- emit one cell's SGR ----------------------------------------- */

static size_t emit_sgr(char *buf, size_t bufsize, size_t pos,
                       const hk_cell *cell, double shift,
                       hue_cache *cache)
{
    char body[160];
    size_t n = 0;
    body[0] = '\0';

    /* style */
    if (cell->style & HK_STYLE_DIM) {
        n += (size_t)snprintf(body + n, sizeof(body) - n, "%s2",
                              n ? ";" : "");
    }
    if (cell->style & HK_STYLE_BOLD) {
        n += (size_t)snprintf(body + n, sizeof(body) - n, "%s1",
                              n ? ";" : "");
    }
    if (cell->style & HK_STYLE_ITALIC) {
        n += (size_t)snprintf(body + n, sizeof(body) - n, "%s3",
                              n ? ";" : "");
    }
    /* fg */
    if (cell->has_fg) {
        hk_rgb fg = cell->fg;
        if (shift && !cell->is_static) fg = cache_get(cache, fg, shift);
        n += (size_t)snprintf(body + n, sizeof(body) - n,
                              "%s38;2;%u;%u;%u",
                              n ? ";" : "",
                              (unsigned)fg.r, (unsigned)fg.g, (unsigned)fg.b);
    }
    /* bg */
    if (cell->has_bg) {
        hk_rgb bg = cell->bg;
        if (shift && !cell->is_static) bg = cache_get(cache, bg, shift);
        n += (size_t)snprintf(body + n, sizeof(body) - n,
                              "%s48;2;%u;%u;%u",
                              n ? ";" : "",
                              (unsigned)bg.r, (unsigned)bg.g, (unsigned)bg.b);
    }
    if (n == 0) {
        int r = append(buf, bufsize, pos, "\x1b[0m");
        return (r > 0) ? (size_t)r : 0;
    }
    char esc[200];
    int len = snprintf(esc, sizeof(esc), "\x1b[0;%sm", body);
    if (len < 0) return 0;
    int r = append(buf, bufsize, pos, esc);
    return (r > 0) ? (size_t)r : 0;
}

/* ---- main entry -------------------------------------------------- */

size_t hk_grid_to_ansi(const hk_grid *g,
                       double hue_shift,
                       hk_hue_field_fn hue_field,
                       void *hue_field_user,
                       char *buf, size_t bufsize)
{
    if (!g || !buf || bufsize == 0) return 0;
    size_t pos = 0;
    if (bufsize > 0) buf[0] = '\0';

    hue_cache cache;
    cache_clear(&cache);

    for (int y = 0; y < g->height; ++y) {
        for (int x = 0; x < g->width; ++x) {
            const hk_cell *c = &g->cells[y * g->width + x];
            if (c->covered) continue;
            if (c->glyph[0] == '\0') {
                int r = append(buf, bufsize, pos, " ");
                if (r < 0) return pos;
                pos += (size_t)r;
                continue;
            }
            double shift = 0.0;
            if (!c->is_static) {
                shift = hue_shift;
                if (hue_field) {
                    shift += hue_field(x, y, hue_field_user);
                }
            }
            size_t sgr = emit_sgr(buf, bufsize, pos, c, shift, &cache);
            pos += sgr;
            int r = append(buf, bufsize, pos, c->glyph);
            if (r < 0) return pos;
            pos += (size_t)r;
        }
        int r = append(buf, bufsize, pos, "\x1b[0m");
        if (r > 0) pos += (size_t)r;
        if (y < g->height - 1) {
            r = append(buf, bufsize, pos, "\r\n");
            if (r < 0) return pos;
            pos += (size_t)r;
        }
    }
    return pos;
}

/* ---- emanate hue field ------------------------------------------- */

void hk_emanate_default(hk_emanate *e)
{
    if (!e) return;
    e->cx = 0; e->cy = 0;
    e->t = 0.0;
    e->period = 5.0;
    e->max_r = 20.0;
    e->folds[0] = 2; e->folds[1] = 4; e->folds[2] = 6;
    e->folds[3] = 8; e->folds[4] = 12;
    e->n_folds = 5;
    e->band = 4.0;
    e->intensity = 140.0;
}

double hk_emanate_at(int x, int y, void *user)
{
    const hk_emanate *e = (const hk_emanate *)user;
    if (!e || e->max_r <= 0 || e->n_folds <= 0) return 0.0;
    double period = e->period > 0.25 ? e->period : 0.25;
    double cycle = e->t / period;
    int wave_idx = ((int)floor(cycle)) % e->n_folds;
    if (wave_idx < 0) wave_idx += e->n_folds;
    int fold = e->folds[wave_idx];
    double phase = cycle - floor(cycle);
    double wave_r = phase * e->max_r;

    double rx = (x - e->cx) / 2.0;
    double ry = (double)(y - e->cy);
    double r = sqrt(rx * rx + ry * ry);
    double d = fabs(r - wave_r);
    if (d > e->band) return 0.0;
    double falloff = 1.0 - d / e->band;
    if (r < 0.5) return falloff * e->intensity;
    double theta = atan2(ry, rx);
    double mod = (1.0 + cos(fold * theta)) / 2.0;
    return falloff * mod * e->intensity;
}
