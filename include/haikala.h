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

/* ---------- color math --------------------------------------------- */

/* Rotate `base` by `degrees` in HLS space. Returns the new RGB. */
hk_rgb hk_rotate_hue(hk_rgb base, double degrees);

/* ---------- fractal palettes (8 stops each) ------------------------ */

typedef enum {
    HK_PAL_AURORA = 0,
    HK_PAL_EMBER,
    HK_PAL_OCEAN,
    HK_PAL_FOREST,
    HK_PAL_SAKURA,
    HK_PAL_TWILIGHT,
    HK_PAL_LAVA,
    HK_PAL_CORAL,
    HK_PAL_COUNT,
} hk_palette_id;

#define HK_PALETTE_STOPS 8

const hk_rgb *hk_palette_named(hk_palette_id id);
hk_palette_id hk_palette_id_from_name(const char *name); /* returns -1 if unknown */
const char   *hk_palette_name(hk_palette_id id);

/* ---------- haiku & symbols ----------------------------------------- */

typedef struct {
    const char        *id;
    const char        *line1;
    const char        *line2;
    const char        *line3;
    const char        *author;
    const char        *season;   /* "spring"|"summer"|"autumn"|"winter"|"new_year" */
    const char *const *tokens;
    size_t             n_tokens;
} hk_haiku;

extern const hk_haiku hk_haiku_table[];
extern const size_t   hk_haiku_count;

const hk_haiku *hk_haiku_get(const char *id);

const char *const *hk_glyphs_for(const char *token, size_t *out_n);
const char *const *hk_text_glyphs_for(const char *token, size_t *out_n);

/* ---------- auto-derived params (palette, fold) -------------------- */

int  hk_fold_for_haiku(const hk_haiku *h);
bool hk_palette_from_haiku(const hk_haiku *h, hk_rgb out[HK_PALETTE_STOPS]);

/* ---------- weather-derived palette --------------------------------- */

typedef enum {
    HK_SEASON_SPRING = 0,
    HK_SEASON_SUMMER = 1,
    HK_SEASON_AUTUMN = 2,
    HK_SEASON_WINTER = 3,
} hk_season;

typedef enum {
    HK_WEATHER_UNKNOWN = 0,
    HK_WEATHER_CLEAR,
    HK_WEATHER_CLOUDY,
    HK_WEATHER_RAIN,
    HK_WEATHER_SNOW,
    HK_WEATHER_STORM,
    HK_WEATHER_FOG,
} hk_weather_cond;

typedef struct {
    hk_season       season;
    hk_weather_cond condition;
    char            raw_text[128]; /* condition string from API, for display */
} hk_weather;

hk_season hk_season_now(void); /* from local clock */

/* Fetch weather for zip via `curl wttr.in/<zip>?format=%C`. Returns
 * false on network/parse failure; the zip is sanitized (alphanumeric +
 * hyphens, ≤ 16 chars) to keep the popen call safe. */
bool hk_weather_fetch(const char *zip, hk_weather *out);

const char *hk_weather_cond_name(hk_weather_cond c);
const char *hk_season_name(hk_season s);

/* Build an 8-stop palette from (season, condition). Uses one of four
 * base season ramps and applies a small HLS mutation per condition. */
void hk_palette_from_weather(hk_season s, hk_weather_cond c,
                             hk_rgb out[HK_PALETTE_STOPS]);

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
    hk_rgb             bg_color;
    bool               has_bg;
    hk_band            band;
    hk_shape           shape;
    double             phase;
} hk_ring;

typedef struct {
    int        fold;
    hk_ring    rings[16];
    size_t     n_rings;
    char       center_glyph[HK_MAX_GLYPH_BYTES];
    hk_rgb     center_color;
    int        grid_radius;
    bool       no_emoji;       /* spec was built in --no-emoji mode */
    const char *haiku_id;
} hk_spec;

typedef enum {
    HK_SIZE_SMALL  = 7,
    HK_SIZE_MEDIUM = 11,
    HK_SIZE_LARGE  = 15,
    HK_SIZE_HUGE   = 20,
} hk_size;

/* Compute the largest mandala radius that fits in (term_width, term_height)
 * after reserving rows for the haiku header. Capped to a practical max
 * to keep the geometry coherent on very large displays. Returns at
 * least HK_SIZE_SMALL even on very small terminals. */
int hk_size_max_for_terminal(int term_width, int term_height);

bool hk_haiku_to_spec(
    const hk_haiku *h,
    int    fold,
    int    grid_radius,
    bool   no_emoji,
    hk_spec *out);

/* ---------- mandala rendering --------------------------------------- */

hk_grid *hk_grid_new(int radius);
void     hk_grid_free(hk_grid *g);
void     hk_grid_clear(hk_grid *g);

bool hk_is_wide(const char *glyph);
bool hk_is_emoji(const char *glyph);

/* Render parameters bundle. Defaults produce a calm, breath-only render
 * that matches the original simple mode. Set flags for the various
 * Python-upstream features. */
typedef struct {
    double t;
    double breath;
    double breath_period;
    bool   vary_breath;
    bool   ripple;
    double ripple_period;
    bool   spin;
    double spin_period;
    bool   fractal;
    const hk_rgb *fractal_colors; /* 8 stops, NULL = aurora default */
} hk_render_params;

void hk_render_params_default(hk_render_params *p);

/* Render the spec into `out`. Out must already be sized for the spec. */
void hk_render_spec(const hk_spec *spec,
                    const hk_render_params *params,
                    hk_grid *out);

/* ---------- backdrop ------------------------------------------------ */

hk_grid *hk_backdrop_new(int width, int height, uint32_t seed);
void     hk_grid_free_arbitrary(hk_grid *g);

/* ---------- compose & emit ------------------------------------------ */

void hk_stamp_grid(hk_grid *dst, const hk_grid *src, int top, int left);
void hk_stamp_text_line(hk_grid *dst, const char *text, int row, uint8_t style);

/* Per-cell hue offset closure. Returns degrees to add to that cell's
 * hue rotation. NULL means: use only the global hue_shift. */
typedef double (*hk_hue_field_fn)(int x, int y, void *user);

/* Convert a Cell grid into ANSI escape sequences with hue rotation
 * applied to non-static cells. Returns bytes written. */
size_t hk_grid_to_ansi(
    const hk_grid *g,
    double hue_shift,
    hk_hue_field_fn hue_field,
    void *hue_field_user,
    char *buf,
    size_t bufsize);

/* ---------- emanating-wave hue field ------------------------------- */

typedef struct {
    int    cx, cy;            /* center in canvas coords */
    double t;                 /* current time, seconds */
    double period;            /* seconds per pulse */
    double max_r;             /* outer radius */
    int    folds[8];          /* angular fold sequence */
    int    n_folds;
    double band;              /* annulus thickness */
    double intensity;         /* peak hue shift in degrees */
} hk_emanate;

void   hk_emanate_default(hk_emanate *e);
double hk_emanate_at(int x, int y, void *user); /* user = const hk_emanate * */

/* ---------- terminal control ---------------------------------------- */

typedef struct {
    int width;
    int height;
} hk_term_size;

bool hk_term_init(void);
void hk_term_restore(void);
bool hk_term_size_get(hk_term_size *out);
bool hk_term_quit_pressed(void);
void hk_term_enter_alt_screen(void);
void hk_term_exit_alt_screen(void);
void hk_term_hide_cursor(void);
void hk_term_show_cursor(void);
void hk_term_home(void);
void hk_term_clear_screen(void);

/* ---------- animation entry ---------------------------------------- */

typedef struct {
    int    fold;            /* 0 = auto */
    int    grid_radius;
    double bpm;
    double fps;
    bool   no_animate;

    bool   no_emoji;

    bool   fractal;
    hk_palette_id palette;  /* HK_PAL_COUNT or negative = auto */

    bool   cycle;
    double cycle_period;

    bool   ripple;
    double ripple_period;

    bool   spin;
    double spin_period;

    bool   emanate;
    double emanate_period;

    bool   vary_breath;

    /* Optional caller-supplied 8-stop palette. When `has_forced_palette`
     * is true it overrides both `palette` (named) and the haiku-auto
     * derivation — used by the `--weather` mode. */
    hk_rgb forced_palette[HK_PALETTE_STOPS];
    bool   has_forced_palette;
} hk_options;

void hk_options_default(hk_options *opt);
int  hk_run(const hk_haiku *h, const hk_options *opt);

#ifdef __cplusplus
}
#endif

#endif /* HAIKALA_H */
