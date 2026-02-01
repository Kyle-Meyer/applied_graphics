# Applied Graphics - Ray Tracing Assignment

## Current Status (Updated Feb 1, 2026)

**Working:**
- Basic ray tracing with Phong-Blinn lighting and shadows
- Reflection (mirror spheres working)
- Refraction (implemented, but solid glass spheres act as ball lenses - physically correct but may look "reflective" due to image inversion)

**Test scene:** Red sphere, mirror sphere, glass sphere, green sphere (behind glass), gray floor, one light source.

---

## Code Components

### 1. scene/camera_node.* - COMPLETE ✓
Added methods to set the view volume, store image plane information, and construct a ray through a pixel position.

**Methods completed:**
- `set_view_volume`
- `construct_ray`
- `set_image_plane_dimensions`

### 2. scene/geometry_node.* - COMPLETE ✓
Base class has default implementations. RTSphereNode provides proper overrides:
- `get_normal` - returns normalized vector from center to intersection
- `get_texture_coord` - uses spherical mapping (theta/phi to s/t)

### 3. scene/bounding* (Bounding Volume Nodes) - PROVIDED ✓
- `BoundingSphereNode`
- `BoundingAABBNode`

Complete for ray tracing. If a ray does not intersect the BV, sub-tree intersection tests are skipped.

### 4. RayTracer/ray_tracer.* - MOSTLY COMPLETE

**Completed:**
- `add_light()` - register lights with ray tracer
- `in_shadow()` - shadow ray casting
- `trace_ray()`:
  - Finds closest intersection
  - Computes ambient + emission + diffuse + specular
  - Spawns reflected rays for reflective materials
  - Spawns refracted rays for transparent materials
  - Handles total internal reflection

**Still TODO:**
- [ ] Get texture color from intersected object and modulate
- [x] Check max depth / attenuation threshold before spawning rays
- [x] Spawn reflected ray if material is reflective
- [x] Spawn transmitted ray if material is transparent

### 5. RayTracer/ray.* - COMPLETE ✓
- `get_refracted_ray()` - computes refracted ray using Snell's law, handles TIR
- `below_threshold()` - checks recursion level (basic implementation)

### 6. geometry/ray3.* - COMPLETE ✓
- `refract()` - core Snell's law calculation with TIR detection

### 7. Scene Graph Extension - COMPLETE ✓
- `find_closest_intersect` - finds nearest ray-object intersection
- `does_intersect_exist` - shadow ray occlusion test
- `scene_state.*` - carries state through scene graph traversal

---

## Scene Requirements Checklist

### Minimum Requirements (86%)

#### Objects
- [x] At least 2 sphere objects (using ray/sphere intersect, not mesh)
- [x] At least 2 planar surfaces (quadrilaterals, walls, floor) - back wall + left wall
- [x] At least 3 triangle mesh objects with:
  - [ ] Modeling transformation (scale, translate, rotate) - TODO
  - [ ] Inverse modeling matrix for ray transformation - TODO
  - [x] AABB bounding volume (check BV before individual triangles)

#### Effects
- [x] At least 2 reflective objects
- [x] Shadows implemented
- [x] At least 1 semi-transparent object with refraction

#### Texturing
- [ ] Procedural texture on 1 object (checkerboard, brick, or wood grain)
- [ ] Image texture on at least 1 object (use barycentric coordinates)

#### Optimization
- [x] Adaptive depth testing (recursion level check implemented)
- [ ] Hierarchy of Bounding Objects (AABB parent to geometry nodes)

### Bonus Extensions (up to +4%)

- [ ] Marble texture using turbulence (+2%)
- [ ] Quadric object (ellipsoid, cylinder) with analytical ray intersection (+2%)
- [ ] Cook-Torrance or alternative to Phong reflection (+2%)
- [ ] Anti-aliasing via supersampling in Framebuffer (+2%)

---

## Current Test Scene (main.cpp)

```
Camera: (7, 0.5, -5) looking at origin
Light: (5, 5, 10)

Spheres:
- Red sphere at origin (radius 0.5)
- Mirror sphere at (-1.5, 0, 0) (radius 0.5) - reflective
- Glass sphere at (1.2, 0, 0) (radius 0.5) - transparent
- Green sphere at (1.5, 0, 3) (radius 0.4) - behind glass
- Floor: large sphere (radius 1000, center at y=-1001)

Planar Surfaces (RTQuadNode):
- Back wall (blue) at z=8
- Left wall (tan) at x=-5

Triangle Meshes (RTMeshNode with AABB):
- Yellow pyramid at (-3, -1, 2)
- Cyan box at (2.5, -1, 4) - slightly reflective
- Purple wedge/ramp at (-4, -1, 5)
```

---

## Known Issues

1. **Glass sphere ball lens effect:** Solid glass spheres with IOR > 1 act as powerful lenses, inverting images. This is physically correct but may look "reflective". Use IOR = 1.0 for simple transparency without refraction, or IOR ~1.01-1.02 for subtle effect.

2. **EPSILON value:** Set to 0.0001 in geometry/geometry.hpp. Needed for avoiding self-intersection artifacts in reflection/refraction.

---

## Files Modified This Session

- `geometry/ray3.cpp` - Added `refract()` implementation
- `RayTracer/ray.cpp` - Added `get_refracted_ray()` implementation
- `RayTracer/ray_tracer.cpp` - Added reflection and refraction in `trace_ray()`
- `RayTracer/main.cpp` - Test scene with spheres, walls, and meshes
- `geometry/geometry.hpp` - Increased EPSILON to 0.0001

## Files Created This Session

- `RayTracer/rt_quad_node.hpp` - Quadrilateral (planar surface) ray tracing node
- `RayTracer/rt_quad_node.cpp` - Implementation using 2-triangle intersection
- `RayTracer/rt_mesh_node.hpp` - Triangle mesh ray tracing node with AABB
- `RayTracer/rt_mesh_node.cpp` - Implementation with AABB culling and barycentric interpolation
