# HaikalaC

A breathing haiku mandala for the terminal — pure C99, no third-party
dependencies. A port of [haikala](https://github.com/SpaceTrucker2196/haikala)
(the Python version) aimed at maximum portability: anywhere you have a
C99 toolchain and a POSIX-ish system, this should compile in seconds.

## Build

```sh
make
./haikalac
```

That's it. No package manager, no virtualenv, no submodules.

Tested on macOS (clang) and Linux (gcc). Other POSIX systems with a
C99 compiler, libc, `termios.h`, and `sys/ioctl.h` should work.

## Run

```sh
./haikalac --list                              # show available haiku
./haikalac                                     # animated, random, huge size
./haikalac --haiku old_pond                    # specific haiku
./haikalac --haiku old_pond --no-animate       # render once and exit
./haikalac --size medium --bpm 4               # smaller, slower breath
./haikalac --fold 12                           # higher rotational symmetry
```

`q`, `Esc`, or `Ctrl-C` exits the animation cleanly (the alternate
screen buffer is restored, raw mode is undone).

## What's in this initial port

- 10 curated haiku (Bashō, Buson, Chiyo-ni)
- 36 tokens with full glyph vocabularies
- Spec construction with banded rings (inner/middle/outer)
- Lotus throne ring (Buddhist mandala detail)
- Static fullscreen backdrop: sparse star/dot field, cardinal axis
  lines, diagonal wheel-spoke hints, four lotus-gate toranas
- Breath animation (sinusoidal radius/density modulation)
- Truecolor (24-bit) ANSI output
- Raw-mode terminal handling with clean restore on signal
- Build & smoke test suite (`make test`)

## What's not (yet) ported from Python upstream

- Fractal background (Julia set with dihedral folding)
- Hue cycling (`--cycle`)
- Ripple effect (`--ripple`)
- Kaleidoscope spin (`--spin`)
- Emanating hue waves (`--emanate`)
- `--no-emoji` mode and cell background tints
- Word-derived auto palette (`--palette auto`)
- The other 90 curated haiku
- Per-ring varying breath speeds

These slot in as discrete additions; see the matching modules in
`mandala.c` / `animate.c` for the integration points.

## Layout

```
HaikalaC/
├── Makefile
├── include/
│   └── haikala.h           # all public types and prototypes
├── src/
│   ├── main.c              # argv parsing, dispatch
│   ├── animate.c           # frame composer + animation loop
│   ├── terminal.c          # termios + ANSI escape control
│   ├── render.c            # Cell grid → ANSI string
│   ├── mandala.c           # ring placement, backdrop, glyph helpers
│   ├── translate.c         # haiku → MandalaSpec
│   ├── symbols.c           # token → glyph table
│   └── haiku.c             # curated haiku table
└── tests/
    └── test_basic.c        # smoke tests for the deterministic pipeline
```

## Compatibility notes

- **UTF-8**: glyphs are stored and emitted as raw UTF-8 byte
  sequences. Your terminal must be UTF-8 capable (every modern terminal
  is, by default).
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
