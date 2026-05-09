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
        "  --haiku <id>          render a specific haiku (see --list)\n"
        "  --list                list available haiku and exit\n"
        "\n"
        "Geometry & timing:\n"
        "  --size <s>            small | medium | large | huge (default huge)\n"
        "  --fold <n>            auto | 4 | 6 | 8 | 10 | 12 | 14 | 16\n"
        "  --bpm <n>             breaths per minute (default 6)\n"
        "  --fps <n>             frames per second (default 24)\n"
        "  --no-animate          render once and exit\n"
        "\n"
        "Visual modes:\n"
        "  --no-emoji            colorable Unicode + bg tints (no emoji)\n"
        "  --emoji               default emoji mode\n"
        "  --fractal             Julia-set background, dihedral folded\n"
        "  --palette <name>      auto | aurora | ember | ocean | forest |\n"
        "                        sakura | twilight | lava | coral\n"
        "  --cycle               slow hue rotation across the mandala\n"
        "  --cycle-period <s>    seconds for one full hue rotation (default 90)\n"
        "  --ripple              transient rings sweep outward from center\n"
        "  --ripple-period <s>   seconds per ripple (default 4)\n"
        "  --spin                kaleidoscope: per-ring rotation, alternating\n"
        "  --spin-period <s>     innermost ring revolution (default 30)\n"
        "  --emanate             hue waves with cycling angular symmetry\n"
        "  --emanate-period <s>  seconds per wave (default 5)\n"
        "  --no-vary-breath      one synchronized breath for all rings\n"
        "\n"
        "  -h, --help            show this help\n"
        "\n"
        "Press q or Ctrl-C in animation mode to exit.\n",
        out);
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

int main(int argc, char **argv)
{
    const char *id = NULL;
    hk_options opt;
    hk_options_default(&opt);

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            usage(stdout); return 0;
        }
        if (strcmp(a, "--list") == 0) { list_haiku(); return 0; }
        if (strcmp(a, "--no-animate") == 0) { opt.no_animate = true; continue; }
        if (strcmp(a, "--no-emoji")   == 0) { opt.no_emoji   = true; continue; }
        if (strcmp(a, "--emoji")      == 0) { opt.no_emoji   = false; continue; }
        if (strcmp(a, "--fractal")    == 0) { opt.fractal    = true; continue; }
        if (strcmp(a, "--no-fractal") == 0) { opt.fractal    = false; continue; }
        if (strcmp(a, "--cycle")      == 0) { opt.cycle      = true; continue; }
        if (strcmp(a, "--no-cycle")   == 0) { opt.cycle      = false; continue; }
        if (strcmp(a, "--ripple")     == 0) { opt.ripple     = true; continue; }
        if (strcmp(a, "--no-ripple")  == 0) { opt.ripple     = false; continue; }
        if (strcmp(a, "--spin")       == 0) { opt.spin       = true; continue; }
        if (strcmp(a, "--no-spin")    == 0) { opt.spin       = false; continue; }
        if (strcmp(a, "--emanate")    == 0) { opt.emanate    = true; continue; }
        if (strcmp(a, "--no-emanate") == 0) { opt.emanate    = false; continue; }
        if (strcmp(a, "--no-vary-breath") == 0) { opt.vary_breath = false; continue; }
        if (strcmp(a, "--haiku") == 0 && i + 1 < argc) {
            id = argv[++i]; continue;
        }
        if (strcmp(a, "--size") == 0 && i + 1 < argc) {
            int r = parse_size(argv[++i]);
            if (r < 0) { fprintf(stderr, "haikalac: bad size %s\n", argv[i]); return 2; }
            opt.grid_radius = r; continue;
        }
        if (strcmp(a, "--fold") == 0 && i + 1 < argc) {
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
        if (strcmp(a, "--bpm") == 0 && i + 1 < argc) {
            opt.bpm = atof(argv[++i]); continue;
        }
        if (strcmp(a, "--fps") == 0 && i + 1 < argc) {
            opt.fps = atof(argv[++i]); continue;
        }
        if (strcmp(a, "--cycle-period") == 0 && i + 1 < argc) {
            opt.cycle_period = atof(argv[++i]); continue;
        }
        if (strcmp(a, "--ripple-period") == 0 && i + 1 < argc) {
            opt.ripple_period = atof(argv[++i]); continue;
        }
        if (strcmp(a, "--spin-period") == 0 && i + 1 < argc) {
            opt.spin_period = atof(argv[++i]); continue;
        }
        if (strcmp(a, "--emanate-period") == 0 && i + 1 < argc) {
            opt.emanate_period = atof(argv[++i]); continue;
        }
        if (strcmp(a, "--palette") == 0 && i + 1 < argc) {
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
    return hk_run(h, &opt);
}
