/* Cell grid → ANSI escape sequences. Truecolor (24-bit) only — every
 * mainstream terminal in use today supports it (macOS Terminal,
 * iTerm2, kitty, alacritty, wezterm, GNOME Terminal, Windows Terminal).
 */

#include <stdio.h>
#include <string.h>

#include "haikala.h"

static int append(char *buf, size_t bufsize, size_t pos, const char *s)
{
    if (!s) return 0;
    size_t len = strlen(s);
    if (pos + len + 1 > bufsize) return -1;
    memcpy(buf + pos, s, len);
    buf[pos + len] = '\0';
    return (int)len;
}

/* SGR: hk_style + fg + bg → \x1b[…m */
static size_t emit_sgr(char *buf, size_t bufsize, size_t pos,
                       const hk_cell *cell)
{
    /* Build the parameter list in fragments. */
    char body[128];
    size_t n = 0;
    body[0] = '\0';

    /* style */
    if (cell->style & HK_STYLE_DIM) {
        n += (size_t)snprintf(body + n, sizeof(body) - n, "%s2", n ? ";" : "");
    }
    if (cell->style & HK_STYLE_BOLD) {
        n += (size_t)snprintf(body + n, sizeof(body) - n, "%s1", n ? ";" : "");
    }
    if (cell->style & HK_STYLE_ITALIC) {
        n += (size_t)snprintf(body + n, sizeof(body) - n, "%s3", n ? ";" : "");
    }
    /* fg */
    if (cell->has_fg) {
        n += (size_t)snprintf(body + n, sizeof(body) - n,
                              "%s38;2;%u;%u;%u",
                              n ? ";" : "",
                              (unsigned)cell->fg.r,
                              (unsigned)cell->fg.g,
                              (unsigned)cell->fg.b);
    }
    /* bg */
    if (cell->has_bg) {
        n += (size_t)snprintf(body + n, sizeof(body) - n,
                              "%s48;2;%u;%u;%u",
                              n ? ";" : "",
                              (unsigned)cell->bg.r,
                              (unsigned)cell->bg.g,
                              (unsigned)cell->bg.b);
    }
    if (n == 0) {
        /* Nothing to set — emit a reset so we don't carry over */
        int r = append(buf, bufsize, pos, "\x1b[0m");
        return (r > 0) ? (size_t)r : 0;
    }
    char esc[160];
    int len = snprintf(esc, sizeof(esc), "\x1b[0;%sm", body);
    if (len < 0) return 0;
    int r = append(buf, bufsize, pos, esc);
    return (r > 0) ? (size_t)r : 0;
}

size_t hk_grid_to_ansi(const hk_grid *g, char *buf, size_t bufsize)
{
    if (!g || !buf || bufsize == 0) return 0;
    size_t pos = 0;
    if (bufsize > 0) buf[0] = '\0';

    for (int y = 0; y < g->height; ++y) {
        for (int x = 0; x < g->width; ++x) {
            const hk_cell *c = &g->cells[y * g->width + x];
            if (c->covered) continue; /* covered by wide left neighbor */
            if (c->glyph[0] == '\0') {
                int n = append(buf, bufsize, pos, " ");
                if (n < 0) return pos;
                pos += (size_t)n;
                continue;
            }
            size_t sgr = emit_sgr(buf, bufsize, pos, c);
            pos += sgr;
            int n = append(buf, bufsize, pos, c->glyph);
            if (n < 0) return pos;
            pos += (size_t)n;
        }
        /* End of row — reset and newline. */
        int n = append(buf, bufsize, pos, "\x1b[0m");
        if (n > 0) pos += (size_t)n;
        if (y < g->height - 1) {
            n = append(buf, bufsize, pos, "\r\n");
            if (n < 0) return pos;
            pos += (size_t)n;
        }
    }
    return pos;
}
