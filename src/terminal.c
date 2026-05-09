/* Terminal control via raw POSIX (termios + ANSI escapes). No
 * third-party deps. Saves the original termios on init and restores
 * it on exit (or via signal). Handles SIGINT/SIGTERM cleanly so
 * Ctrl-C never leaves the terminal in a half-cooked state.
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "haikala.h"

static struct termios g_orig;
static bool           g_have_orig = false;
static volatile sig_atomic_t g_quit = 0;

static void on_signal(int sig)
{
    (void)sig;
    g_quit = 1;
}

bool hk_term_init(void)
{
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        return false;
    }
    if (tcgetattr(STDIN_FILENO, &g_orig) != 0) {
        return false;
    }
    g_have_orig = true;

    struct termios raw = g_orig;
    /* Disable canonical mode and echo; keep ISIG so Ctrl-C still arrives
     * as SIGINT. We handle the signal so the caller can clean up. */
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_iflag &= ~(IXON | INPCK | ISTRIP);
    raw.c_cc[VMIN]  = 0;  /* non-blocking reads */
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        g_have_orig = false;
        return false;
    }

    /* Handle Ctrl-C / TERM cleanly. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Make sure we restore on abnormal exit. */
    atexit(hk_term_restore);
    return true;
}

void hk_term_restore(void)
{
    if (g_have_orig) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig);
        g_have_orig = false;
    }
    /* Best-effort: leave alt screen, show cursor, reset SGR. */
    /* These are no-ops on a non-tty. */
    fputs("\x1b[?25h\x1b[?1049l\x1b[0m", stdout);
    fflush(stdout);
}

bool hk_term_size_get(hk_term_size *out)
{
    if (!out) return false;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) {
        out->width  = 80;
        out->height = 24;
        return false;
    }
    out->width  = ws.ws_col;
    out->height = ws.ws_row;
    return true;
}

bool hk_term_quit_pressed(void)
{
    if (g_quit) return true;
    if (!isatty(STDIN_FILENO)) return false;
    char buf[16];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) return false;
    for (ssize_t i = 0; i < n; ++i) {
        char c = buf[i];
        if (c == 'q' || c == 'Q' || c == '\x03' || c == '\x1b') {
            return true;
        }
    }
    return false;
}

void hk_term_enter_alt_screen(void) { fputs("\x1b[?1049h", stdout); fflush(stdout); }
void hk_term_exit_alt_screen(void)  { fputs("\x1b[?1049l", stdout); fflush(stdout); }
void hk_term_hide_cursor(void)      { fputs("\x1b[?25l",  stdout); fflush(stdout); }
void hk_term_show_cursor(void)      { fputs("\x1b[?25h",  stdout); fflush(stdout); }
void hk_term_home(void)             { fputs("\x1b[H",     stdout); fflush(stdout); }
void hk_term_clear_screen(void)     { fputs("\x1b[2J\x1b[H", stdout); fflush(stdout); }
