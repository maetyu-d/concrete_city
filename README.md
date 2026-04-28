# Zone Drifter Prototype

Deterministic post-apocalyptic exploration prototype using C++17 and raylib.

The world is generated from:

```text
world_at_position = f(player_position, world_seed)
```

Chunks are generated on demand and unloaded when far away. Player markers are stored as deltas in `markers.txt`; the generated world itself is not saved.

The game supports both a top-down exploration view and a Minecraft-style first-person view over the same deterministic chunk data.

## Build and run

```sh
cmake -S . -B build -G Ninja
cmake --build build
./build/zone_drifter
```

The first configure step downloads raylib 5.5.

To run the deterministic chunk regeneration check:

```sh
./build/determinism_check
```

## Controls

- `WASD` or arrow keys: move
- `Tab`: switch between top-down and Minecraft-style first-person view
- First-person view: `W/S` or up/down move forward/back, `A/D` strafe, left/right arrows turn
- `M`: place/remove a marker on the current tile
- `F3`: print current chunk seed and tile type under the player
- `Esc`: quit
