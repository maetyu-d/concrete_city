# Concrete City

A C++17 / raylib prototype for a first-person procedural brutalist city: rainy, night-lit, textured, and generated deterministically around the player.

The core world rule is still:

```text
world_at_position = f(player_position, world_seed)
```

The world is not stored as a full map. Terrain, towers, streets, lamps, puddles, and objects are generated from deterministic chunk/tile rules. Player-authored changes are stored separately as deltas in `markers.txt`.

## Current Features

- Fullscreen first-person exploration through an infinite generated brutalist city.
- Deterministic chunk generation with stable revisits.
- Psychedelic bleached-painting post-processing, rain direction influenced by camera look, feedback, halftone, wet surfaces, and reflective oily puddles.
- Looping rain soundtrack with crossfade support and a larger stream buffer for smoother playback.
- Runtime shader-resolution menu for balancing image quality and performance.
- Rust-red lamp posts with reduced deterministic density for better readability.
- Debug output for chunk seed and tile type under the player.

## Build And Run

```sh
cd "/Users/user/Documents/PCG RPG"
cmake -S . -B build -G Ninja
cmake --build build
./build/zone_drifter
```

The first configure step downloads raylib 5.5.

If the build directory already exists, this is usually enough:

```sh
cd "/Users/user/Documents/PCG RPG"
cmake --build build
./build/zone_drifter
```

To run the deterministic chunk regeneration check:

```sh
./build/determinism_check
```

## Controls

- `Mouse`: look around
- `WASD`: move and strafe
- `Arrow Left / Arrow Right`: turn with keys
- `M`: open/close shader resolution menu
- `1`: set shader resolution to `1600x900` while the menu is open
- `2`: set shader resolution to `1920x1080` while the menu is open
- `Esc`: close the resolution menu, or quit when the game window handles close input
- `P`: place/remove a persistent marker on the current tile
- `I`: toggle the alternate mesh-style shader view
- `F3`: print current chunk seed and tile type under the player

## Releases

Latest release: [v0.1.1](https://github.com/maetyu-d/concrete_city/releases/tag/v0.1.1)

Repository: [maetyu-d/concrete_city](https://github.com/maetyu-d/concrete_city)
