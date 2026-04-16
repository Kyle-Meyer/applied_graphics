# Ray Tracer Optimization Plan

## High Impact

### 1. Per-mesh BVH ✅ Done
Replaced the linear triangle scan in `RTMeshNode` with a flat midpoint-split binary BVH.
Each mesh builds the BVH once at construction time (`build_bvh()` → `build_bvh_recursive()`).
Traversal is stack-based with front-to-back child ordering for early `closest.t_min` tightening.
Shadow path (`does_intersect_exist`) exits on the first hit.

**Bug fixed during implementation:** `ray.intersect(AABB)` returns `t_max` (exit distance)
when the ray origin is inside the AABB, not `t_min` (entry). Using it as a lower-bound
guard against `closest.t_min` incorrectly pruned rock meshes whose object-space AABB
contained the camera (visible at close range). Fixed by gating inner-node pushes on
`intersects` only; distance is used solely for front-to-back ordering.

**Files:** `RayTracer/rt_mesh_node.cpp`, `RayTracer/rt_mesh_node.hpp`

---

### 2. Thread pool instead of respawn-per-pass ✅ Done
Replaced the per-pass `new std::thread[]` / `join` pattern in `display()` with a
persistent `ThreadPool`. Workers sleep on a condition variable between passes; the main
thread submits row-range lambdas via `submit()` then calls `wait()` to block until the
pass completes. The pool is created once on the first render and lives until process exit,
eliminating 4 × N thread create/join cycles per render.

**Files:** `RayTracer/main.cpp`

---

### 3. Adaptive AA corner reuse ✅ Done
`adaptive_sample` now passes pre-computed corner colors into each recursive call instead
of re-tracing them. Each subdivision level inherits 1 parent corner and shares the center
with its 3 siblings — only 5 new rays are needed per subdivision step instead of 16.
Corner reuse comment in code: old = 84 rays/pixel max, new = 29 rays/pixel max (−65%).

**Files:** `RayTracer/main.cpp`

---

### 4. Precomputed ray reciprocals for BVH traversal ✅ Done
`aabb_entry_t` was computing `1.0f / ray.d.x/y/z` once per internal BVH node visit.
For a rock mesh (~100 internal nodes) with 8 shadow rays, that's ~2400 divisions per
lit pixel just for the reciprocals. Now `find_closest_intersect` and `does_intersect_exist`
each compute `inv_dx/dy/dz` once at the top and pass them into every `aabb_entry_t` call.

**Files:** `RayTracer/rt_mesh_node.cpp`

---

### 5. Shadow BVH traversal consistency ✅ Done
`does_intersect_exist` was using `ray.intersect(AABB)` (the `ray3.cpp` function) for
internal node tests in the shadow path. That function has different distance semantics
(clamps t_min to 0, returns t_max for inside-origin rays) and didn't get the same
entry-distance pruning. Switched to `aabb_entry_t` with precomputed reciprocals,
matching the closest-hit path exactly.

**Files:** `RayTracer/rt_mesh_node.cpp`

---

### 6. Fine-grained task dispatch ✅ Done
Thread pool was given exactly N tasks per render pass (one per worker thread), each
covering `height/N` contiguous rows. Shadow-heavy rows (mid-scene rocks) take
significantly longer than background sky rows, leaving some threads idle while one
finishes late. Now one task is submitted per logical row so the pool self-balances.
For the final pass (768 rows, 7 threads) maximum thread idle time drops from ~109 rows
of potential imbalance to 1 row.

**Files:** `RayTracer/main.cpp`

---

## Medium Impact

### 7. Cache shadow disc basis vectors
`shadow_factor` (`ray_tracer.cpp`) rebuilds `disc_u` and `disc_v` every call via two
cross-products + two normalizes. Since the moon is a fixed directional light, these are
constant per render and could be computed once in `add_light` or cached the first time
they are needed.

**Files:** `RayTracer/ray_tracer.cpp`, `RayTracer/ray_tracer.hpp`

---

### 8. Sand bump trig reduction
`SandBumpNode::sample` (`sand_bump_node.cpp`) calls `sinf`/`cosf` 8+ times per
invocation. Several pairs share the same argument and could be computed with a single
`sincosf`. The primary and secondary ripple contributions could also share the warp
derivatives already computed for the first ripple pass.

**Files:** `RayTracer/sand_bump_node.cpp`

---

### 9. Fix texture fallback logic
`ray_tracer.cpp` checks `if (tex_color.r == 0.0f && tex_color.g == 0.0f &&
tex_color.b == 0.0f)` to decide whether to fall back to UV sampling. This float
equality is unreliable and forces a second `get_color` call for most procedural
textures. The `TextureNode` should expose an `is_procedural()` flag, or the two paths
should be dispatched via virtual dispatch rather than a post-hoc black-pixel check.

**Files:** `RayTracer/ray_tracer.cpp`, `RayTracer/texture_node.hpp`

---

## Lower Impact / Correctness

### 10. `uint16_t` face indices
`RTMeshNode` uses `uint16_t` for face indices (max ~65K vertices). The OBJ loader
silently truncates larger meshes. Switching to `uint32_t` costs nothing in a ray
tracer (no GPU upload) and removes a silent correctness hazard.

**Files:** `RayTracer/rt_mesh_node.hpp`, `RayTracer/rt_mesh_node.cpp`, `RayTracer/main.cpp`

---

### 11. Shadow traversal redundant state construction
Inside `shadow_factor`, a new `SceneState state` is stack-allocated and initialized
for every one of the 8 stratified samples. Only `geometry_node` needs to be set; the
rest is wasted zero-initialization. Hoisting the state construction outside the loop
saves ~7 redundant constructions per shadow test.

**Files:** `RayTracer/ray_tracer.cpp`

---

### 12. Unnecessary normalize calls in the shading path
`ray_tracer.cpp` builds `normal = T*tn.x + B*tn.y + normal*tn.z` from an already-
orthonormal TBN frame. The Gram-Schmidt re-orthogonalization contains a normalize that
could be skipped if `T` is already confirmed unit-length after the transform step.

**Files:** `RayTracer/ray_tracer.cpp`

---

## Recommended Implementation Order

1. **BVH** ✅ — pays dividends on every ray cast (primary + shadow)
2. **Thread pool** ✅ — eliminates repeated thread creation overhead across block passes
3. **Adaptive AA corner reuse** ✅ — cuts sample count significantly in adaptive mode
4. **Precomputed ray reciprocals** ✅ — eliminates per-node divisions in BVH traversal
5. **Shadow BVH consistency** ✅ — correct distance semantics + same reciprocal benefit in shadow path
6. **Fine-grained task dispatch** ✅ — automatic load balancing across shadow-heavy rows
7. **Shadow disc cache** — easy win, single-line fix
8. **Sand bump trig** — micro-optimization for a hot per-pixel path
9. **Texture dispatch** — correctness improvement that also avoids double lookup
10. **uint32_t indices** — correctness fix, trivial change
11. **Shadow state hoisting** — minor cleanup
12. **Normalize audit** — profile first to confirm it's measurable
