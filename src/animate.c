/* Animation loop. Compose each frame as a single ANSI string and write
 * it in one fputs/flush pair. Sleep between frames using nanosleep.
 *
 * The composer mirrors the upstream Python:
 *   - copy of the static backdrop (sized to the terminal),
 *   - haiku header lines stamped at the top,
 *   - mandala body (centered) overlaid on top.
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

void hk_options_default(hk_options *opt)
{
    if (!opt) return;
    opt->fold        = 0;
    opt->grid_radius = HK_SIZE_HUGE;
    opt->bpm         = 6.0;
    opt->fps         = 24.0;
    opt->no_animate  = false;
}

/* fold_for_haiku — same signal as the Python:
 *   round(1.5 * n_tokens + n_words / 3), clamped to [4, 16] even. */
static int auto_fold(const hk_haiku *h)
{
    int n_words = 0;
    const char *lines[] = {h->line1, h->line2, h->line3};
    for (int i = 0; i < 3; ++i) {
        const char *s = lines[i];
        bool in_word = false;
        for (; s && *s; ++s) {
            char c = *s;
            bool is_alpha = ((c >= 'a' && c <= 'z') ||
                             (c >= 'A' && c <= 'Z'));
            if (is_alpha) {
                if (!in_word) { ++n_words; in_word = true; }
            } else {
                in_word = false;
            }
        }
    }
    double score = 1.5 * (double)h->n_tokens + (double)n_words / 3.0;
    int fold = (int)floor(score + 0.5);
    if (fold < 4) fold = 4;
    if (fold > 16) fold = 16;
    if (fold % 2) ++fold;
    if (fold > 16) fold = 16;
    return fold;
}

/* Compose one frame into the caller's buffer; return bytes written. */
static size_t compose_frame(char *buf, size_t bufsize,
                            const hk_grid *backdrop,
                            const hk_spec *spec,
                            const hk_haiku *haiku,
                            hk_grid *mandala_grid,
                            hk_grid *canvas,
                            double breath)
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
    hk_render_spec(spec, breath, mandala_grid);

    int header_block = header_top + 3 + 3;
    int top  = header_block;
    if ((canvas->height - mandala_grid->height) / 2 + (header_block / 4) > top) {
        top = (canvas->height - mandala_grid->height) / 2 + (header_block / 4);
    }
    if (top + mandala_grid->height > canvas->height) {
        top = canvas->height - mandala_grid->height;
        if (top < 0) top = 0;
    }
    int left = (canvas->width - mandala_grid->width) / 2;
    if (left < 0) left = 0;
    hk_stamp_grid(canvas, mandala_grid, top, left);

    return hk_grid_to_ansi(canvas, buf, bufsize);
}

static void sleep_for(double seconds)
{
    if (seconds <= 0) return;
    struct timespec ts;
    ts.tv_sec  = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
}

static double monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int hk_run(const hk_haiku *h, const hk_options *opt)
{
    if (!h || !opt) return 1;

    int fold = (opt->fold == 0) ? auto_fold(h) : opt->fold;
    int radius = opt->grid_radius;
    if (radius < 5) radius = HK_SIZE_HUGE;

    hk_spec spec;
    if (!hk_haiku_to_spec(h, fold, radius, &spec)) {
        fprintf(stderr, "haikalac: failed to build mandala spec for %s\n",
                h->id);
        return 1;
    }

    if (opt->no_animate) {
        /* Static one-shot render: no terminal init, no alt screen. */
        hk_grid *mandala = hk_grid_new(radius);
        if (!mandala) return 1;
        hk_render_spec(&spec, 0.0, mandala);

        /* Output haiku header in plain text + the mandala. */
        printf("\n%*s%s\n", (mandala->width - (int)strlen(h->line1)) / 2, "",
               h->line1);
        printf("%*s%s\n", (mandala->width - (int)strlen(h->line2)) / 2, "",
               h->line2);
        printf("%*s%s\n", (mandala->width - (int)strlen(h->line3)) / 2, "",
               h->line3);
        printf("\n%*s— %s\n\n", (mandala->width - (int)strlen(h->author) - 2) / 2,
               "", h->author);

        /* ANSI buffer needs ~30 bytes/cell for SGR + glyph in the worst
         * case; round up to 64. */
        size_t bufsize = (size_t)mandala->width * (size_t)mandala->height * 64
                         + 1024;
        char *buf = (char *)malloc(bufsize);
        if (!buf) { hk_grid_free(mandala); return 1; }
        hk_grid_to_ansi(mandala, buf, bufsize);
        fputs(buf, stdout);
        fputs("\n", stdout);
        free(buf);
        hk_grid_free(mandala);
        return 0;
    }

    /* Animated mode: alt screen, raw mode, repaint loop. */
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
    /* Replace canvas with a fresh empty grid; we use canvas as a working
     * copy so we don't need to allocate every frame. The seed=0 backdrop
     * we just made will be overwritten by memcpy each frame. */
    if (!backdrop || !mandala || !canvas) {
        hk_grid_free(backdrop);
        hk_grid_free(mandala);
        hk_grid_free(canvas);
        hk_term_show_cursor();
        hk_term_exit_alt_screen();
        hk_term_restore();
        return 1;
    }

    /* Allocate a generous frame buffer. */
    size_t bufsize = (size_t)term.width * (size_t)term.height * 48 + 4096;
    char *buf = (char *)malloc(bufsize);
    if (!buf) {
        hk_grid_free(backdrop); hk_grid_free(mandala); hk_grid_free(canvas);
        hk_term_show_cursor(); hk_term_exit_alt_screen(); hk_term_restore();
        return 1;
    }

    double period = 60.0 / (opt->bpm > 0 ? opt->bpm : 6.0);
    double frame_interval = 1.0 / (opt->fps > 0 ? opt->fps : 24.0);
    double start = monotonic_seconds();

    while (1) {
        if (hk_term_quit_pressed()) break;
        double t = monotonic_seconds() - start;
        double breath = sin(2.0 * M_PI * t / period);

        size_t n = compose_frame(buf, bufsize, backdrop, &spec, h,
                                 mandala, canvas, breath);
        (void)n;
        hk_term_home();
        fputs(buf, stdout);
        fflush(stdout);

        sleep_for(frame_interval);
    }

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
