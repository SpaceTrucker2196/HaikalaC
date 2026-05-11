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

/* Compression / AGC tuning. These are deliberately not exposed in the
 * public API — the user already has `--sound-gain` for room-level
 * adjustments, and these constants are about *shape* of the response
 * curve, which should be stable across rooms. */
#define AGC_ATTACK        0.15   /* how fast peak_ref chases loud peaks */
#define AGC_RELEASE       0.002  /* how slowly peak_ref decays in silence */
#define AGC_FLOOR         0.003  /* noise floor — below this is "silence" */
#define AGC_MAX_NORM      1.5    /* allow brief overshoot above ref */
#define COMPRESSOR_POWER  0.55   /* power-curve exponent; <1 = compressed */
#define SMOOTH_ALPHA      0.35   /* EMA on the compressed value */
#define IDLE_DECAY        0.96   /* per-frame EMA decay when no audio */
#define PEAK_IDLE_DECAY   0.9995 /* peak_ref decay during long silence */

/* Filter state — AGC peak follower + EMA. Exposed via the (file-scope)
 * `g_filter` and the test-only `hk_sound_filter_apply` so the algorithm
 * is unit-testable without a real audio pipe. */
typedef struct {
    double peak_ref;
    double ema;
} sound_filter_state;

static FILE              *g_pipe   = NULL;
static int                g_fd     = -1;
static sound_filter_state g_filter = { 0.01, 0.0 };

/* Apply one frame of the AGC + compressor + EMA chain.
 *
 *   raw_rms ──► [ AGC peak follower ] ──► raw_rms / peak_ref
 *           ──► [ soft-knee compressor: pow(norm, 0.55) ]
 *           ──► [ EMA smoothing ]                          ──► 0..1
 *
 * The AGC chases loud peaks quickly and releases very slowly, so a
 * quiet room produces a small reference and any speech/music quickly
 * fills the response range. A loud room raises the reference instead
 * of clipping at 1.0. The power-curve compressor then pulls quiet
 * values up and ceilings the loud ones — same shape a hardware audio
 * compressor uses on a vocal mic. */
static double filter_apply(sound_filter_state *st, double raw_rms,
                           bool has_data)
{
    if (has_data) {
        double coeff = (raw_rms > st->peak_ref) ? AGC_ATTACK : AGC_RELEASE;
        st->peak_ref += (raw_rms - st->peak_ref) * coeff;
        if (st->peak_ref < AGC_FLOOR) st->peak_ref = AGC_FLOOR;

        double norm = raw_rms / st->peak_ref;
        if (norm > AGC_MAX_NORM) norm = AGC_MAX_NORM;

        double comp = pow(norm, COMPRESSOR_POWER);
        if (comp > 1.0) comp = 1.0;
        if (comp < 0.0) comp = 0.0;

        st->ema = SMOOTH_ALPHA * comp + (1.0 - SMOOTH_ALPHA) * st->ema;
    } else {
        st->ema      *= IDLE_DECAY;
        st->peak_ref *= PEAK_IDLE_DECAY;
        if (st->peak_ref < AGC_FLOOR) st->peak_ref = AGC_FLOOR;
    }
    if (st->ema < 0.0) st->ema = 0.0;
    if (st->ema > 1.0) st->ema = 1.0;
    return st->ema;
}

/* Public test hook: drive the filter with a synthetic RMS, no pipe. */
double hk_sound_filter_apply(double raw_rms, bool has_data)
{
    return filter_apply(&g_filter, raw_rms, has_data);
}

void hk_sound_filter_reset(void)
{
    g_filter.peak_ref = 0.01;
    g_filter.ema      = 0.0;
}

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
    hk_sound_filter_reset();
    return true;
}

void hk_sound_close(void)
{
    if (g_pipe) {
        pclose(g_pipe);
        g_pipe = NULL;
        g_fd = -1;
    }
    hk_sound_filter_reset();
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
        double raw_rms =
            (samples > 0) ? sqrt(sumsq / (double)samples) : 0.0;
        return filter_apply(&g_filter, raw_rms, true);
    }
    return filter_apply(&g_filter, 0.0, false);
}
