/* CLI entry point. Hand-rolled argv parser — no getopt dependency, so
 * we stay strictly within ISO C and can compile on any C99 toolchain. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "haikala.h"

static void usage(FILE *out)
{
    fputs(
        "haikalac — a breathing haiku mandala for the terminal\n"
        "\n"
        "Usage: haikalac [options]\n"
        "\n"
        "Selection:\n"
        "  -H, --haiku <id>          render a specific haiku (see --list)\n"
        "  -l, --list                list available haiku and exit\n"
        "\n"
        "Geometry & timing:\n"
        "  -s, --size <s>            small | medium | large | huge | max\n"
        "                            (default huge; max fits current terminal)\n"
        "  -F, --fold <n>            auto | 4 | 6 | 8 | 10 | 12 | 14 | 16\n"
        "  -b, --bpm <n>             breaths per minute (default 6)\n"
        "  -r, --fps <n>             frames per second (default 24)\n"
        "  -n, --no-animate          render once and exit\n"
        "\n"
        "Visual modes:\n"
        "  -e, --no-emoji            colorable Unicode + bg tints (no emoji)\n"
        "      --emoji               default emoji mode\n"
        "  -f, --fractal             Julia-set background, dihedral folded\n"
        "      --no-fractal          turn fractal off\n"
        "  -P, --palette <name>      auto | aurora | ember | ocean | forest |\n"
        "                            sakura | twilight | lava | coral\n"
        "  -c, --cycle               slow hue rotation across the mandala\n"
        "      --cycle-period <s>    seconds for one full hue rotation (default 90)\n"
        "  -R, --ripple              transient rings sweep outward from center\n"
        "      --ripple-period <s>   seconds per ripple (default 4)\n"
        "  -S, --spin                kaleidoscope: per-ring rotation, alternating\n"
        "      --spin-period <s>     innermost ring revolution (default 30)\n"
        "  -E, --emanate             hue waves with cycling angular symmetry\n"
        "      --emanate-period <s>  seconds per wave (default 5)\n"
        "      --no-vary-breath      one synchronized breath for all rings\n"
        "  -t, --trails              fading echoes of past ring positions\n"
        "      --trail-length <n>    trail history depth, 1..8 (default 4)\n"
        "\n"
        "Weather mode (forces --fractal; needs `curl` on PATH):\n"
        "  -w, --weather             fetch local weather, derive palette\n"
        "  -z, --zip <code>          zip / postal code (else prompts on stdin)\n"
        "\n"
        "Sound mode (audio-reactive hue rotation; needs `sox` on PATH):\n"
        "  -a, --sound               drive hue rotation from microphone level\n"
        "  -g, --sound-gain <n>      multiplier on energy (default 1.0)\n"
        "\n"
        "  -h, --help                show this help\n"
        "\n"
        "Press q or Ctrl-C in animation mode to exit.\n",
        out);
}

/* Accept either the short form or the long form. NULL short skips. */
static bool match(const char *arg, const char *short_, const char *long_)
{
    if (short_ && strcmp(arg, short_) == 0) return true;
    return strcmp(arg, long_) == 0;
}

static int parse_size(const char *s)
{
    if (strcmp(s, "small")  == 0) return HK_SIZE_SMALL;
    if (strcmp(s, "medium") == 0) return HK_SIZE_MEDIUM;
    if (strcmp(s, "large")  == 0) return HK_SIZE_LARGE;
    if (strcmp(s, "huge")   == 0) return HK_SIZE_HUGE;
    return -1;
}

static void list_haiku(void)
{
    printf("%-28s %-9s %s\n", "id", "season", "first line");
    printf("%-28s %-9s %s\n", "----", "------", "----------");
    for (size_t i = 0; i < hk_haiku_count; ++i) {
        const hk_haiku *h = &hk_haiku_table[i];
        printf("%-28s %-9s %s\n", h->id, h->season, h->line1);
    }
}

/* Read a single line from stdin into `out` (max len-1 bytes), strip
 * trailing whitespace. Returns false on EOF or empty input. */
static bool prompt_line(const char *prompt, char *out, size_t len)
{
    if (!out || len == 0) return false;
    if (prompt) { fputs(prompt, stdout); fflush(stdout); }
    if (!fgets(out, (int)len, stdin)) return false;
    size_t n = strlen(out);
    while (n > 0) {
        unsigned char c = (unsigned char)out[n - 1];
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') out[--n] = '\0';
        else break;
    }
    return n > 0;
}

int main(int argc, char **argv)
{
    const char *id = NULL;
    const char *zip = NULL;
    bool weather_mode = false;
    hk_options opt;
    hk_options_default(&opt);

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (match(a, "-h", "--help"))             { usage(stdout); return 0; }
        if (match(a, "-l", "--list"))             { list_haiku();  return 0; }
        if (match(a, "-n", "--no-animate"))       { opt.no_animate = true;  continue; }
        if (match(a, "-e", "--no-emoji"))         { opt.no_emoji = true;    continue; }
        if (match(a, NULL, "--emoji"))            { opt.no_emoji = false;   continue; }
        if (match(a, "-f", "--fractal"))          { opt.fractal = true;     continue; }
        if (match(a, NULL, "--no-fractal"))       { opt.fractal = false;    continue; }
        if (match(a, "-c", "--cycle"))            { opt.cycle = true;       continue; }
        if (match(a, NULL, "--no-cycle"))         { opt.cycle = false;      continue; }
        if (match(a, "-R", "--ripple"))           { opt.ripple = true;      continue; }
        if (match(a, NULL, "--no-ripple"))        { opt.ripple = false;     continue; }
        if (match(a, "-S", "--spin"))             { opt.spin = true;        continue; }
        if (match(a, NULL, "--no-spin"))          { opt.spin = false;       continue; }
        if (match(a, "-E", "--emanate"))          { opt.emanate = true;     continue; }
        if (match(a, NULL, "--no-emanate"))       { opt.emanate = false;    continue; }
        if (match(a, NULL, "--no-vary-breath"))   { opt.vary_breath = false; continue; }
        if (match(a, "-w", "--weather"))          { weather_mode = true;    continue; }
        if (match(a, "-a", "--sound"))            { opt.sound = true;       continue; }
        if (match(a, NULL, "--no-sound"))         { opt.sound = false;      continue; }
        if (match(a, "-t", "--trails"))           { opt.trails = true;      continue; }
        if (match(a, NULL, "--no-trails"))        { opt.trails = false;     continue; }
        if (match(a, NULL, "--trail-length") && i + 1 < argc) {
            int n = atoi(argv[++i]);
            if (n < 1) n = 1; if (n > 8) n = 8;
            opt.trail_length = n;
            continue;
        }

        if (match(a, "-H", "--haiku") && i + 1 < argc) { id = argv[++i]; continue; }
        if (match(a, "-z", "--zip")   && i + 1 < argc) { zip = argv[++i]; continue; }
        if (match(a, "-g", "--sound-gain") && i + 1 < argc) {
            opt.sound_gain = atof(argv[++i]);
            if (opt.sound_gain < 0) opt.sound_gain = 0;
            continue;
        }
        if (match(a, "-s", "--size") && i + 1 < argc) {
            const char *v = argv[++i];
            if (strcmp(v, "max") == 0) {
                hk_term_size ts;
                if (hk_term_size_get(&ts)) {
                    opt.grid_radius =
                        hk_size_max_for_terminal(ts.width, ts.height);
                } else {
                    opt.grid_radius = HK_SIZE_HUGE;
                }
                continue;
            }
            int r = parse_size(v);
            if (r < 0) { fprintf(stderr, "haikalac: bad size %s\n", v); return 2; }
            opt.grid_radius = r; continue;
        }
        if (match(a, "-F", "--fold") && i + 1 < argc) {
            const char *v = argv[++i];
            if (strcmp(v, "auto") == 0) {
                opt.fold = 0;
            } else {
                int n = atoi(v);
                if (n < 4 || n > 16 || (n % 2)) {
                    fprintf(stderr,
                        "haikalac: --fold must be auto or even in 4..16\n");
                    return 2;
                }
                opt.fold = n;
            }
            continue;
        }
        if (match(a, "-b", "--bpm") && i + 1 < argc) {
            opt.bpm = atof(argv[++i]); continue;
        }
        if (match(a, "-r", "--fps") && i + 1 < argc) {
            opt.fps = atof(argv[++i]); continue;
        }
        if (match(a, NULL, "--cycle-period") && i + 1 < argc) {
            opt.cycle_period = atof(argv[++i]); continue;
        }
        if (match(a, NULL, "--ripple-period") && i + 1 < argc) {
            opt.ripple_period = atof(argv[++i]); continue;
        }
        if (match(a, NULL, "--spin-period") && i + 1 < argc) {
            opt.spin_period = atof(argv[++i]); continue;
        }
        if (match(a, NULL, "--emanate-period") && i + 1 < argc) {
            opt.emanate_period = atof(argv[++i]); continue;
        }
        if (match(a, "-P", "--palette") && i + 1 < argc) {
            const char *v = argv[++i];
            if (strcmp(v, "auto") == 0) {
                opt.palette = (hk_palette_id)-1;
            } else {
                hk_palette_id pid = hk_palette_id_from_name(v);
                if (pid < 0) {
                    fprintf(stderr, "haikalac: unknown palette %s\n", v);
                    return 2;
                }
                opt.palette = pid;
            }
            continue;
        }
        fprintf(stderr, "haikalac: unknown option %s\n", a);
        usage(stderr);
        return 2;
    }

    const hk_haiku *h = NULL;
    if (id) {
        h = hk_haiku_get(id);
        if (!h) {
            fprintf(stderr, "haikalac: unknown haiku %s — try --list\n", id);
            return 2;
        }
    } else {
        size_t idx = (size_t)(time(NULL)) % hk_haiku_count;
        h = &hk_haiku_table[idx];
    }

    /* Weather mode: prompt for zip if missing, fetch, build palette. */
    if (weather_mode) {
        char buf[64];
        if (!zip) {
            if (!prompt_line("Enter zip / postal code: ", buf, sizeof(buf))) {
                fprintf(stderr, "haikalac: no zip provided\n");
                return 2;
            }
            zip = buf;
        }
        hk_weather w;
        if (!hk_weather_fetch(zip, &w)) {
            fprintf(stderr,
                "haikalac: weather fetch failed for %s "
                "(needs `curl` on PATH and network access)\n", zip);
            return 1;
        }
        printf("Weather for %s: %s — season=%s, condition=%s\n",
               zip, w.raw_text,
               hk_season_name(w.season), hk_weather_cond_name(w.condition));
        opt.fractal = true;
        opt.has_forced_palette = true;
        hk_palette_from_weather(w.season, w.condition, opt.forced_palette);
    }

    return hk_run(h, &opt);
}
