# Final Project Timeline
**Project:** Normal Mapping + Soft Shadows (Ray Tracer)
**Start:** 2026-03-17 | **Presentations:** ~2026-04-21 (5 weeks)

**Scene:** *Moonlit Desert Ruins* — a smooth stone obelisk surrounded by partially buried rocks
scattered across a sparkling sand floor. A pale blue-white moon provides directional soft
shadows. LOD degrades distant rocks to ellipsoids/spheres. Spatial AABB groups demonstrate
hierarchical BV culling.

> **Note on shadow maps:** Shadow maps are a rasterization workaround for a problem ray
> tracing solves naturally (shadow rays). The technique demonstrated here is **soft shadows
> via area light sampling** — multiple shadow rays per hit point, averaged — which is the
> ray-tracing equivalent and arguably more interesting. Flag this tradeoff with the
> instructor if the project brief requires shadow maps specifically.

---

## Week 1 (Mar 17–23) — Scene Layout + Geometry

**Goal:** All scene objects placed and rendering (flat shading, no shadows yet).

- [ ] Lock in scene layout — positions for obelisk, hero rocks (foreground), mid rocks, far rocks
  - Obelisk at world origin, slightly offset so its shadow sweeps diagonally across sand
  - Moon direction: roughly `(-0.6, -0.8, 0.4)` normalized — low angle, maximizes shadow length
  - Hero rocks within ~10 units; mid group 15–35 units; far group 35–60 units
- [ ] Design obelisk Bezier patch control points (4 longitudinal patches, oval cross-section,
  tapering toward apex) — derive from existing `BezierPatchSurface` infrastructure
- [ ] Source one good high-poly rock OBJ (500–2000 triangles) for foreground hero rocks
  - Morgan McGuire's graphics archive or Kenney.nl are good sources
- [ ] Build `RTMeshNode` entries for the sand floor (large quad), obelisk patches, and hero rocks
- [ ] Set background color to deep blue-black (`vec3(0.02, 0.04, 0.08)`)
- [ ] Confirm scene renders without crashes; objects appear in correct positions

**Milestone:** Full scene visible with flat/ambient shading; all geometry placed correctly.

---

## Week 2 (Mar 24–30) — LOD + Hierarchical BV Culling

**Goal:** Rocks use LOD; spatial AABB groups cull correctly when camera moves.

- [ ] Wire `LODNode` for each rock using three levels:
  - HIGH (`dist < 15`): detailed rock OBJ via `RTMeshNode`
  - MED  (`dist < 40`): simpler rock OBJ or scaled `RTSphereNode` with slight ellipsoid feel
  - LOW  (`dist ≥ 40`): `RTSphereNode` — indistinguishable at that distance
- [ ] Build spatial AABB hierarchy over rock groups:
  ```
  SceneRoot
  └── AABBNode "Near Rocks"   (covers foreground cluster)
      ├── LODNode "Rock_A"
      └── LODNode "Rock_B"
  └── AABBNode "Mid Rocks"    (covers middle-distance cluster)
      ├── LODNode "Rock_C"
      └── ...
  └── AABBNode "Far Rocks"    (covers distant cluster)
      └── ...
  ```
- [ ] Verify culling messages print when camera looks away from each group
- [ ] Verify LOD level switches print at correct distance thresholds
- [ ] Obelisk gets its own `BoundingSphereNode` (or `AABBNode`) wrapper

**Milestone:** Culling and LOD both confirmed working via console output.

---

## Week 3 (Apr 1–7) — Soft Shadows via Area Light Sampling

**Goal:** Obelisk and rocks cast soft-edged shadows on the sand.

- [ ] Extend `RTLight` (or add `RTAreaLight`) to represent the moon as a disc light
  - Store center direction + disc radius (small radius = hard shadows, larger = softer)
  - `sample_direction(hit_point)` returns a random point on the disc
- [ ] In the shading loop, cast N shadow rays (start with N=4, tune later) toward random
  points on the moon disc; average the binary occluded/unoccluded results
  - Shadow factor 0.0 = fully in shadow, 1.0 = fully lit, 0.x = soft penumbra
- [ ] Moon light color: `vec3(0.7, 0.85, 1.0)` — pale blue-white
- [ ] Ambient: `vec3(0.02, 0.04, 0.12)` — faint blue night sky fill
- [ ] Test: obelisk shadow should sweep diagonally across sand; rocks cast smaller shadows

**Milestone:** Visible soft-edged shadows; penumbra width tuned to look natural.

---

## Week 4 (Apr 8–14) — Normal Mapping + Sand Sparkle + Adaptive Super-Sampling

**Goal:** Obelisk surface has stone texture detail; sand sparkles blue under moonlight; edges and high-contrast regions are crisp without uniform oversampling.

### Normal mapping in the ray tracer
At each mesh/patch hit point, before shading:
1. Look up UV coordinates (already available from `RTMeshNode` / Bezier hit)
2. Sample a normal map texture; remap `[0,1] → [-1,1]`
3. Build TBN from interpolated vertex tangent + bitangent + normal
4. Transform sampled normal to world space; use for all lighting calculations

- [ ] Add tangent attribute to `RTMeshNode` vertex data — compute from UV deltas per triangle
  during OBJ load (same math as rasterizer TBN, just stored differently)
- [ ] For `BezierPatchSurface` hits, tangent/bitangent come free from `dS/du` and `dS/dv`
  — wire them through to the hit record
- [ ] Apply granite/basalt normal map to obelisk — keep perturbation subtle to preserve
  the "smooth, glistening" feel; high specular exponent (~200)
- [ ] Apply fine sand grain normal map to floor

### Sand sparkle material
At each sand hit point, after normal perturbation:
```
float grain = fract(sin(dot(hit_uv * 80.0, vec2(12.9898, 78.233))) * 43758.5453);
float sparkle = step(0.97, grain);           // ~3% of grains catch light
vec3  sparkle_color = vec3(0.3, 0.6, 1.0) * sparkle * shadow_factor;
```
- [ ] Implement sparkle term in sand material's `shade()` method
- [ ] Tune density threshold and color until it reads as moonlit quartz glinting

### Adaptive super-sampling
Standard uniform AA casts N×N rays per pixel everywhere — expensive. Adaptive
super-sampling concentrates samples only where they matter (object edges, shadow
penumbrae, sparkle glints) and uses a single ray where the image is smooth.

Algorithm:
1. **Corner sample pass** — cast one ray at each corner of the pixel (4 rays).
   Compute luminance of each result.
2. **Contrast test** — if `max(luminance) − min(luminance) > threshold` the pixel
   needs more samples; otherwise average the 4 corners and move on.
3. **Recursive subdivision** — split the pixel into 4 sub-pixels; recurse on each
   (up to `max_depth = 2`, giving at most 4² = 16 samples per pixel).
4. Final pixel color = average of all leaf samples.

```
// Pseudocode for one pixel
Color adaptive_sample(float x, float y, float size, int depth) {
    Color c[4] = { trace(x,       y      ),   // bottom-left
                   trace(x+size,  y      ),   // bottom-right
                   trace(x,       y+size ),   // top-left
                   trace(x+size,  y+size ) }; // top-right
    if (depth >= max_depth || contrast(c) < threshold)
        return average(c);
    float half = size * 0.5f;
    return 0.25f * (adaptive_sample(x,      y,      half, depth+1) +
                    adaptive_sample(x+half, y,      half, depth+1) +
                    adaptive_sample(x,      y+half, half, depth+1) +
                    adaptive_sample(x+half, y+half, half, depth+1));
}
```

- [ ] Implement `adaptive_sample()` in `RayTracer` — share the pixel-sampling path
  with the existing uniform AA so both modes call a common `trace_pixel()` helper
- [ ] Contrast metric: `max_luminance − min_luminance`; threshold ≈ 0.1 (tune)
- [ ] `max_depth = 2` (up to 16 samples at edges, 4 samples in flat regions)
- [ ] Add `a` key cycle: **off → uniform 2×2 → adaptive** (3-way toggle)
  - Off: 1 centre ray per pixel
  - Uniform 2×2: 4 jittered rays per pixel (current behaviour)
  - Adaptive: recursive subdivision with contrast test
- [ ] Print sample statistics (avg samples/pixel) to console so contrast can be
  shown in the presentation
- [ ] Verify: edges around obelisk and shadow penumbra show crisp AA; open sky
  (constant colour) uses only 4 samples

**Milestone:** Obelisk looks carved and polished; sand has scattered blue-white glints; silhouette edges are alias-free with no perceptible cost in flat regions.

---

## Week 5 (Apr 15–21) — Polish, Documentation, Presentation

**Goal:** Polished deliverable ready for class.

- [ ] Camera presets (`1`–`3` keys):
  - `1` Wide shot — full obelisk, shadow sweep across sand, rocks in foreground
  - `2` Ground-level shot — camera near sand surface looking up at obelisk; maximizes sparkle and shadow drama
  - `3` Close rock shot — foreground hero rock with normal map detail visible
- [ ] Toggle keys:
  - `m` — toggle normal map on/off (before/after comparison for slides)
  - `k` — toggle sparkle on/off
  - `s` — toggle soft shadows → hard shadows (shows N=1 vs N=4+ shadow rays)
  - `a` — cycle AA mode: off → uniform 2×2 → adaptive super-sampling
- [ ] Write README updates (scene description, new keyboard commands, build instructions)
- [ ] Create 5–7 slide presentation:
  - Slide 1: title + scene concept (sketch or early screenshot)
  - Slide 2: soft shadows — area light sampling algorithm, shadow ray diagram
  - Slide 3: hard vs. soft shadow comparison screenshots
  - Slide 4: normal mapping in a ray tracer — TBN at hit point diagram
  - Slide 5: normal map on/off comparison (obelisk + sand)
  - Slide 6: adaptive super-sampling — algorithm diagram, contrast metric, sample-count heatmap
  - Slide 7: AA comparison: off / uniform / adaptive — closeup of obelisk edge + sample count stats
  - Slide 8: sand sparkle + LOD + BV culling overview
- [ ] Take all screenshots for slides + README
- [ ] Final build test on clean checkout; verify no stray asset paths

**Milestone:** Presentation ready; code submitted before class day.

---

## Risk / Contingency

| Risk | Mitigation |
|------|------------|
| Area light sampling is noisy at low N | Cap N=8, accept slight noise — frame it as "the nature of Monte Carlo sampling" in slides |
| Rock OBJ tangent computation is fiddly | Skip normal map on rocks; apply only to obelisk (Bezier tangents are analytic and reliable) |
| Bezier patch obelisk geometry is hard to get right | Fall back to a simple tapered box mesh built from triangles — still demonstrates normal mapping |
| Sand sparkle looks like noise, not glints | Increase hash threshold (fewer, brighter glints) and add a view-dependent falloff so only near-grazing angles sparkle |
| Instructor requires shadow maps specifically | Implement shadow map in SampleProject as a separate demo; ray tracer scene demonstrates equivalent technique |
| Adaptive sampling is slow to converge | Cap `max_depth=2` (16 samples max); the contrast threshold avoids spending samples in smooth sky regions |
| Running out of time | Drop sparkle; hard shadows + normal mapping + adaptive AA is still a three-technique demo |

---

## Key Dates

| Date | Event |
|------|-------|
| 2026-03-17 | Project starts; submit idea to instructor — flag shadow ray vs. shadow map question |
| 2026-03-23 | All scene geometry placed and rendering |
| 2026-03-30 | LOD + BV culling verified |
| 2026-04-07 | Soft shadows working; shadow quality tuned |
| 2026-04-14 | Normal mapping + sand sparkle working |
| 2026-04-21 | Code submitted; presentation ready |
