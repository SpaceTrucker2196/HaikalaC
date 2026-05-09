# HaikalaC

A breathing haiku mandala for the terminal — pure C99, no third-party
dependencies. A 1:1 port of [haikala](https://github.com/SpaceTrucker2196/haikala)
(the Python version) aimed at maximum portability: anywhere you have a
C99 toolchain, libc, and POSIX termios/ioctl, this should compile in
seconds.

## Build

```sh
make
./haikalac
```

That's it. No package manager, no virtualenv, no submodules.

Tested on macOS (clang) and Linux (gcc). Other POSIX systems with a
C99 compiler should work.

## Run

```sh
./haikalac --list                                   # 100 haiku
./haikalac                                          # animated, random
./haikalac --haiku old_pond                         # specific haiku
./haikalac --haiku old_pond --no-animate            # render once and exit
./haikalac --size medium --bpm 4                    # smaller, slower

./haikalac --animate --fractal                      # Julia-set background
./haikalac --animate --fractal --palette ocean      # named palette
./haikalac --animate --fractal --palette auto       # derive from haiku words
./haikalac --animate --cycle                        # slow hue rotation
./haikalac --animate --ripple                       # rings sweep outward
./haikalac --animate --spin                         # kaleidoscope mode
./haikalac --animate --emanate                      # hue waves with cycling symmetry
./haikalac --animate --no-emoji                     # block/dingbat + bg tints

./haikalac --animate --fractal --ripple --spin --cycle --emanate

./haikalac --weather                                # prompts for zip
./haikalac --weather --zip 94027                    # explicit, skip prompt
```

### Weather mode

`--weather` fetches the current condition for the given postal code via
`wttr.in` (no API key required) and builds a fractal palette from a
4-season base ramp (spring/summer/autumn/winter, picked from the local
clock) plus a condition modifier (clear brightens, cloudy desaturates,
rain/storm shift hue + darken, snow brightens + desaturates, fog flattens).

Network is delegated to `curl` via `popen` — keeps HaikalaC itself free
of any C-level networking dependency. The zip is sanitized to
alphanumeric + hyphens (≤ 16 chars) before being interpolated.

### Sound mode

`--sound` makes the colors react to your microphone. Audio capture is
delegated to `sox` (raw 16-bit signed mono PCM @ 22.05 kHz over a
non-blocking pipe). Each frame, HaikalaC drains whatever bytes are
buffered, computes RMS amplitude, smooths it with an exponential
moving average, and adds a hue rotation proportional to the level —
louder = faster color rotation, with a slow ringing decay so beats
trail visually.

Install sox if missing:
- macOS:   `brew install sox`
- Debian:  `apt install sox`
- Fedora:  `dnf install sox`

Without sox, `--sound` prints a single-line note and continues without
audio reactivity. `--sound-gain <n>` (default 1.0) multiplies the
energy if the response is too subtle or too aggressive for the room.

`q`, `Esc`, or `Ctrl-C` exits the animation cleanly (alternate screen
buffer is restored, raw mode is undone).

## Feature parity with Python upstream

All animations and modes from the Python `haikala` are present:

| Feature                       | Status |
|-------------------------------|--------|
| 100 curated haiku             | ✓ |
| ~150 token → glyph vocabulary | ✓ |
| Banded ring layout (inner/middle/outer) | ✓ |
| Lotus throne ring (Buddhist mandala detail) | ✓ |
| Static fullscreen backdrop with axis lines + lotus gates | ✓ |
| Sinusoidal breath animation   | ✓ |
| Per-ring varying breath speed (cascading wave) | ✓ |
| Hue cycling (`--cycle`)       | ✓ |
| Fractal Julia-set background, dihedral folded | ✓ |
| 8 named palettes (aurora/ember/ocean/forest/sakura/twilight/lava/coral) | ✓ |
| Auto palette derived from haiku words | ✓ |
| Auto fold derived from word/token counts | ✓ |
| Ripple effect (transient rings sweeping outward) | ✓ |
| Spin / kaleidoscope (per-ring rotation, alternating directions) | ✓ |
| Emanate (hue waves with cycling angular symmetry) | ✓ |
| `--no-emoji` mode + cell background tints | ✓ |
| Truecolor (24-bit) ANSI       | ✓ |
| Build & smoke test suite      | ✓ |

## Layout

```
HaikalaC/
├── Makefile                 # single-command build
├── include/
│   └── haikala.h            # all public types + prototypes
├── src/
│   ├── main.c               # argv parser, dispatch
│   ├── animate.c            # per-frame composer + loop
│   ├── terminal.c           # termios + ANSI escape control
│   ├── render.c             # Cell grid → ANSI string, hue rotation,
│   │                        #   bg color emission, emanate field
│   ├── mandala.c            # ring placement, breath, spin, ripple,
│   │                        #   fractal field, backdrop
│   ├── translate.c          # haiku → MandalaSpec, no-emoji + bg tints
│   ├── symbols.c            # token → glyph table (~150 entries)
│   ├── palette.c            # RGB↔HLS, fractal palettes, word→color,
│   │                        #   auto-fold, auto-palette
│   └── haiku.c              # 100 curated haiku
└── tests/
    └── test_basic.c         # 14 smoke tests
```

## Compatibility notes

- **UTF-8**: glyphs are stored and emitted as raw UTF-8 byte
  sequences. Your terminal must be UTF-8 capable.
- **Truecolor**: every reasonable modern terminal supports 24-bit
  ANSI (macOS Terminal, iTerm2, kitty, alacritty, wezterm, GNOME
  Terminal, Windows Terminal). If yours doesn't, colors will look
  approximate.
- **Wide glyphs**: emoji and CJK characters are detected and reserve
  two cells. The right cell is sentinel-marked so the renderer skips
  it on output.
- **Signals**: `SIGINT` and `SIGTERM` set a quit flag. `atexit`
  handles the cleanup. Killing with `SIGKILL` (kill -9) will leave
  your terminal in raw mode — recover with `reset` or `stty sane`.

## License

MIT (matching the Python upstream).
