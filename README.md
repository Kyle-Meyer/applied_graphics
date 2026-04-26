# Moonlit Desert Ruins — JHU 605.767 Final Project

A ray-traced scene demonstrating soft shadows via area light sampling, normal mapping with TBN,
adaptive super-sampling, and procedural materials/geometry. A pale moon illuminates a stone
obelisk surrounded by partially buried rocks on a sparkling sand floor.

---

## Build

Requires CMake 3.15+, a C++17 compiler, SDL2, and Assimp.

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Run the ray tracer:

```bash
./build/RayTracer
```

The window opens and immediately begins rendering. Progressive refinement displays coarse
blocks first, then refines to full resolution.

---

## Techniques Implemented

| Technique | Description |
|-----------|-------------|
| **Soft shadows** | Area light disc sampling — N=8 stratified rays toward the moon disc per hit point; shadow factor is the lit fraction |
| **Normal mapping** | TBN-space perturbation at each hit point; obelisk uses a granite image map, sand uses a procedural ripple + grain shader (`SandBumpNode`) |
| **Adaptive super-sampling** | Recursive pixel subdivision triggered by luminance contrast; up to 16 samples at edges, 1 in flat regions |
| **Hierarchical BV culling** | Three `AABBNode` groups (NearRocks, MidRocks, FarRocks) short-circuit intersection traversal |
| **LOD** | Near/mid rocks degrade from OBJ mesh to sphere beyond their distance threshold |
| **Procedural textures** | `DesertRockTexture` (fBm noise → two-layer colour mix) on rocks and obelisk; `SandBumpNode` (domain-warped sine ripples + hash grain) on the floor |
| **Dune height field** | `HeightFieldFloorNode` ray-marches a two-octave procedural dune surface with an asymmetric windward/leeward profile |
| **Sand sparkle** | Hash-based micro-facet glint — view-dependent half-vector alignment simulates moonlit quartz |

---

## Keyboard Controls

### Camera Presets
| Key | View |
|-----|------|
| `1` | Wide shot — full obelisk and scene |
| `2` | Ground level — sand sparkle and shadow drama |
| `3` | Close rock — foreground detail and normal map |

### Camera Rotation
| Key | Action |
|-----|--------|
| `r` / `R` | Roll +5° / −5° |
| `p` / `P` | Pitch +5° / −5° |
| `h` / `H` | Heading +5° / −5° |

### Technique Toggles
| Key | Toggle |
|-----|--------|
| `s` | Soft shadows (8 rays) ↔ hard shadows (1 ray) |
| `m` | Normal map on ↔ off |
| `k` | Sand sparkle on ↔ off |
| `a` | Anti-aliasing: off → uniform 2×2 → adaptive (cycles) |
| `g` | Atmospheric glow on ↔ off |
| `Esc` | Quit |

Each toggle prints its new state to the console and re-renders immediately — useful for
before/after comparisons during the presentation.

---

## Project Structure

```
RayTracer/
  main.cpp                  Scene definition and render loop
  ray_tracer.cpp/.hpp       Recursive ray tracer, soft shadow sampling
  height_field_floor_node   Procedural dune height field (ray march + bisect)
  sand_bump_node            Procedural sand normal map (ripples + grain)
  desert_rock_texture       Procedural fBm rock/obelisk albedo
  normal_map_node           TBN normal-map infrastructure
  material_node             Cook-Torrance material + sparkle
scene/
  lod_node                  Distance-based LOD selection
  bounding_aabb_node        AABB ray-intersection culling
model/
  rock1.obj … rock9.obj     Rock meshes (OBJ)
  obelisk_*.obj             Obelisk mesh sections
textures/
  stone_normal.png          Granite normal map (obelisk)
```

---

## Notes

- Rendering is multi-threaded (one worker per logical core minus one for the main thread).
  A 1024×768 render with soft shadows and AA takes roughly 15–60 seconds depending on hardware.
- The scene uses Y-up coordinates. Camera starts at the wide-shot preset (key `1`).
- Console output shows LOD level selections, AABB culling events, and adaptive AA sample
  statistics after each render pass.
