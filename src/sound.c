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

/* FFT window size — power of two. 512 samples = ~23 ms at 22050 Hz.
 * Bin resolution = SAMPLE_RATE / FFT_N ≈ 43 Hz/bin. */
#define FFT_N             512
#define FFT_LOG2          9

/* Band boundaries in Hz. */
#define BAND_LO_HZ        250.0
#define BAND_HI_HZ        2000.0

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

/* Forward decl — defined below; spectrum code (above hk_sound_energy)
 * needs to call into it. */
static double filter_apply(sound_filter_state *st, double raw_rms,
                           bool has_data);

static FILE              *g_pipe   = NULL;
static int                g_fd     = -1;
static sound_filter_state g_filter = { 0.01, 0.0 };

/* Spectrum analysis state. Ring buffer of the most recent FFT_N audio
 * samples plus three per-band AGC + EMA filters. */
typedef struct {
    double   ring[FFT_N];
    int      write_pos;
    bool     primed;          /* true once the ring has been filled once */
    sound_filter_state low;
    sound_filter_state mid;
    sound_filter_state high;
    /* Pre-computed Hann window, lazily built once. */
    double   hann[FFT_N];
    bool     hann_ready;
} spectrum_state;

static spectrum_state g_spec = {
    .ring       = {0},
    .write_pos  = 0,
    .primed     = false,
    .low        = {0.01, 0.0},
    .mid        = {0.01, 0.0},
    .high       = {0.01, 0.0},
    .hann       = {0},
    .hann_ready = false,
};

static void hann_init(void)
{
    if (g_spec.hann_ready) return;
    for (int i = 0; i < FFT_N; ++i) {
        g_spec.hann[i] = 0.5 * (1.0 - cos(2.0 * M_PI * i / (FFT_N - 1)));
    }
    g_spec.hann_ready = true;
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* In-place radix-2 Cooley-Tukey FFT on parallel real/imag arrays.
 * Standard bit-reverse permutation, then log2(N) butterfly passes. */
static void fft_radix2(double *re, double *im)
{
    /* Bit-reverse permutation. */
    int j = 0;
    for (int i = 0; i < FFT_N - 1; ++i) {
        if (i < j) {
            double tr = re[i]; re[i] = re[j]; re[j] = tr;
            double ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
        int k = FFT_N >> 1;
        while (k <= j) { j -= k; k >>= 1; }
        j += k;
    }
    /* Butterflies. */
    for (int s = 1; s <= FFT_LOG2; ++s) {
        int m    = 1 << s;
        int half = m >> 1;
        double theta = -2.0 * M_PI / m;
        double wpr = cos(theta), wpi = sin(theta);
        for (int kk = 0; kk < FFT_N; kk += m) {
            double wr = 1.0, wi = 0.0;
            for (int jj = 0; jj < half; ++jj) {
                int i1 = kk + jj;
                int i2 = i1 + half;
                double tr = wr * re[i2] - wi * im[i2];
                double ti = wr * im[i2] + wi * re[i2];
                re[i2] = re[i1] - tr;
                im[i2] = im[i1] - ti;
                re[i1] += tr;
                im[i1] += ti;
                double nwr = wr * wpr - wi * wpi;
                wi = wr * wpi + wi * wpr;
                wr = nwr;
            }
        }
    }
}

static void spectrum_push_sample(double s)
{
    g_spec.ring[g_spec.write_pos++] = s;
    if (g_spec.write_pos >= FFT_N) {
        g_spec.write_pos = 0;
        g_spec.primed = true;
    }
}

/* Compute the three-band raw energies from the current ring buffer. */
static void spectrum_compute_raw(double *raw_low, double *raw_mid, double *raw_high)
{
    hann_init();
    double re[FFT_N], im[FFT_N];
    /* Walk the ring from oldest to newest, apply Hann window. */
    for (int i = 0; i < FFT_N; ++i) {
        int idx = (g_spec.write_pos + i) % FFT_N;
        re[i] = g_spec.ring[idx] * g_spec.hann[i];
        im[i] = 0.0;
    }
    fft_radix2(re, im);

    double hz_per_bin = (double)SOUND_SAMPLE_RATE / (double)FFT_N;
    int lo_split = (int)(BAND_LO_HZ / hz_per_bin + 0.5);
    int hi_split = (int)(BAND_HI_HZ / hz_per_bin + 0.5);
    int nyquist  = FFT_N / 2;
    if (lo_split < 1) lo_split = 1;
    if (hi_split > nyquist) hi_split = nyquist;

    double sum_low = 0.0, sum_mid = 0.0, sum_high = 0.0;
    int n_low = 0, n_mid = 0, n_high = 0;
    for (int k = 1; k < nyquist; ++k) {
        double mag = sqrt(re[k] * re[k] + im[k] * im[k]);
        if (k < lo_split)         { sum_low  += mag; ++n_low; }
        else if (k < hi_split)    { sum_mid  += mag; ++n_mid; }
        else                      { sum_high += mag; ++n_high; }
    }
    /* Mean magnitude per band — normalizes for band bin counts. */
    *raw_low  = n_low  ? sum_low  / n_low  : 0.0;
    *raw_mid  = n_mid  ? sum_mid  / n_mid  : 0.0;
    *raw_high = n_high ? sum_high / n_high : 0.0;
}

bool hk_sound_spectrum(double *low, double *mid, double *high)
{
    if (!g_pipe || !g_spec.primed) {
        if (low)  *low = 0.0;
        if (mid)  *mid = 0.0;
        if (high) *high = 0.0;
        return false;
    }
    double raw_low, raw_mid, raw_high;
    spectrum_compute_raw(&raw_low, &raw_mid, &raw_high);
    double l = filter_apply(&g_spec.low,  raw_low,  raw_low  > 0.0);
    double m = filter_apply(&g_spec.mid,  raw_mid,  raw_mid  > 0.0);
    double h = filter_apply(&g_spec.high, raw_high, raw_high > 0.0);
    if (low)  *low  = l;
    if (mid)  *mid  = m;
    if (high) *high = h;
    return true;
}

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
            /* Feed every sample into the spectrum ring as well.
             * Cheap: just a write to a circular buffer. */
            spectrum_push_sample(s);
        }
        double raw_rms =
            (samples > 0) ? sqrt(sumsq / (double)samples) : 0.0;
        return filter_apply(&g_filter, raw_rms, true);
    }
    return filter_apply(&g_filter, 0.0, false);
}
