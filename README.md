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

Every long option also has a short single-dash form. Use whichever you
prefer:

```sh
./haikalac -l                                       # --list — 100 haiku
./haikalac                                          # animated, random
./haikalac -H old_pond                              # --haiku <id>
./haikalac -H old_pond -n                           # --no-animate (render once)
./haikalac -s medium -b 4                           # --size medium --bpm 4

./haikalac -f                                       # --fractal
./haikalac -f -P ocean                              # --fractal --palette ocean
./haikalac -f -P auto                               # derive palette from haiku words
./haikalac -c                                       # --cycle (hue rotation)
./haikalac -R                                       # --ripple (rings outward)
./haikalac -S                                       # --spin (kaleidoscope)
./haikalac -E                                       # --emanate (hue waves)
./haikalac -e                                       # --no-emoji (color patches)

./haikalac -f -R -S -c -E                           # stack them all

./haikalac -w                                       # --weather (prompts for zip)
./haikalac -w -z 94027                              # explicit zip
./haikalac -a                                       # --sound (mic reactive)
./haikalac -a -g 2.5                                # --sound-gain 2.5
./haikalac -s max -f -S -c                          # fill the terminal
```

### Sample output

`./haikalac -H old_pond -n -s medium` (ANSI stripped for plain
rendering — in your terminal each glyph is colored):

```
                 old pond—
               a frog leaps in
             the sound of water

               — Matsuo Bashō

                 ˙          ·
              ˙   ˙        ·   · ·
           ˙   ˙ 💧˙  ·    💧   · · ·
        ˙       ˙    ░ ˙ · · ·       ·
     ˙   ˙     · · · ·  ░ ˙ ░ · · ·
       · 💧· ˙  ·˙ ·  · ··· · · · ˙💧 ˙
  ·       ░ ˙·░ ˙ ░💦·✦· 💦· · ·˙· ˙   ˙  ·
 · · · · ░ ˙ ░ 💦✦ ˙  · · ·✦·💦 · · ˙   ˙· ·
  · · · ░   ░✦˙ ░ ▒🐸·🐸░🐸· · ✦ · ·      · ·
    💧  ··· · · 🐸·░·❀ ❀ ▒ ░🐸 ░ ˙ · ˙ ·💧
·  ˙   · · ·💦░·▒·░·❀ ░·❀ ░ ▒ ░ 💦░ ˙ · ·
    ˙ ·  ░· ✦ ·🐸 ·❀·░💧·❀·▒·🐸˙✦░ ˙ ░ ·    ˙
 ˙   ˙ · · ·💦 · · ·❀·▒·❀·▒·░·˙ 💦˙ ░    ˙
    💧 ░ · ░ ˙ ░🐸·░·❀·❀· · 🐸· · ··· ˙ 💧
    ·   ˙   ˙✦░·˙·░🐸·🐸 🐸· · ✦ · ·   ˙  · ·
 · · · · ░·˙ ░ 💦✦ ˙  · · ·✦·💦 · ·     ˙· ·
  · · · · ˙ ░·˙ ░ ˙💦·✦·░💦· · · ·        ·
         💧 · · · ···˙ ░ · ░   ░ · 💧· ·
       ˙   ˙   ·˙· ·    ˙ ░ ˙   · · · · ·
            ˙   ˙   ˙░ ˙       · · · ·
         ˙       💧   · · ·💧 ·     ·
                     · · · · · ·
                · · ·      ˙
```

The center bindu is the frog (`🐸`), surrounded by the 8-petal lotus
throne (`❀`), then the inner ring (`🐸 ✿`), middle ring (`💦 ✦`), and
the outer "atmosphere" ring of soft dots (`· ˙ ░ ▒`). In animated mode
the whole thing breathes, the symmetry rotates if `-S` is set, and a
fractal Julia field fills the disc behind it if `-f` is set.

### Flags

| short | long | what it does |
|---|---|---|
| `-h` | `--help` | show help and exit |
| `-l` | `--list` | list available haiku |
| `-H` | `--haiku ID` | render a specific haiku |
| `-s` | `--size SIZE` | small / medium / large / huge / max |
| `-F` | `--fold N` | rotational symmetry (auto, 4..16 even) |
| `-b` | `--bpm N` | breaths per minute |
| `-r` | `--fps N` | frames per second |
| `-n` | `--no-animate` | render once and exit |
| `-e` | `--no-emoji` | colorable Unicode + bg color patches |
| `-f` | `--fractal` | Julia-set background, dihedral folded |
| `-P` | `--palette NAME` | auto / aurora / ember / ocean / forest / sakura / twilight / lava / coral |
| `-c` | `--cycle` | slow hue rotation |
| `-R` | `--ripple` | rings sweep outward from center |
| `-S` | `--spin` | kaleidoscope rotation |
| `-E` | `--emanate` | hue waves with cycling angular symmetry |
| `-w` | `--weather` | derive palette from local weather (needs `curl`) |
| `-z` | `--zip CODE` | postal code for `--weather` |
| `-a` | `--sound` | audio-reactive hue (needs `sox`) |
| `-g` | `--sound-gain N` | multiplier on mic energy |

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
