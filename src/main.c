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
        "Options:\n"
        "  --haiku <id>      render a specific haiku (see --list)\n"
        "  --list            list available haiku and exit\n"
        "  --size <s>        small | medium | large | huge (default huge)\n"
        "  --fold <n>        4 | 6 | 8 | 10 | 12 | 14 | 16 (or auto, default)\n"
        "  --bpm <n>         breaths per minute (default 6)\n"
        "  --fps <n>         frames per second (default 24)\n"
        "  --no-animate      render once and exit\n"
        "  -h, --help        show this help\n"
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
    printf("%-22s %-9s %s\n", "id", "season", "first line");
    printf("%-22s %-9s %s\n", "----", "------", "----------");
    for (size_t i = 0; i < hk_haiku_count; ++i) {
        const hk_haiku *h = &hk_haiku_table[i];
        printf("%-22s %-9s %s\n", h->id, h->season, h->line1);
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
            usage(stdout);
            return 0;
        }
        if (strcmp(a, "--list") == 0) {
            list_haiku();
            return 0;
        }
        if (strcmp(a, "--no-animate") == 0) {
            opt.no_animate = true;
            continue;
        }
        if (strcmp(a, "--haiku") == 0 && i + 1 < argc) {
            id = argv[++i];
            continue;
        }
        if (strcmp(a, "--size") == 0 && i + 1 < argc) {
            int r = parse_size(argv[++i]);
            if (r < 0) {
                fprintf(stderr, "haikalac: bad size %s\n", argv[i]);
                return 2;
            }
            opt.grid_radius = r;
            continue;
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
            opt.bpm = atof(argv[++i]);
            continue;
        }
        if (strcmp(a, "--fps") == 0 && i + 1 < argc) {
            opt.fps = atof(argv[++i]);
            continue;
        }
        fprintf(stderr, "haikalac: unknown option %s\n", a);
        usage(stderr);
        return 2;
    }

    /* Pick haiku: explicit, or rotate by seconds-since-epoch as a
     * deterministic-but-varying default. */
    const hk_haiku *h = NULL;
    if (id) {
        h = hk_haiku_get(id);
        if (!h) {
            fprintf(stderr, "haikalac: unknown haiku %s — try --list\n", id);
            return 2;
        }
    } else {
        /* Default: pick by clock seconds modulo count for variety. */
        size_t idx = (size_t)(time(NULL)) % hk_haiku_count;
        h = &hk_haiku_table[idx];
    }

    return hk_run(h, &opt);
}
