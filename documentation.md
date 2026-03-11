# Applied Computer Graphics — Assignment 2

JHU 605.767 Applied Computer Graphics
Assignment 2: Hierarchical Bounding Volumes, View Frustum Culling, LOD, and Parametric Surfaces

---

## Overview

This project extends a scene graph framework with four major features:

1. **View Frustum Culling** — skip geometry outside the camera's view
2. **Hierarchical Bounding Volumes** — AABB and bounding sphere acceleration structures
3. **Level of Detail (LOD)** — distance-based geometry selection
4. **Parametric Surfaces** — Bezier patches and Bezier curve animation

The scene graph is shared between two renderers:
- **SampleProject** — real-time OpenGL/SDL rasterizer
- **RayTracer** — offline CPU ray tracer

---

## Build

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Executables land in `build/`:
- `build/SampleProject`
- `build/RayTracer`

---

## Project Structure

| Directory        | Contents |
|------------------|----------|
| `scene/`         | Scene graph nodes shared by both renderers |
| `geometry/`      | Math primitives: Point3, Vector3, Ray3, AABB, BoundingSphere, Plane |
| `SampleProject/` | OpenGL rasterizer entry point and GLSL shaders |
| `RayTracer/`     | Ray tracer entry point and RT-specific geometry nodes |
| `textures/`      | Image texture files |
| `model/`         | OBJ mesh files |

`scene/CMakeLists.txt` uses `file(GLOB *.cpp)`, so new source files are picked up automatically.

---

## Feature 1: View Frustum Culling

**Files:** `scene/view_frustum.hpp`, `scene/view_frustum.cpp`

The `ViewFrustum` class extracts the six frustum planes from the combined view-projection matrix using the Gribb-Hartmann method and tests geometry against them.

### Plane Extraction

Given the combined view-projection matrix M (rows r0..r3), the six planes are:

| Plane  | Coefficients |
|--------|-------------|
| Left   | r3 + r0     |
| Right  | r3 − r0     |
| Bottom | r3 + r1     |
| Top    | r3 − r1     |
| Near   | r3 + r2     |
| Far    | r3 − r2     |

Planes are normalized after extraction so signed distances are in world-space units.

### Intersection Tests

**Bounding Sphere** — signed-distance test against all six planes. A sphere is OUTSIDE if its signed distance to any plane is less than −radius; INSIDE if all distances exceed +radius; INTERSECT otherwise.

**AABB** — P-vertex / N-vertex test. For each plane, the P-vertex is the corner of the AABB most in the direction of the plane normal. If the P-vertex is outside any plane, the box is OUTSIDE. If the N-vertex is inside all planes, the box is INSIDE.

### Integration

`CameraNode::draw()` constructs the frustum from the view-projection matrix and stores it in `SceneState`. During scene traversal:

- `AABBNode::draw()` — tests its bounding box; prints `"CULLED (AABB): <name>"` and skips children if outside
- `BoundingSphereNode::draw()` — tests its bounding sphere; prints `"CULLED (Sphere): <name>"` and skips children if outside

---

## Feature 2: Hierarchical Bounding Volumes

**Files:** `geometry/aabb.hpp`, `geometry/bounding_sphere.hpp`, `scene/bounding_aabb_node.hpp`, `scene/bounding_sphere_node.hpp`

Two bounding volume types are supported. In the rasterizer they act as frustum cull gates; in the ray tracer they serve as acceleration structures.

### AABB

```
AABB(min_pt, max_pt)
AABB(vertex_list)          // computes tight fit
aabb.merge(other_aabb)     // expand to contain both
```

Constructed from explicit corner points or from a vertex list (fits the tightest axis-aligned box).

### Bounding Sphere

```
BoundingSphere(center, radius)
BoundingSphere(vertex_list)   // Ritter's approximate algorithm
sphere.merge_with(other)      // expand to contain both
```

### AABBNode

Wraps one or more child nodes behind an AABB test.

- **Rasterizer:** frustum-culls via `ViewFrustum::intersect(AABB)`
- **Ray tracer:** slab test against the bounding box; skips all children on a miss

### BoundingSphereNode

Wraps children behind a bounding sphere test.

- **Rasterizer:** frustum-culls via `ViewFrustum::intersect(BoundingSphere)`
- **Ray tracer:** sphere-ray intersection test; skips all children on a miss

### Scene Usage

| SampleProject Object | Bounding Volume      |
|----------------------|----------------------|
| Table + items        | `AABBNode`           |
| Box + cone           | `BoundingSphereNode` |
| Vase                 | `AABBNode`           |
| Globe                | `BoundingSphereNode` |
| Painting             | `AABBNode`           |
| Bezier patch         | `BoundingSphereNode` |

---

## Feature 3: Level of Detail

**Files:** `scene/lod_node.hpp`, `scene/lod_node.cpp`

`LODNode` selects one child geometry node to render based on the camera's distance to the object. LOD levels are registered in ascending distance order; the first level whose `max_distance` exceeds the actual distance is used.

```cpp
auto lod = std::make_shared<LODNode>("Globe");
lod->add_level( 60.0f,    high_sphere);   // distance ≤ 60
lod->add_level(120.0f,    mid_sphere);    // distance ≤ 120
lod->add_level(FLT_MAX,   low_sphere);    // everything beyond
```

Every frame the node prints: `LOD: <name> level=N distance=D`

### Distance Computation

| Renderer     | Method |
|--------------|--------|
| Rasterizer   | Extracts camera distance from the translation column of the accumulated model matrix |
| Ray tracer   | Computes distance from the ray origin to the world-space `position_` set via `set_position()` |

### LOD Objects

**SampleProject:**

| Object       | Levels                              | Thresholds      |
|--------------|-------------------------------------|-----------------|
| Globe        | 36×18 → 18×9 → 9×4 sphere subdivisions | dist 60 / 120 / ∞ |
| Bezier patch | High → Mid → Low tessellation      | dist 50 / 100 / ∞ |

**RayTracer:**

| Object  | High Detail | Low Detail | Threshold |
|---------|-------------|------------|-----------|
| NearLOD | Pyramid     | Sphere     | dist 11   |
| FarLOD  | Cube        | Sphere     | dist 11   |

---

## Feature 4: Parametric Surfaces

### 4a. Bicubic Bezier Patch

**Files:** `scene/bezier_patch.hpp`, `scene/bezier_patch.cpp`

`BezierPatchSurface` implements a bicubic Bezier surface defined by a 4×4 grid of control points. It inherits from `TriSurface` for rasterizer rendering and exposes a static `tessellate()` method for the ray tracer.

#### Surface Evaluation

The patch is parameterized over (u, v) ∈ [0,1]²:

```
S(u, v) = Σᵢ Σⱼ B_i(u) · B_j(v) · P[i][j]
```

where B_k(t) are the cubic Bernstein basis polynomials:

```
B₀(t) = (1−t)³
B₁(t) = 3t(1−t)²
B₂(t) = 3t²(1−t)
B₃(t) = t³
```

Control points are stored row-major: `cp[i*4 + j] = P(u_i, v_j)`.

#### Normal Computation

Surface normals are computed analytically from the partial derivatives:

```
∂S/∂u = Σᵢ Σⱼ B'_i(u) · B_j(v) · P[i][j]
∂S/∂v = Σᵢ Σⱼ B_i(u) · B'_j(v) · P[i][j]

N = (∂S/∂u) × (∂S/∂v),  normalized
```

This gives smooth, analytically correct normals at every tessellation level.

#### Tessellation

The patch is tessellated into a quad mesh with `n × n` quads (where n is the subdivision count). Faces are wound CCW using row-major indexing. The outer loop iterates v (rows), the inner loop iterates u (columns), which keeps `∂S/∂u × ∂S/∂v` consistent with CCW winding.

#### Coordinate System Notes

The two renderers use opposite up-axes, so control point layout differs:

| Renderer     | Up axis | u direction | v direction |
|--------------|---------|-------------|-------------|
| SampleProject| Z       | X           | Y           |
| RayTracer    | Y       | Z           | X           |

The ray tracer has backface culling enabled, so normals must face the camera; the control point orientation above ensures this.

#### LOD with Bezier Patches

`BezierPatchSurface` is paired with `LODNode` by constructing multiple instances at different subdivision counts:

```
High detail:   30 subdivisions
Medium detail: 15 subdivisions
Low detail:     8 subdivisions
```

Each level is a separate `BezierPatchSurface`; the `LODNode` selects which one to render.

---

### 4b. Bezier Curve with Forward Differencing

**Files:** `scene/bezier_curve.hpp`, `scene/bezier_curve.cpp`

`BezierCurve` implements cubic Bezier curve animation using forward differencing — a technique that steps along the curve in O(1) time using only additions.

#### Forward Differencing

A cubic Bezier is sampled at t = 0, h, 2h, 3h (where h = 1/steps) to build a Newton forward-difference table:

```
d[0] = P(0)
d[1] = P(h) − P(0)
d[2] = P(2h) − 2·P(h) + P(0)
d[3] = P(3h) − 3·P(2h) + 3·P(h) − P(0)
```

Each call to `step()` advances one sample with three additions per component:

```
d[0] += d[1]
d[1] += d[2]
d[2] += d[3]
```

`d[3]` is the constant third difference for a cubic and never changes. The current position is always `d[0]`.

This gives smooth, exact curve traversal with minimal computation per frame.

#### Direct Evaluation

For pre-computing marker positions (e.g., static spheres placed at known t values), the static method `BezierCurve::evaluate(t, p0, p1, p2, p3)` performs direct Bernstein evaluation.

---

### 4c. Bezier Path Node

**Files:** `scene/bezier_path_node.hpp`, `scene/bezier_path_node.cpp`

`BezierPathNode` combines the `BezierCurve` stepper with scene graph rendering. Each frame it:

1. Renders the full curve as a `GL_LINE_STRIP`
2. Translates all child nodes to the current curve position
3. Advances one step (using forward differencing)
4. Resets and loops when the end is reached

This allows any scene node (sphere, mesh, etc.) to be smoothly animated along a Bezier path.

---

## Ray Tracer Specific Notes

### RTSphereNode — World-Space Center

`RTSphereNode` stores its center as an absolute world-space coordinate. **Do not** wrap it in `RTTransformNode` — that would pass an object-space ray to the world-space sphere and produce incorrect intersections. To animate, call `sphere->set_center(new_pos)` directly.

`RTMeshNode` vertices are in object space, so `RTTransformNode` is correct for meshes.

### Backface Culling

The ray-triangle intersection in `ray3.cpp` uses:
```cpp
if (det < EPSILON) return false;  // backface cull
```

All geometry normals must face toward the camera or the surface will be invisible.

### Coordinate System

The ray tracer uses Y-up. With the camera at `(0, 0.5, -10)` looking toward `+Z` and Y-up:

```
right = view_dir × up  →  points toward −X
```

So screen-right is in the negative-X direction.

---

## RayTracer Scene Objects

| Object          | Description |
|-----------------|-------------|
| Floor           | Large sphere trick (radius 1000, centre y=−1001) |
| NearLOD         | Pyramid (high) or sphere (low), threshold dist=11 |
| FarLOD          | Cube (high) or sphere (low), threshold dist=11 |
| Mirror sphere   | Reflective material, at (−1.8, 0, 3) |
| Test cube       | Green, at (2, −0.5, 2) |
| OffscreenGroup  | AABB culling demo — outside frustum, prints RT CULLED |
| Bezier patch    | Teal dome at (2.5, −1, −2) |
| Bezier curve    | 7 grey marker spheres + 1 orange animated sphere |

**Bezier curve control points:**
```
P0(−1.5, 0.5, 0) → P1(−0.5, 2.0, 3) → P2(0.5, 2.0, 6) → P3(1.5, 0.5, 9)
60 steps total; sweeps from screen-right (near) to screen-left (far)
```

**RayTracer keyboard shortcuts:**
- `n` — advance Bezier curve one step
- `a` — toggle anti-aliasing
- `r / p / h` — roll / pitch / heading camera rotation

---

## SampleProject Keyboard Shortcuts

| Key | Camera target   |
|-----|-----------------|
| `1` | Teapot          |
| `2` | Vase            |
| `3` | Cone            |
| `4` | Globe           |
| `5` | Painting        |
| `6` | Can             |
| `7` | Bezier patch    |
