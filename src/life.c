/* Conway-style life automaton overlay.
 *
 * A 2D bool grid (stored as uint8_t age values) sized to the mandala
 * body. Each frame the engine runs one B3/S23 step — alive cells with
 * 2 or 3 neighbors survive, dead cells with exactly 3 neighbors are
 * born. The age counter increments on each survival so the renderer
 * can dim older cells and pick different glyphs per generation.
 *
 * The simulation is restricted to cells inside the disc (Euclidean
 * distance from center ≤ grid_radius + 0.5) so the visualization
 * keeps the circular mandala silhouette. Births and deaths outside
 * the disc are ignored.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "haikala.h"

struct hk_life {
    int       width;
    int       height;
    int       grid_radius;
    uint8_t  *cells;  /* 0 = dead, >0 = age (frames alive, capped at 255) */
    uint8_t  *next;
    uint8_t  *in_disc; /* precomputed mask: 1 if cell is inside the disc */
    int       initial_alive;
};

hk_life *hk_life_new(int width, int height, int grid_radius)
{
    if (width <= 0 || height <= 0) return NULL;
    hk_life *L = (hk_life *)calloc(1, sizeof(*L));
    if (!L) return NULL;
    L->width = width;
    L->height = height;
    L->grid_radius = grid_radius;
    size_t n = (size_t)width * (size_t)height;
    L->cells   = (uint8_t *)calloc(n, 1);
    L->next    = (uint8_t *)calloc(n, 1);
    L->in_disc = (uint8_t *)calloc(n, 1);
    if (!L->cells || !L->next || !L->in_disc) {
        hk_life_free(L);
        return NULL;
    }
    /* Precompute disc mask. x is half-weighted because terminal cells
     * are roughly 2:1 (height:width). */
    int cx = width / 2, cy = height / 2;
    double edge = grid_radius + 0.5;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double rx = (x - cx) / 2.0;
            double ry = (double)(y - cy);
            if (sqrt(rx * rx + ry * ry) <= edge) {
                L->in_disc[y * width + x] = 1;
            }
        }
    }
    return L;
}

void hk_life_free(hk_life *L)
{
    if (!L) return;
    free(L->cells);
    free(L->next);
    free(L->in_disc);
    free(L);
}

void hk_life_seed_from_grid(hk_life *L, const hk_grid *src)
{
    if (!L || !src) return;
    if (src->width != L->width || src->height != L->height) return;
    memset(L->cells, 0, (size_t)L->width * (size_t)L->height);
    int alive = 0;
    for (int y = 0; y < L->height; ++y) {
        for (int x = 0; x < L->width; ++x) {
            int idx = y * L->width + x;
            if (!L->in_disc[idx]) continue;
            const hk_cell *c = &src->cells[idx];
            if (c->covered) continue;
            if (c->glyph[0] == '\0' || (c->glyph[0] == ' ' && c->glyph[1] == '\0')) {
                continue;
            }
            L->cells[idx] = 1;  /* freshly born */
            ++alive;
        }
    }
    L->initial_alive = alive;
}

int hk_life_tick(hk_life *L)
{
    if (!L) return 0;
    int w = L->width, h = L->height;
    int alive_next = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            if (!L->in_disc[idx]) {
                L->next[idx] = 0;
                continue;
            }
            int n = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                int ny = y + dy;
                if (ny < 0 || ny >= h) continue;
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx;
                    if (nx < 0 || nx >= w) continue;
                    if (L->cells[ny * w + nx] > 0) ++n;
                }
            }
            uint8_t cur = L->cells[idx];
            uint8_t nv  = 0;
            if (cur > 0) {
                if (n == 2 || n == 3) {
                    /* survives — age up, cap at 255 */
                    nv = (cur < 255) ? (uint8_t)(cur + 1) : 255;
                }
            } else {
                if (n == 3) nv = 1; /* born */
            }
            L->next[idx] = nv;
            if (nv > 0) ++alive_next;
        }
    }
    /* swap buffers */
    uint8_t *tmp = L->cells;
    L->cells = L->next;
    L->next  = tmp;
    return alive_next;
}

int hk_life_alive_count(const hk_life *L)
{
    if (!L) return 0;
    int n = 0;
    int total = L->width * L->height;
    for (int i = 0; i < total; ++i) if (L->cells[i] > 0) ++n;
    return n;
}

int hk_life_initial_count(const hk_life *L)
{
    return L ? L->initial_alive : 0;
}

/* Glyph repertoire by age. Fresh births are bright/dense; oldsters
 * fade to small dots. */
static const char *LIFE_GLYPHS[] = {
    "●", "●", "◉", "◯", "○", "◌", "·", "˙",
};
#define LIFE_GLYPH_N (sizeof(LIFE_GLYPHS) / sizeof(LIFE_GLYPHS[0]))

void hk_life_stamp(const hk_life *L,
                   hk_grid *dst,
                   int top, int left,
                   const hk_rgb palette[HK_PALETTE_STOPS])
{
    if (!L || !dst || !palette) return;
    for (int y = 0; y < L->height; ++y) {
        int dy = top + y;
        if (dy < 0 || dy >= dst->height) continue;
        for (int x = 0; x < L->width; ++x) {
            uint8_t age = L->cells[y * L->width + x];
            if (age == 0) continue;
            int dx = left + x;
            if (dx < 0 || dx >= dst->width) continue;
            hk_cell *c = &dst->cells[dy * dst->width + dx];
            /* Don't trample the haiku bindu (center glyph). The center
             * is at floor(dst->width/2), floor(dst->height/2) for
             * standalone grids — but inside our composited canvas the
             * center is offset. The simplest cheap test: skip if the
             * existing cell glyph looks "wide" (emoji-bindu candidate)
             * — keeps the heart of the haiku readable through the
             * life churn. */
            if (hk_is_wide(c->glyph)) continue;

            /* Pick glyph + color by age. Index saturates so very old
             * cells stay at the dimmest dot. */
            int gi = (age < LIFE_GLYPH_N) ? (age - 1) : (int)(LIFE_GLYPH_N - 1);
            if (gi < 0) gi = 0;
            /* Palette index: newer → stop 7 (brightest), older → toward
             * stop 1 (dim). Skip stop 0 (almost-black). */
            int pi = (int)(HK_PALETTE_STOPS - 1 - age);
            if (pi < 1) pi = 1;
            snprintf(c->glyph, HK_MAX_GLYPH_BYTES, "%s", LIFE_GLYPHS[gi]);
            c->fg = palette[pi];
            c->has_fg = true;
            c->style = (age > 6) ? HK_STYLE_DIM : HK_STYLE_NONE;
            c->is_static = false;
        }
    }
}
