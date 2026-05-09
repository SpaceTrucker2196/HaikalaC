/* Audio-reactive mode.
 *
 * Captures from the system default audio device by spawning `sox` via
 * popen — same "delegate to a runtime tool" pattern that --weather uses
 * for `curl`. Keeps HaikalaC itself free of any C-level audio library
 * dependency.
 *
 * The sox command requests raw signed 16-bit mono PCM at 22.05 kHz.
 * Each animation frame we read whatever bytes are buffered (the pipe
 * is set non-blocking) and compute a windowed RMS amplitude. That
 * value is fed through an exponential moving average so the visual
 * response feels musical rather than jittery.
 *
 * sox availability:
 *   macOS:   `brew install sox`
 *   Debian:  `apt install sox`
 *   Fedora:  `dnf install sox`
 * On systems without sox, --sound prints a warning and continues with
 * energy fixed at 0 (no visual effect).
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "haikala.h"

#define SOUND_SAMPLE_RATE 22050
#define SOUND_BUFFER      8192   /* int16 samples = 16 KB max per read */

static FILE  *g_pipe = NULL;
static int    g_fd   = -1;
static double g_ema  = 0.0;

bool hk_sound_open(void)
{
    if (g_pipe) return true;
    /* `-d` = default audio device. Quiet mode keeps stderr clean.
     * stderr is redirected so sox's chatter doesn't interleave with
     * our terminal output. */
    g_pipe = popen(
        "sox -d -q -t raw -r 22050 -e signed -b 16 -c 1 - 2>/dev/null",
        "r");
    if (!g_pipe) return false;
    g_fd = fileno(g_pipe);
    int flags = fcntl(g_fd, F_GETFL, 0);
    if (flags == -1 || fcntl(g_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        pclose(g_pipe);
        g_pipe = NULL;
        g_fd = -1;
        return false;
    }
    g_ema = 0.0;
    return true;
}

void hk_sound_close(void)
{
    if (g_pipe) {
        pclose(g_pipe);
        g_pipe = NULL;
        g_fd = -1;
    }
    g_ema = 0.0;
}

double hk_sound_energy(void)
{
    if (!g_pipe || g_fd < 0) return 0.0;

    int16_t buf[SOUND_BUFFER];
    /* Drain whatever the pipe has accumulated since last call.
     * Non-blocking: returns -1 with EAGAIN when no data is ready. */
    ssize_t total = read(g_fd, buf, sizeof(buf));
    if (total > 0) {
        size_t samples = (size_t)total / sizeof(int16_t);
        double sumsq = 0.0;
        for (size_t i = 0; i < samples; ++i) {
            double s = (double)buf[i] / 32768.0;
            sumsq += s * s;
        }
        double rms = (samples > 0) ? sqrt(sumsq / (double)samples) : 0.0;
        /* α = 0.35 — fast enough to feel reactive, slow enough that a
         * single transient doesn't dominate the visual.  */
        const double alpha = 0.35;
        g_ema = alpha * rms + (1.0 - alpha) * g_ema;
    } else {
        /* Slow decay when no fresh audio. Gives a nice "ringing" feel
         * after a beat instead of cutting off abruptly. */
        g_ema *= 0.96;
    }
    if (g_ema < 0.0) g_ema = 0.0;
    if (g_ema > 1.0) g_ema = 1.0;
    return g_ema;
}
