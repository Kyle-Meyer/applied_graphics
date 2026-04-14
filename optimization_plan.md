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

### 3. Adaptive AA corner reuse
`adaptive_sample` (`main.cpp`) samples all 4 corners every call, then recurses into
quadrants that re-sample those same corners. Each parent corner is re-traced 2–3 times.
Passing the already-computed corner colors into the recursive call would eliminate ~75%
of redundant samples in adaptive mode.

**Files:** `RayTracer/main.cpp`

---

## Medium Impact

### 4. Cache shadow disc basis vectors
`shadow_factor` (`ray_tracer.cpp`) rebuilds `disc_u` and `disc_v` every call via two
cross-products + two normalizes. Since the moon is a fixed directional light, these are
constant per render and could be computed once in `add_light` or cached the first time
they are needed.

**Files:** `RayTracer/ray_tracer.cpp`, `RayTracer/ray_tracer.hpp`

---

### 5. Sand bump trig reduction
`SandBumpNode::sample` (`sand_bump_node.cpp`) calls `sinf`/`cosf` 8+ times per
invocation. Several pairs share the same argument and could be computed with a single
`sincosf`. The primary and secondary ripple contributions could also share the warp
derivatives already computed for the first ripple pass.

**Files:** `RayTracer/sand_bump_node.cpp`

---

### 6. Fix texture fallback logic
`ray_tracer.cpp` checks `if (tex_color.r == 0.0f && tex_color.g == 0.0f &&
tex_color.b == 0.0f)` to decide whether to fall back to UV sampling. This float
equality is unreliable and forces a second `get_color` call for most procedural
textures. The `TextureNode` should expose an `is_procedural()` flag, or the two paths
should be dispatched via virtual dispatch rather than a post-hoc black-pixel check.

**Files:** `RayTracer/ray_tracer.cpp`, `RayTracer/texture_node.hpp`

---

## Lower Impact / Correctness

### 7. `uint16_t` face indices
`RTMeshNode` uses `uint16_t` for face indices (max ~65K vertices). The OBJ loader
silently truncates larger meshes. Switching to `uint32_t` costs nothing in a ray
tracer (no GPU upload) and removes a silent correctness hazard.

**Files:** `RayTracer/rt_mesh_node.hpp`, `RayTracer/rt_mesh_node.cpp`, `RayTracer/main.cpp`

---

### 8. Shadow traversal redundant state construction
Inside `shadow_factor`, a new `SceneState state` is stack-allocated and initialized
for every one of the 8 stratified samples. Only `geometry_node` needs to be set; the
rest is wasted zero-initialization. Hoisting the state construction outside the loop
saves ~7 redundant constructions per shadow test.

**Files:** `RayTracer/ray_tracer.cpp`

---

### 9. Unnecessary normalize calls in the shading path
`ray_tracer.cpp` builds `normal = T*tn.x + B*tn.y + normal*tn.z` from an already-
orthonormal TBN frame. The Gram-Schmidt re-orthogonalization contains a normalize that
could be skipped if `T` is already confirmed unit-length after the transform step.

**Files:** `RayTracer/ray_tracer.cpp`

---

## Recommended Implementation Order

1. **BVH** ✅ — pays dividends on every ray cast (primary + shadow)
2. **Thread pool** ✅ — eliminates repeated thread creation overhead across block passes
3. **Adaptive AA corner reuse** — cuts sample count significantly in adaptive mode
4. **Shadow disc cache** — easy win, single-line fix
5. **Sand bump trig** — micro-optimization for a hot per-pixel path
6. **Texture dispatch** — correctness improvement that also avoids double lookup
7. **uint32_t indices** — correctness fix, trivial change
8. **Shadow state hoisting** — minor cleanup
9. **Normalize audit** — profile first to confirm it's measurable
