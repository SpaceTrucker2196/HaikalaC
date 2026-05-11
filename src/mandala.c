/* Mandala geometry and grid composition. Ports the upstream Python
 * algorithms 1:1 — same radial layout, same fold-sector spacing, same
 * deterministic background fill. Produces a Cell grid sized to
 * (4*r+1, 2*r+1).
 *
 * Terminal cells are roughly 2:1 (tall:wide), so x is multiplied by 2
 * when placing positions on a circle, otherwise circles render as
 * flat ovals.
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

/* ---- grid lifecycle ------------------------------------------------ */

static hk_grid *grid_new(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return NULL;
    }
    hk_grid *g = (hk_grid *)calloc(1, sizeof(*g));
    if (!g) return NULL;
    g->width = width;
    g->height = height;
    g->cells = (hk_cell *)calloc((size_t)width * (size_t)height, sizeof(hk_cell));
    if (!g->cells) {
        free(g);
        return NULL;
    }
    return g;
}

hk_grid *hk_grid_new(int radius)
{
    int width  = 4 * radius + 1;
    int height = 2 * radius + 1;
    return grid_new(width, height);
}

void hk_grid_free(hk_grid *g)
{
    if (!g) return;
    free(g->cells);
    free(g);
}

void hk_grid_free_arbitrary(hk_grid *g)
{
    hk_grid_free(g);
}

void hk_grid_clear(hk_grid *g)
{
    if (!g) return;
    memset(g->cells, 0, (size_t)g->width * (size_t)g->height * sizeof(hk_cell));
}

/* ---- glyph width / emoji detection --------------------------------- */

bool hk_is_emoji(const char *glyph)
{
    if (!glyph) return false;
    /* VS-16 (U+FE0F): "render preceding char with emoji presentation" */
    const char *p = glyph;
    while (*p) {
        unsigned char c = (unsigned char)*p;
        if (c < 0x80) {
            ++p;
            continue;
        }
        /* Decode the leading bytes of one UTF-8 codepoint */
        uint32_t cp = 0;
        int len = 0;
        if ((c & 0xE0) == 0xC0)      { cp = c & 0x1F; len = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 3; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; len = 4; }
        else { return false; }
        for (int i = 1; i < len; ++i) {
            unsigned char cc = (unsigned char)p[i];
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (cp == 0xFE0F) return true;        /* VS-16 anywhere → emoji */
        if (cp >= 0x1F300) return true;       /* SMP emoji range */
        p += len;
    }
    return false;
}

bool hk_is_wide(const char *glyph)
{
    if (!glyph) return false;
    /* If it contains VS-16 or any codepoint >= 0x1F300, treat as wide. */
    const char *p = glyph;
    while (*p) {
        unsigned char c = (unsigned char)*p;
        if (c < 0x80) {
            ++p;
            continue;
        }
        uint32_t cp = 0;
        int len = 0;
        if ((c & 0xE0) == 0xC0)      { cp = c & 0x1F; len = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 3; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; len = 4; }
        else { return false; }
        for (int i = 1; i < len; ++i) {
            unsigned char cc = (unsigned char)p[i];
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (cp == 0xFE0F)                       return true;
        if (cp >= 0x1F300)                      return true;
        if (cp >= 0x2E80 && cp <= 0x9FFF)       return true; /* CJK */
        if (cp >= 0xAC00 && cp <= 0xD7A3)       return true; /* Hangul */
        if (cp >= 0xFF00 && cp <= 0xFF60)       return true; /* fullwidth */
        p += len;
    }
    return false;
}

/* ---- ring placement helpers ---------------------------------------- */

static int positions_per_sector(double radius, int fold)
{
    double raw = 2.6 * radius / fold + 0.5;
    int n = (int)floor(raw + 0.5);
    if (n < 1) n = 1;
    return n;
}

#define STAR_INNER_RATIO 0.55
#define PETAL_AMP        0.22

static double shape_radius(hk_shape shape, double R, int fold, double theta_local)
{
    double sector = 2.0 * M_PI / fold;
    double midpoint = M_PI / fold;
    switch (shape) {
    case HK_SHAPE_POLYGON: {
        double a = fmod(theta_local, sector);
        if (a < 0) a += sector;
        double edge = fabs(a - midpoint);
        double denom = cos(edge);
        if (denom < 1e-9) return R;
        return R * cos(midpoint) / denom;
    }
    case HK_SHAPE_STAR: {
        double a = fmod(theta_local, sector);
        if (a < 0) a += sector;
        double t = fabs(a - midpoint) / midpoint;
        return R * (STAR_INNER_RATIO + (1.0 - STAR_INNER_RATIO) * t);
    }
    case HK_SHAPE_PETAL:
        return R * (1.0 + PETAL_AMP * cos(fold * theta_local)) /
               (1.0 + PETAL_AMP);
    case HK_SHAPE_CIRCLE:
    default:
        return R;
    }
}

static bool place(hk_grid *g, int x, int y,
                  const char *glyph, hk_rgb color, uint8_t style,
                  bool has_color,
                  const hk_rgb *bg, bool has_bg)
{
    if (y < 0 || y >= g->height) return false;
    if (x < 0 || x >= g->width)  return false;
    hk_cell *cell = &g->cells[y * g->width + x];
    if (cell->glyph[0] != '\0') return false; /* occupied */
    bool wide = hk_is_wide(glyph);
    if (wide) {
        if (x + 1 >= g->width) return false;
        if (g->cells[y * g->width + x + 1].glyph[0] != '\0') return false;
    }
    snprintf(cell->glyph, HK_MAX_GLYPH_BYTES, "%s", glyph);
    if (has_color) {
        cell->fg = color;
        cell->has_fg = true;
    }
    if (has_bg && bg) {
        cell->bg = *bg;
        cell->has_bg = true;
    }
    cell->style = style;
    cell->is_static = false;
    if (wide) {
        hk_cell *right = &g->cells[y * g->width + x + 1];
        snprintf(right->glyph, HK_MAX_GLYPH_BYTES, "%s", "");
        right->covered = true;
        right->is_static = false;
    }
    return true;
}

/* ---- background dot fill ------------------------------------------- */

static const char *const BG_INNER[]  = { "░", "·", "▒", "·" };
static const char *const BG_MIDDLE[] = { "░", "·", "˙", "·" };
static const char *const BG_OUTER[]  = { "·", "˙", "·", " " };

static void bg_zone(double rx, double ry, int grid_radius,
                    const char *const **palette,
                    size_t *n_palette,
                    double *prob)
{
    double r = sqrt(rx * rx + ry * ry);
    double edge = grid_radius + 0.5;
    if (r > edge) {
        *palette = NULL;
        *n_palette = 0;
        *prob = 0.0;
        return;
    }
    double norm = r / (edge < 1.0 ? 1.0 : edge);
    if (norm < 0.35) {
        *palette = BG_INNER;  *n_palette = 4; *prob = 0.78;
    } else if (norm < 0.70) {
        *palette = BG_MIDDLE; *n_palette = 4; *prob = 0.55;
    } else {
        *palette = BG_OUTER;  *n_palette = 4; *prob = 0.32;
    }
}

static void paint_background(hk_grid *g, int grid_radius)
{
    int width = g->width, height = g->height;
    int cx = width / 2, cy = height / 2;
    for (int gy = 0; gy < height; ++gy) {
        for (int gx = 0; gx < width; ++gx) {
            hk_cell *c = &g->cells[gy * width + gx];
            if (c->glyph[0] != '\0' || c->covered) {
                continue;
            }
            double rx = (gx - cx) / 2.0;
            double ry = (double)(gy - cy);
            const char *const *pal = NULL;
            size_t n_pal = 0;
            double prob = 0.0;
            bg_zone(rx, ry, grid_radius, &pal, &n_pal, &prob);
            if (prob <= 0.0 || !pal) continue;
            uint32_t h = (uint32_t)(gx * 2654435761u) ^ (uint32_t)(gy * 40503u);
            double rnd = (h & 0xFFFFu) / 65535.0;
            if (rnd > prob) continue;
            uint32_t hh = (uint32_t)(gx * 73856093u) ^ (uint32_t)(gy * 19349663u);
            const char *ch = pal[(hh & 0xFFFFu) % n_pal];
            if (ch[0] == ' ' && ch[1] == '\0') continue;
            snprintf(c->glyph, HK_MAX_GLYPH_BYTES, "%s", ch);
            c->style |= HK_STYLE_DIM;
        }
    }
}

/* ---- selected indices for a ring ----------------------------------- */

static int selected_indices(int n, double density, int *out, int max_out)
{
    int m = (int)floor(n * density + 0.5);
    if (m < 1) m = 1;
    if (m > n) m = n;
    if (m > max_out) m = max_out;
    if (m == n) {
        for (int i = 0; i < n; ++i) out[i] = i;
        return n;
    }
    for (int i = 0; i < m; ++i) {
        out[i] = (int)floor((double)i * (double)n / (double)m + 0.5);
    }
    return m;
}

/* ---- per-ring breath / spin --------------------------------------- */

static double ring_breath(double breath_global, int ring_idx, int n_rings,
                          double t, double breath_period, bool vary)
{
    if (!vary || breath_period <= 0) return breath_global;
    int span = n_rings > 1 ? (n_rings - 1) : 1;
    double speed = 1.0 + 0.10 * (1.0 - 2.0 * (double)ring_idx / (double)span);
    double phase = 0.45 * (double)ring_idx;
    return sin(2.0 * M_PI * t * speed / breath_period + phase);
}

#define SPIN_RING_FALLOFF 0.32
#define SPIN_FIELD_FRACTION 0.35

static double spin_speed_for_ring(int ring_idx)
{
    double sign = (ring_idx % 2 == 1) ? -1.0 : 1.0;
    double mag = 1.0;
    for (int i = 0; i < ring_idx; ++i) mag *= (1.0 - SPIN_RING_FALLOFF);
    return sign * mag;
}

/* ---- ripple -------------------------------------------------------- */

#define RIPPLE_COUNT 2
static const char RIPPLE_GLYPH[] = "◌";
static const hk_rgb RIPPLE_COLOR = {0x9a, 0xd8, 0xff};

static void paint_ripple(hk_grid *g, int grid_radius, int fold,
                         double t, double ripple_period)
{
    int width = g->width, height = g->height;
    int cx = width / 2, cy = height / 2;
    double base_phase = fmod(t / ripple_period, 1.0);
    if (base_phase < 0) base_phase += 1.0;
    for (int k = 0; k < RIPPLE_COUNT; ++k) {
        double phase = fmod(base_phase + (double)k / (double)RIPPLE_COUNT, 1.0);
        double r_eff = phase * (grid_radius - 0.5);
        if (r_eff < 0.5) continue;
        uint8_t style = (phase < 0.85) ? HK_STYLE_NONE : HK_STYLE_DIM;
        int n = positions_per_sector(r_eff, fold);
        if (n < 1) n = 1;
        for (int sector = 0; sector < fold; ++sector) {
            for (int j = 0; j < n; ++j) {
                double theta = 2.0 * M_PI *
                               (double)(sector * n + j) /
                               (double)(fold * n);
                int x = (int)floor(cx + r_eff * cos(theta) * 2.0 + 0.5);
                int y = (int)floor(cy + r_eff * sin(theta)       + 0.5);
                place(g, x, y, RIPPLE_GLYPH, RIPPLE_COLOR, style,
                      true, NULL, false);
            }
        }
    }
}

/* ---- fractal field (Julia set with dihedral folding) -------------- */

static const char *FRACTAL_GLYPHS[] = {
    " ", " ", "·", "˙", "░", "░", "▒", "▓",
};
#define FRACTAL_N 8
#define FRACTAL_MAX_ITER (FRACTAL_N - 1)

static void apply_fractal_field(hk_grid *g, int grid_radius, int fold,
                                double t, const hk_rgb colors[FRACTAL_N],
                                double spin_angle)
{
    if (!g || !colors) return;
    int width = g->width, height = g->height;
    int cx = width / 2, cy = height / 2;
    double edge = grid_radius + 0.5;
    double scale = 1.5 / (grid_radius < 1 ? 1.0 : (double)grid_radius);
    double drift = 0.05 * t;
    double cre = 0.7885 * cos(drift);
    double cim = 0.7885 * sin(drift);
    double sector = 2.0 * M_PI / (fold > 0 ? fold : 1);
    double half = sector * 0.5;

    for (int gy = 0; gy < height; ++gy) {
        for (int gx = 0; gx < width; ++gx) {
            hk_cell *c = &g->cells[gy * width + gx];
            if (c->covered) continue;
            double rx = (gx - cx) / 2.0;
            double ry = (double)(gy - cy);
            double r = sqrt(rx * rx + ry * ry);
            if (r > edge) continue;
            double theta = fmod(atan2(ry, rx) - spin_angle, sector);
            if (theta < 0) theta += sector;
            if (theta > half) theta = sector - theta;
            double zr = r * cos(theta) * scale;
            double zi = r * sin(theta) * scale;
            int i = 0;
            while (i < FRACTAL_MAX_ITER && (zr * zr + zi * zi) < 4.0) {
                double new_zr = zr * zr - zi * zi + cre;
                double new_zi = 2.0 * zr * zi + cim;
                zr = new_zr; zi = new_zi;
                ++i;
            }
            hk_rgb fill = colors[i];
            if (c->glyph[0] == '\0') {
                const char *ch = FRACTAL_GLYPHS[i];
                if (ch[0] == ' ' && ch[1] == '\0') continue;
                snprintf(c->glyph, HK_MAX_GLYPH_BYTES, "%s", ch);
                c->fg = fill;
                c->has_fg = true;
                c->style = HK_STYLE_NONE;
            } else {
                /* Existing ring/center cell: recolor fg, keep bg/style. */
                c->fg = fill;
                c->has_fg = true;
            }
        }
    }
}

/* ---- main render --------------------------------------------------- */

void hk_render_params_default(hk_render_params *p)
{
    if (!p) return;
    p->t = 0.0;
    p->breath = 0.0;
    p->breath_period = 10.0;
    p->vary_breath = false;
    p->ripple = false;
    p->ripple_period = 4.0;
    p->spin = false;
    p->spin_period = 30.0;
    p->fractal = false;
    p->fractal_colors = NULL;
    p->rings_only = false;
}

void hk_render_spec(const hk_spec *spec, const hk_render_params *params,
                    hk_grid *g)
{
    if (!spec || !g) return;
    hk_render_params local;
    if (!params) { hk_render_params_default(&local); params = &local; }
    hk_grid_clear(g);

    int width = g->width, height = g->height;
    int cx = width / 2, cy = height / 2;
    double t = params->t;
    int n_rings = (int)spec->n_rings;

    /* Center: uses the global breath only. */
    uint8_t center_style = HK_STYLE_NONE;
    if (params->breath > 0.5) center_style = HK_STYLE_BOLD;
    else if (params->breath < -0.5) center_style = HK_STYLE_DIM;
    place(g, cx, cy, spec->center_glyph, spec->center_color,
          center_style, true, NULL, false);

    double base_spin = 0.0;
    if (params->spin && params->spin_period > 0) {
        base_spin = 2.0 * M_PI * t / params->spin_period;
    }

    int sel_buf[256];

    for (int ri = 0; ri < n_rings; ++ri) {
        const hk_ring *ring = &spec->rings[ri];
        double rb = ring_breath(params->breath, ri, n_rings,
                                t, params->breath_period,
                                params->vary_breath);
        double radius_mod = 1.0 + 0.13 * rb;
        double density_mod = 1.0 + 0.18 * rb;
        uint8_t ring_style = HK_STYLE_NONE;
        if (rb < -0.4) ring_style = HK_STYLE_DIM;
        else if (rb > 0.4) ring_style = HK_STYLE_BOLD;

        double r_eff = ring->radius * radius_mod;
        if (r_eff < 0.5) continue;
        int n = positions_per_sector(r_eff, spec->fold);
        if (n < 1) n = 1;
        double density_eff = ring->density * density_mod;
        if (density_eff < 0.05) density_eff = 0.05;
        if (density_eff > 1.0)  density_eff = 1.0;
        int m = selected_indices(n, density_eff, sel_buf,
                                 (int)(sizeof(sel_buf)/sizeof(sel_buf[0])));
        double ring_spin = base_spin * spin_speed_for_ring(ri);

        for (int sector = 0; sector < spec->fold; ++sector) {
            for (int j = 0; j < m; ++j) {
                int k = sel_buf[j];
                double theta = 2.0 * M_PI *
                               (double)(sector * n + k) /
                               (double)(spec->fold * n)
                               + ring->phase + ring_spin;
                double r_at = shape_radius(ring->shape, r_eff, spec->fold,
                                           theta - ring->phase - ring_spin);
                int x = (int)floor(cx + r_at * cos(theta) * 2.0 + 0.5);
                int y = (int)floor(cy + r_at * sin(theta)       + 0.5);
                const char *gly = ring->glyphs[(size_t)k % ring->n_glyphs];
                place(g, x, y, gly, ring->color, ring_style, true,
                      ring->has_bg ? &ring->bg_color : NULL, ring->has_bg);
            }
        }
    }

    if (params->ripple && params->ripple_period > 0) {
        paint_ripple(g, spec->grid_radius, spec->fold,
                     t, params->ripple_period);
    }

    /* rings_only: skip both fractal pass and bg fill. Result: a sparse
     * grid with just center + rings + ripples — used by trail capture. */
    if (params->rings_only) {
        return;
    }

    if (params->fractal) {
        const hk_rgb *colors = params->fractal_colors
            ? params->fractal_colors
            : hk_palette_named(HK_PAL_AURORA);
        double field_spin = base_spin * SPIN_FIELD_FRACTION;
        apply_fractal_field(g, spec->grid_radius, spec->fold,
                            t, colors, field_spin);
    } else {
        paint_background(g, spec->grid_radius);
    }
}

/* ---- backdrop ------------------------------------------------------ */

/* Sparse star field + cardinal axis lines + four lotus gates, sized to
 * any (width, height). Cells are flagged `is_static` so the static
 * field never animates. */

static const char *const BACKDROP_FIELD[] = {
    "·", "·", "˙", "⋅", "∙",
};
static const char *const BACKDROP_ACCENT[] = {
    "✦", "✧",
};
static const hk_rgb BACKDROP_FIELD_COLOR  = {0x1c, 0x20, 0x30};
static const hk_rgb BACKDROP_ACCENT_COLOR = {0x2a, 0x32, 0x52};
static const hk_rgb BACKDROP_AXIS_COLOR   = {0x1a, 0x22, 0x38};
static const hk_rgb BACKDROP_GATE_COLOR   = {0x3a, 0x44, 0x70};

hk_grid *hk_backdrop_new(int width, int height, uint32_t seed)
{
    hk_grid *g = grid_new(width, height);
    if (!g) return NULL;

    /* Sparse star/dot field */
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint32_t h = (uint32_t)(x * 2654435761u) ^
                         (uint32_t)(y * 40503u) ^ seed;
            double r = (h & 0xFFFFFFu) / (double)0xFFFFFFu;
            if (r < 0.012) {
                hk_cell *c = &g->cells[y * width + x];
                snprintf(c->glyph, HK_MAX_GLYPH_BYTES, "%s",
                         BACKDROP_ACCENT[(h >> 13) % 2]);
                c->fg = BACKDROP_ACCENT_COLOR;
                c->has_fg = true;
                c->style = HK_STYLE_DIM;
                c->is_static = true;
            } else if (r < 0.085) {
                hk_cell *c = &g->cells[y * width + x];
                snprintf(c->glyph, HK_MAX_GLYPH_BYTES, "%s",
                         BACKDROP_FIELD[(h >> 17) % 5]);
                c->fg = BACKDROP_FIELD_COLOR;
                c->has_fg = true;
                c->style = HK_STYLE_DIM;
                c->is_static = true;
            }
        }
    }

    /* Cardinal axis lines */
    int cx = width / 2, cy = height / 2;
    for (int x = 0; x < width; ++x) {
        if (x % 3 != 0) continue;
        if (cy >= 0 && cy < height) {
            hk_cell *c = &g->cells[cy * width + x];
            if (c->glyph[0] == '\0') {
                snprintf(c->glyph, HK_MAX_GLYPH_BYTES, "%s", "─");
                c->fg = BACKDROP_AXIS_COLOR;
                c->has_fg = true;
                c->style = HK_STYLE_DIM;
                c->is_static = true;
            }
        }
    }
    for (int y = 0; y < height; ++y) {
        if (y % 2 != 0) continue;
        if (cx >= 0 && cx < width) {
            hk_cell *c = &g->cells[y * width + cx];
            if (c->glyph[0] == '\0') {
                snprintf(c->glyph, HK_MAX_GLYPH_BYTES, "%s", "│");
                c->fg = BACKDROP_AXIS_COLOR;
                c->has_fg = true;
                c->style = HK_STYLE_DIM;
                c->is_static = true;
            }
        }
    }

    /* Diagonal wheel-spoke hints */
    if (width > 6 && height > 6) {
        int max_r = (cx < cy) ? cx : cy;
        for (int step = 0; step < max_r; step += 4) {
            int angles[4] = {45, 135, 225, 315};
            for (int a = 0; a < 4; ++a) {
                double rad = angles[a] * M_PI / 180.0;
                int px = cx + (int)floor(step * cos(rad) * 2 + 0.5);
                int py = cy + (int)floor(step * sin(rad)     + 0.5);
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    hk_cell *c = &g->cells[py * width + px];
                    if (c->glyph[0] == '\0') {
                        snprintf(c->glyph, HK_MAX_GLYPH_BYTES, "%s", "·");
                        c->fg = BACKDROP_AXIS_COLOR;
                        c->has_fg = true;
                        c->style = HK_STYLE_DIM;
                        c->is_static = true;
                    }
                }
            }
        }
    }

    /* Four cardinal lotus gates (toranas) */
    int gate_y_top    = height / 6;
    if (gate_y_top < 1) gate_y_top = 1;
    int gate_y_bot    = height - height / 6;
    if (gate_y_bot >= height) gate_y_bot = height - 2;
    int gate_x_left   = width / 8;
    if (gate_x_left < 2) gate_x_left = 2;
    int gate_x_right  = width - width / 8;
    if (gate_x_right >= width) gate_x_right = width - 3;

    int gates[4][2] = {
        {cx, gate_y_top},
        {gate_x_right, cy},
        {cx, gate_y_bot},
        {gate_x_left, cy},
    };
    for (int i = 0; i < 4; ++i) {
        int gx = gates[i][0], gy = gates[i][1];
        if (gx < 0 || gx >= width || gy < 0 || gy >= height) continue;
        hk_cell *c = &g->cells[gy * width + gx];
        snprintf(c->glyph, HK_MAX_GLYPH_BYTES, "%s", "❀");
        c->fg = BACKDROP_GATE_COLOR;
        c->has_fg = true;
        c->style = HK_STYLE_DIM;
        c->is_static = true;
    }

    return g;
}

/* ---- compose helpers ----------------------------------------------- */

void hk_stamp_grid(hk_grid *dst, const hk_grid *src, int top, int left)
{
    if (!dst || !src) return;
    for (int y = 0; y < src->height; ++y) {
        int dy = top + y;
        if (dy < 0 || dy >= dst->height) continue;
        for (int x = 0; x < src->width; ++x) {
            int dx = left + x;
            if (dx < 0 || dx >= dst->width) continue;
            const hk_cell *s = &src->cells[y * src->width + x];
            if (s->glyph[0] == '\0' && !s->covered) continue;
            dst->cells[dy * dst->width + dx] = *s;
        }
    }
}

static uint8_t scale_u8(uint8_t v, double k)
{
    long out = (long)((double)v * k + 0.5);
    if (out < 0) out = 0;
    if (out > 255) out = 255;
    return (uint8_t)out;
}

void hk_stamp_grid_dimmed(hk_grid *dst, const hk_grid *src,
                          int top, int left, double dim)
{
    if (!dst || !src) return;
    if (dim < 0.0) dim = 0.0;
    if (dim > 1.0) dim = 1.0;
    for (int y = 0; y < src->height; ++y) {
        int dy = top + y;
        if (dy < 0 || dy >= dst->height) continue;
        for (int x = 0; x < src->width; ++x) {
            int dx = left + x;
            if (dx < 0 || dx >= dst->width) continue;
            const hk_cell *s = &src->cells[y * src->width + x];
            if (s->glyph[0] == '\0' && !s->covered) continue;
            hk_cell out = *s;
            if (out.has_fg) {
                out.fg.r = scale_u8(out.fg.r, dim);
                out.fg.g = scale_u8(out.fg.g, dim);
                out.fg.b = scale_u8(out.fg.b, dim);
            }
            if (out.has_bg) {
                out.bg.r = scale_u8(out.bg.r, dim);
                out.bg.g = scale_u8(out.bg.g, dim);
                out.bg.b = scale_u8(out.bg.b, dim);
            }
            /* Always dim style so trails recede behind current frame. */
            out.style |= HK_STYLE_DIM;
            dst->cells[dy * dst->width + dx] = out;
        }
    }
}

/* Approximate visible width of a UTF-8 string — counts each codepoint as
 * 1 (or 2 if wide). Used only for text-line centering, so good enough. */
static int visible_width(const char *s)
{
    int w = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        int len = 1;
        if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        char tmp[8] = {0};
        for (int i = 0; i < len && s[i]; ++i) tmp[i] = s[i];
        w += hk_is_wide(tmp) ? 2 : 1;
        s += len;
    }
    return w;
}

void hk_stamp_text_line(hk_grid *dst, const char *text, int row, uint8_t style)
{
    if (!dst || !text || !*text) return;
    if (row < 0 || row >= dst->height) return;
    int tw = visible_width(text);
    int left = (dst->width - tw) / 2;
    if (left < 0) left = 0;

    /* Walk the bytes, treat each codepoint as one cell. */
    const char *s = text;
    int x = left;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        int len = 1;
        if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        if (x >= 0 && x < dst->width) {
            hk_cell *cell = &dst->cells[row * dst->width + x];
            cell->glyph[0] = '\0';
            for (int i = 0; i < len && i < HK_MAX_GLYPH_BYTES - 1 && s[i]; ++i) {
                cell->glyph[i] = s[i];
                cell->glyph[i + 1] = '\0';
            }
            cell->has_fg = false;
            cell->has_bg = false;
            cell->style = style;
            cell->is_static = false;
            cell->covered = false;
        }
        ++x;
        s += len;
    }
}
