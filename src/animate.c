/* Animation loop. Composes each frame as a single ANSI string and
 * writes it in one fputs/flush pair. Sleeps between frames using
 * nanosleep. Wires every Python-upstream feature: breath, vary breath,
 * cycle (hue rotation), fractal, ripple, spin (kaleidoscope), emanate
 * (hue waves with cycling angular symmetry), no-emoji + bg tints, and
 * auto palette/fold from the haiku.
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "haikala.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Layout overhead: header is 5 visible rows (3 lines + spacer + author)
 * plus 2 padding rows above and 1 below — keep in sync with the composer
 * below where the mandala stamp position is computed. */
#define HK_HEADER_OVERHEAD 8
#define HK_RADIUS_HARD_CAP 40

int hk_size_max_for_terminal(int term_width, int term_height)
{
    if (term_width < 30 || term_height < HK_HEADER_OVERHEAD + 5) {
        return HK_SIZE_SMALL;
    }
    /* Mandala body is (2r+1) rows tall, (4r+1) cells wide. Solve for r
     * so the body fits within (term_h - header_overhead) rows and
     * term_w columns. */
    int rh = (term_height - HK_HEADER_OVERHEAD - 1) / 2;
    int rw = (term_width - 1) / 4;
    int r  = (rh < rw) ? rh : rw;
    if (r < HK_SIZE_SMALL)        r = HK_SIZE_SMALL;
    if (r > HK_RADIUS_HARD_CAP)   r = HK_RADIUS_HARD_CAP;
    return r;
}

void hk_options_default(hk_options *opt)
{
    if (!opt) return;
    opt->fold        = 0;
    opt->grid_radius = HK_SIZE_HUGE;
    opt->bpm         = 6.0;
    opt->fps         = 24.0;
    opt->no_animate  = false;

    opt->no_emoji    = false;

    opt->fractal     = false;
    opt->palette     = (hk_palette_id)-1;  /* -1 = auto */

    opt->cycle        = false;
    opt->cycle_period = 90.0;

    opt->ripple       = false;
    opt->ripple_period = 4.0;

    opt->spin         = false;
    opt->spin_period  = 30.0;

    opt->emanate         = false;
    opt->emanate_period  = 5.0;

    opt->vary_breath  = true;

    opt->sound        = false;
    opt->sound_gain   = 1.0;

    opt->has_forced_palette = false;
    /* forced_palette left zero-initialized; not consulted unless flag set */
}

/* Resolve fractal palette: forced (e.g. --weather) > explicit name >
 * auto (haiku words) > aurora fallback. Writes into `out`. */
static void resolve_fractal_palette(const hk_haiku *h,
                                    const hk_options *opt,
                                    hk_rgb out[HK_PALETTE_STOPS])
{
    if (opt->has_forced_palette) {
        memcpy(out, opt->forced_palette, sizeof(hk_rgb) * HK_PALETTE_STOPS);
        return;
    }
    if ((int)opt->palette >= 0 && opt->palette < HK_PAL_COUNT) {
        const hk_rgb *named = hk_palette_named(opt->palette);
        memcpy(out, named, sizeof(hk_rgb) * HK_PALETTE_STOPS);
        return;
    }
    if (hk_palette_from_haiku(h, out)) {
        return;
    }
    const hk_rgb *named = hk_palette_named(HK_PAL_AURORA);
    memcpy(out, named, sizeof(hk_rgb) * HK_PALETTE_STOPS);
}

static double monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void sleep_for(double seconds)
{
    if (seconds <= 0) return;
    struct timespec ts;
    ts.tv_sec  = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
}

/* Compose one frame into `buf`, returning bytes written. */
static size_t compose_frame(char *buf, size_t bufsize,
                            const hk_grid *backdrop,
                            const hk_spec *spec,
                            const hk_haiku *haiku,
                            hk_grid *mandala_grid,
                            hk_grid *canvas,
                            const hk_render_params *rp,
                            double hue_shift,
                            const hk_emanate *emanate)
{
    /* canvas := copy of backdrop */
    memcpy(canvas->cells, backdrop->cells,
           (size_t)canvas->width * (size_t)canvas->height * sizeof(hk_cell));

    /* Stamp haiku header at the top. */
    int header_top = 1;
    hk_stamp_text_line(canvas, haiku->line1, header_top + 0, HK_STYLE_NONE);
    hk_stamp_text_line(canvas, haiku->line2, header_top + 1, HK_STYLE_NONE);
    hk_stamp_text_line(canvas, haiku->line3, header_top + 2, HK_STYLE_NONE);
    char author_line[128];
    snprintf(author_line, sizeof(author_line), "— %s", haiku->author);
    hk_stamp_text_line(canvas, author_line, header_top + 4,
                       HK_STYLE_DIM | HK_STYLE_ITALIC);

    /* Render mandala body. */
    hk_render_spec(spec, rp, mandala_grid);

    int header_block = header_top + 3 + 3;
    int top  = header_block;
    int alt  = (canvas->height - mandala_grid->height) / 2 + (header_block / 4);
    if (alt > top) top = alt;
    if (top + mandala_grid->height > canvas->height) {
        top = canvas->height - mandala_grid->height;
        if (top < 0) top = 0;
    }
    int left = (canvas->width - mandala_grid->width) / 2;
    if (left < 0) left = 0;
    hk_stamp_grid(canvas, mandala_grid, top, left);

    /* Update emanate center to current mandala location. */
    hk_emanate local;
    if (emanate) {
        local = *emanate;
        local.cx = left + mandala_grid->width / 2;
        local.cy = top + mandala_grid->height / 2;
    }

    return hk_grid_to_ansi(canvas,
                           hue_shift,
                           emanate ? hk_emanate_at : NULL,
                           emanate ? &local : NULL,
                           buf, bufsize);
}

int hk_run(const hk_haiku *h, const hk_options *opt)
{
    if (!h || !opt) return 1;

    int fold = (opt->fold == 0) ? hk_fold_for_haiku(h) : opt->fold;
    int radius = opt->grid_radius;
    if (radius < 5) radius = HK_SIZE_HUGE;

    hk_spec spec;
    if (!hk_haiku_to_spec(h, fold, radius, opt->no_emoji, &spec)) {
        fprintf(stderr, "haikalac: failed to build mandala spec for %s\n",
                h->id);
        return 1;
    }

    /* Resolve fractal palette if needed. */
    hk_rgb fractal_colors[HK_PALETTE_STOPS];
    if (opt->fractal) {
        resolve_fractal_palette(h, opt, fractal_colors);
    }

    /* Build a render-params bundle that mirrors the options. */
    hk_render_params rp;
    hk_render_params_default(&rp);
    rp.ripple        = opt->ripple;
    rp.ripple_period = opt->ripple_period;
    rp.spin          = opt->spin;
    rp.spin_period   = opt->spin_period;
    rp.fractal       = opt->fractal;
    rp.fractal_colors = opt->fractal ? fractal_colors : NULL;

    if (opt->no_animate) {
        hk_grid *mandala = hk_grid_new(radius);
        if (!mandala) return 1;
        rp.t = 0.0;
        rp.breath = 0.0;
        rp.vary_breath = false;
        hk_render_spec(&spec, &rp, mandala);

        printf("\n%*s%s\n", (mandala->width - (int)strlen(h->line1)) / 2, "",
               h->line1);
        printf("%*s%s\n", (mandala->width - (int)strlen(h->line2)) / 2, "",
               h->line2);
        printf("%*s%s\n", (mandala->width - (int)strlen(h->line3)) / 2, "",
               h->line3);
        printf("\n%*s— %s\n\n",
               (mandala->width - (int)strlen(h->author) - 2) / 2, "",
               h->author);

        size_t bufsize = (size_t)mandala->width * (size_t)mandala->height
                         * 96 + 1024;
        char *buf = (char *)malloc(bufsize);
        if (!buf) { hk_grid_free(mandala); return 1; }
        hk_grid_to_ansi(mandala, 0.0, NULL, NULL, buf, bufsize);
        fputs(buf, stdout);
        fputs("\n", stdout);
        free(buf);
        hk_grid_free(mandala);
        return 0;
    }

    /* Animated mode. */
    if (!hk_term_init()) {
        fprintf(stderr, "haikalac: not running in a tty; use --no-animate\n");
        return 1;
    }
    hk_term_size term;
    hk_term_size_get(&term);
    hk_term_enter_alt_screen();
    hk_term_hide_cursor();
    hk_term_clear_screen();

    hk_grid *backdrop = hk_backdrop_new(term.width, term.height,
                                        (uint32_t)0x9e3779b1u);
    hk_grid *mandala  = hk_grid_new(radius);
    hk_grid *canvas   = hk_backdrop_new(term.width, term.height, 0);
    if (!backdrop || !mandala || !canvas) {
        hk_grid_free(backdrop);
        hk_grid_free(mandala);
        hk_grid_free(canvas);
        hk_term_show_cursor();
        hk_term_exit_alt_screen();
        hk_term_restore();
        return 1;
    }

    size_t bufsize = (size_t)term.width * (size_t)term.height * 96 + 4096;
    char *buf = (char *)malloc(bufsize);
    if (!buf) {
        hk_grid_free(backdrop); hk_grid_free(mandala); hk_grid_free(canvas);
        hk_term_show_cursor(); hk_term_exit_alt_screen(); hk_term_restore();
        return 1;
    }

    double period = 60.0 / (opt->bpm > 0 ? opt->bpm : 6.0);
    double frame_interval = 1.0 / (opt->fps > 0 ? opt->fps : 24.0);
    rp.breath_period = period;
    rp.vary_breath = opt->vary_breath;

    hk_emanate emanate;
    hk_emanate_default(&emanate);
    emanate.period = opt->emanate_period;
    emanate.max_r  = (double)(spec.grid_radius + 1);

    /* Sound mode: open sox capture before the loop. If sox isn't
     * installed we print a one-line note and continue without effect. */
    bool sound_active = false;
    if (opt->sound) {
        sound_active = hk_sound_open();
        if (!sound_active) {
            fprintf(stderr,
                "haikalac: --sound requires `sox` on PATH; continuing "
                "without audio reactivity\n");
        }
    }

    double start = monotonic_seconds();
    double sound_phase = 0.0;
    double prev_t      = 0.0;

    while (1) {
        if (hk_term_quit_pressed()) break;
        double t = monotonic_seconds() - start;
        double dt = t - prev_t;
        prev_t = t;
        rp.t = t;
        rp.breath = sin(2.0 * M_PI * t / period);

        double cycle_period = opt->cycle_period > 1.0 ? opt->cycle_period : 1.0;
        double hue_shift = opt->cycle ? (t / cycle_period) * 360.0 : 0.0;

        /* Audio reactivity: drain the sox pipe and accumulate extra
         * hue rotation proportional to recent loudness. A full-volume
         * second rotates ~720°·gain by default (gain=1.0). */
        if (sound_active) {
            double e = hk_sound_energy() * opt->sound_gain;
            if (e < 0.0) e = 0.0;
            sound_phase += e * 720.0 * dt;
            hue_shift += sound_phase;
        }

        emanate.t = t;
        compose_frame(buf, bufsize, backdrop, &spec, h, mandala, canvas,
                      &rp, hue_shift,
                      opt->emanate ? &emanate : NULL);
        hk_term_home();
        fputs(buf, stdout);
        fflush(stdout);

        sleep_for(frame_interval);
    }

    if (sound_active) hk_sound_close();
    free(buf);
    hk_grid_free(backdrop);
    hk_grid_free(mandala);
    hk_grid_free(canvas);

    hk_term_show_cursor();
    hk_term_exit_alt_screen();
    hk_term_restore();
    fputs("breath out.\n", stdout);
    return 0;
}
