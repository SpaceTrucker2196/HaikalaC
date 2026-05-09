/* HaikalaC — a breathing haiku mandala for the terminal, in pure C99.
 *
 * No third-party dependencies. Only ISO C and POSIX (termios.h, sys/ioctl.h,
 * unistd.h). UTF-8 glyphs are written as raw byte sequences; the terminal
 * does the rendering. Truecolor (24-bit) ANSI sequences are used for fg/bg.
 *
 * This header is the project's single public API surface.
 */

#ifndef HAIKALA_H
#define HAIKALA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum bytes for one glyph: emoji + VS-16 + a few combining marks.
 * Keep generous so we never truncate a multi-codepoint cluster. */
#define HK_MAX_GLYPH_BYTES 16

/* ---------- color & cells ------------------------------------------- */

typedef struct {
    uint8_t r, g, b;
} hk_rgb;

typedef enum {
    HK_STYLE_NONE   = 0,
    HK_STYLE_DIM    = 1u << 0,
    HK_STYLE_BOLD   = 1u << 1,
    HK_STYLE_ITALIC = 1u << 2,
} hk_style_bits;

typedef struct {
    char    glyph[HK_MAX_GLYPH_BYTES]; /* UTF-8, NUL-terminated; "" = empty */
    hk_rgb  fg;
    hk_rgb  bg;
    bool    has_fg;
    bool    has_bg;
    uint8_t style;     /* OR of hk_style_bits */
    bool    is_static; /* backdrop cell — never hue-cycle, never redraw */
    bool    covered;   /* right half of a wide glyph (do not output) */
} hk_cell;

typedef struct {
    int      width;
    int      height;
    hk_cell *cells; /* row-major: cells[y * width + x] */
} hk_grid;

/* ---------- haiku & symbols ----------------------------------------- */

typedef struct {
    const char        *id;
    const char        *line1;
    const char        *line2;
    const char        *line3;
    const char        *author;
    const char        *season;   /* "spring"|"summer"|"autumn"|"winter"|"new_year" */
    const char *const *tokens;   /* array of token strings */
    size_t             n_tokens;
} hk_haiku;

extern const hk_haiku hk_haiku_table[];
extern const size_t   hk_haiku_count;

const hk_haiku *hk_haiku_get(const char *id);

/* Look up the glyph tuple for a token. Returns NULL if the token is unknown. */
const char *const *hk_glyphs_for(const char *token, size_t *out_n);

/* ---------- mandala spec -------------------------------------------- */

typedef enum {
    HK_BAND_INNER  = 0,
    HK_BAND_MIDDLE = 1,
    HK_BAND_OUTER  = 2,
} hk_band;

typedef enum {
    HK_SHAPE_CIRCLE  = 0,
    HK_SHAPE_POLYGON = 1,
    HK_SHAPE_STAR    = 2,
    HK_SHAPE_PETAL   = 3,
} hk_shape;

typedef struct {
    double             radius;
    const char *const *glyphs;
    size_t             n_glyphs;
    double             density;
    hk_rgb             color;
    hk_band            band;
    hk_shape           shape;
    double             phase;
} hk_ring;

typedef struct {
    int     fold;
    hk_ring rings[16];   /* fixed cap; mandalas have ≤ ~9 rings in practice */
    size_t  n_rings;
    char    center_glyph[HK_MAX_GLYPH_BYTES];
    hk_rgb  center_color;
    int     grid_radius;
    const char *haiku_id;
} hk_spec;

/* Sizes (y-cell radii). Width is 4*r+1, height is 2*r+1. */
typedef enum {
    HK_SIZE_SMALL  = 7,   /* fits tight terminals */
    HK_SIZE_MEDIUM = 11,  /* fits 80x24 */
    HK_SIZE_LARGE  = 15,
    HK_SIZE_HUGE   = 20,  /* default — circular ~40 tall */
} hk_size;

bool hk_haiku_to_spec(const hk_haiku *h, int fold, int grid_radius, hk_spec *out);

/* ---------- mandala rendering --------------------------------------- */

hk_grid *hk_grid_new(int radius);
void     hk_grid_free(hk_grid *g);
void     hk_grid_clear(hk_grid *g);

bool hk_is_wide(const char *glyph);
bool hk_is_emoji(const char *glyph);

/* Render the spec into `out` (must already be sized for spec->grid_radius).
 * `breath` is in [-1, 1]: 0 neutral, +1 full inhale, -1 full exhale. */
void hk_render_spec(const hk_spec *spec, double breath, hk_grid *out);

/* ---------- backdrop ------------------------------------------------ */

/* Build a deterministic, dim full-screen backdrop sized to the terminal.
 * Caller owns the returned grid (free with hk_grid_free_arbitrary). */
hk_grid *hk_backdrop_new(int width, int height, uint32_t seed);
void     hk_grid_free_arbitrary(hk_grid *g);

/* ---------- compose & emit ------------------------------------------ */

/* Stamp `src` onto `dst` at (top, left). EMPTY src cells fall through. */
void hk_stamp_grid(hk_grid *dst, const hk_grid *src, int top, int left);

/* Center `text` (UTF-8) onto `dst` at the given row, with the given style. */
void hk_stamp_text_line(hk_grid *dst, const char *text, int row, uint8_t style);

/* Convert a Cell grid into ANSI escape sequences. Caller provides a buffer.
 * Returns number of bytes written (excluding NUL). If `bufsize` is too
 * small, returns the size required (computed conservatively). */
size_t hk_grid_to_ansi(const hk_grid *g, char *buf, size_t bufsize);

/* ---------- terminal control ---------------------------------------- */

typedef struct {
    int width;
    int height;
} hk_term_size;

bool hk_term_init(void);     /* save state, enable raw mode */
void hk_term_restore(void);  /* restore on exit */
bool hk_term_size_get(hk_term_size *out);

/* Non-blocking: returns true if the user pressed q/Q/ESC/Ctrl-C since
 * the last call. Drains any other pending bytes. */
bool hk_term_quit_pressed(void);

void hk_term_enter_alt_screen(void);
void hk_term_exit_alt_screen(void);
void hk_term_hide_cursor(void);
void hk_term_show_cursor(void);
void hk_term_home(void);
void hk_term_clear_screen(void);

/* ---------- animation entry point ----------------------------------- */

typedef struct {
    int    fold;          /* 4..16 even, or 0 = auto */
    int    grid_radius;   /* one of the hk_size enum values */
    double bpm;           /* breaths per minute */
    double fps;           /* target frames per second */
    bool   no_animate;    /* render once and exit */
} hk_options;

void hk_options_default(hk_options *opt);
int  hk_run(const hk_haiku *h, const hk_options *opt);

#ifdef __cplusplus
}
#endif

#endif /* HAIKALA_H */
