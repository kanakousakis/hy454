# Sonic Engine - CS454 

Sonic the Hedgehog game engine in C++ using SFML.

---

## Quick Start

### Install SFML

**Linux:**
```bash
sudo apt-get install libsfml-dev    # Ubuntu/Debian
sudo dnf install SFML-devel         # Fedora
sudo pacman -S sfml                 # Arch
```

**macOS:**
```bash
brew install sfml
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-sfml mingw-w64-x86_64-gcc
```

### Build and Run

```bash
make
./bin/SonicGame
```

---

## Controls

- **Arrow Keys / WASD** - Move
- **Space / Up / W** - Jump
- **Down / S** - Crouch / Roll
- **Down + Jump** - Spindash
- **ESC** - Pause
- **Q (hold)** - Quit
- **G** - Toggle grid
- **G (hold)** - Toggle god mode
- **F1** - Toggle debug
- **U / Mouse left click (hold)** - Look up

---

## Build Commands

```bash
make          # Build project
make run      # Build and run
make clean    # Remove build files
make help     # Show help
```

---

## Project Structure

```
sonic_engine/
├── App/              # Game logic (Sonic, enemies, rings, etc.)
├── Engine/           # Core engine (animation, tiles, sprites, etc.)
├── assets/           # Graphics, maps, sounds
├── Makefile          # Build system
└── README.md
```

---

## Features

- Tile-based terrain with collision detection
- Parallax scrolling background
- Sonic physics (acceleration, jumping, rolling, spindash)
- Ring system with scatter on damage
- Multiple enemy types 
- Springs, spikes, checkpoints, monitors
- Power-ups (shield, speed shoes, invincibility e.t.c)
- HUD with score, rings, time, lives
- Pause menu and game over screen
- Everything from pdf implemented as instructed +more
---

## Troubleshooting

**"SFML not found"**  
→ Install SFML 

**"undefined reference to SFML"**  
→ Install pkg-config: `sudo apt-get install pkg-config`

**Black screen**  
→ Run from project root, not from `bin/` directory

**Low FPS**  
→ Turn off grid (G) and debug mode (F1)

---

## Requirements

- C++17 compiler (GCC 9+ / Clang 10+)
- SFML 2.5+
- Make or CMake

---

Built with zero warnings using `-Wall -Wextra`.
