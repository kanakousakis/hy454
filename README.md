# Sonic Engine - HY454

2D Sonic-style game project in C++ using SFML.

## Repository Layout

The actual project lives inside the `sonic_engine/` subdirectory.

```text
hy454/
|-- README.md
|-- Project - Sonic_V3.pdf
`-- sonic_engine/
    |-- App/
    |-- Engine/
    |-- assets/
    |-- Makefile
    `-- CMakeLists.txt
```

## Requirements

- C++17 compiler
- SFML 2.5+
- `make`

## Install SFML

### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install libsfml-dev pkg-config
```

### Fedora

```bash
sudo dnf install SFML-devel pkgconf-pkg-config
```

### Arch

```bash
sudo pacman -S sfml pkgconf
```

### macOS

```bash
brew install sfml pkg-config
```

### Windows

Use MSYS2 and install the MinGW SFML toolchain:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sfml
```

## Build

From the repository root:

```bash
cd sonic_engine && make
```

## Run

From the repository root:

```bash
cd sonic_engine && ./bin/SonicGame
```

Or build and run in one step:

```bash
cd sonic_engine && make run
```

## Useful Commands

```bash
cd sonic_engine && make
cd sonic_engine && make run
cd sonic_engine && make clean
cd sonic_engine && make help
```

## Controls

- Arrow Keys / WASD: Move
- Space / Up / W: Jump
- Down / S: Crouch / Roll
- Down + Jump: Spindash
- Esc: Pause
- Q (hold): Quit
- G: Toggle grid
- G (hold): Toggle god mode
- F1: Toggle debug
- U / Left mouse button (hold): Look up

## Features

- Tile-based terrain and collision handling
- Animation and sprite systems
- Parallax background rendering
- Sonic movement, jumping, rolling, and spindash
- Rings, enemies, checkpoints, springs, spikes, and monitors
- HUD, pause flow, and game-over flow
- Audio and map assets included in the project

## Troubleshooting

### SFML not found

Install SFML and `pkg-config` first.

### Undefined SFML references

Make sure the SFML development libraries are installed, not just the runtime.

### Black screen or missing assets

Run the game from inside `sonic_engine/` so relative asset paths resolve correctly.

### Slow WSL filesystem access

Building under `/mnt/c/...` in WSL can be slower than native Linux paths.
