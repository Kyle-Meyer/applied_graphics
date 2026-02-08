# Applied Graphics - Ray Tracing Assignment

## Current Status (Updated Feb 3, 2026)

**Working:**
- Basic ray tracing with Phong-Blinn lighting and shadows
- Reflection (mirror spheres working)
- Refraction (implemented, but solid glass spheres act as ball lenses - physically correct but may look "reflective" due to image inversion)
- Procedural texturing (checkerboard on floor via FunctionalTexture)
- Image texturing (floor_tiles.jpg on back wall via ImageTexture)

**Test scene:** Red sphere, mirror sphere, glass sphere, green sphere, checkered floor, textured back wall, one light source.

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

### 4. RayTracer/ray_tracer.* - COMPLETE ✓

**Completed:**
- `add_light()` - register lights with ray tracer
- `in_shadow()` - shadow ray casting
- `trace_ray()`:
  - Finds closest intersection
  - Computes ambient + emission + diffuse + specular
  - Gets texture color and modulates (procedural or UV-based)
  - Spawns reflected rays for reflective materials
  - Spawns refracted rays for transparent materials
  - Handles total internal reflection
  - Checks max depth / attenuation threshold before spawning rays

### 5. RayTracer/ray.* - COMPLETE ✓
- `get_refracted_ray()` - computes refracted ray using Snell's law, handles TIR
- `below_threshold()` - checks recursion level (basic implementation)

### 6. geometry/ray3.* - COMPLETE ✓
- `refract()` - core Snell's law calculation with TIR detection

### 7. Scene Graph Extension - COMPLETE ✓
- `find_closest_intersect` - finds nearest ray-object intersection
- `does_intersect_exist` - shadow ray occlusion test
- `scene_state.*` - carries state through scene graph traversal

### 8. RTTransformNode - COMPLETE ✓
Transform node for ray tracing that supports modeling transformations on geometry:
- Transforms rays to object space using inverse of modeling matrix
- Stores inverse matrix and normal matrix in SceneState for result transformation
- Normal matrix (transpose of inverse) correctly transforms normals back to world space
- Works with translate, rotate, scale operations from parent TransformNode class
- Caches inverse/normal matrices for performance (computed once on first ray)

### 9. AABBNode (Bounding Volume Hierarchy) - COMPLETE ✓
Axis-aligned bounding box node for hierarchical culling:
- `find_closest_intersect`: Tests ray against AABB, skips children if miss
- `does_intersect_exist`: Tests ray against AABB, skips children if miss or beyond distance
- Used to wrap groups of geometry (e.g., all mesh objects) for early rejection

---

## Scene Requirements Checklist

### Minimum Requirements (86%)

#### Objects
- [x] At least 2 sphere objects (using ray/sphere intersect, not mesh)
- [x] At least 2 planar surfaces (quadrilaterals, walls, floor) - back wall + left wall
- [x] At least 3 triangle mesh objects with:
  - [x] Modeling transformation (scale, translate, rotate) - RTTransformNode
  - [x] Inverse modeling matrix for ray transformation - RTTransformNode
  - [x] AABB bounding volume (check BV before individual triangles)

#### Effects
- [x] At least 2 reflective objects
- [x] Shadows implemented
- [x] At least 1 semi-transparent object with refraction

#### Texturing
- [x] Procedural texture on 1 object (checkerboard on floor via FunctionalTexture)
- [x] Image texture on at least 1 object (floor_tiles.jpg on back wall via ImageTexture)

#### Optimization
- [x] Adaptive depth testing (recursion level check implemented)
- [x] Hierarchy of Bounding Objects (AABBNode wrapping mesh objects)

### Bonus Extensions (up to +4%)

- [ ] Marble texture using turbulence (+2%)
- [ ] Quadric object (ellipsoid, cylinder) with analytical ray intersection (+2%)
- [ ] Cook-Torrance or alternative to Phong reflection (+2%)
- [ ] Anti-aliasing via supersampling in Framebuffer (+2%)

---

## Current Test Scene (main.cpp)

```
Camera: (7, 0.5, -5) looking at origin
Light: (4, 6, -1)

Spheres (moved left to reveal mesh objects):
- Red sphere at (-1.5, 0, 0) (radius 0.5)
- Mirror sphere at (-3, 0, 0) (radius 0.5) - reflective
- Glass sphere at (-0.3, 0, 0) (radius 0.5) - transparent
- Green sphere at (0, 0, 3) (radius 0.4)
- Floor: large sphere (radius 1000, center at y=-1001) - checkered texture

Planar Surfaces (RTQuadNode):
- Back wall at z=8 - image texture (floor_tiles.jpg)
- Left wall (tan) at x=-5

Triangle Meshes (RTMeshNode with RTTransformNode, wrapped in AABBNode):
- Parent AABBNode bounds: (-5, -1.5, 1) to (4, 1, 8) - culls all meshes if ray misses
- Yellow pyramid: unit mesh at origin, transformed with scale(1.5), rotate_y(15°), translate(-2.5, -1, 2.5)
- Cyan box: unit cube at origin, transformed with scale(0.8), rotate_y(30°), translate(2.9, -0.6, 4.4) - reflective
- Purple wedge: unit wedge at origin, transformed with scale(1, 1, 2), rotate_y(-20°), translate(-3.5, -1, 6)
```

---

## Known Issues

1. **Glass sphere ball lens effect:** Solid glass spheres with IOR > 1 act as powerful lenses, inverting images. This is physically correct but may look "reflective". Use IOR = 1.0 for simple transparency without refraction, or IOR ~1.01-1.02 for subtle effect.

2. **EPSILON value:** Set to 0.0001 in geometry/geometry.hpp. Needed for avoiding self-intersection artifacts in reflection/refraction.

---

## Files Modified

- `geometry/ray3.cpp` - Added `refract()` implementation
- `RayTracer/ray.cpp` - Added `get_refracted_ray()` implementation
- `RayTracer/ray_tracer.cpp` - Added reflection, refraction, texture modulation, and normal transformation
- `RayTracer/main.cpp` - Test scene with spheres, walls, meshes, textures, transforms, and bounding hierarchy
- `geometry/geometry.hpp` - Increased EPSILON to 0.0001
- `scene/bounding_aabb_node.cpp` - Implemented `find_closest_intersect` and `does_intersect_exist` for ray tracing

## Files Created

- `RayTracer/rt_quad_node.hpp` - Quadrilateral (planar surface) ray tracing node
- `RayTracer/rt_quad_node.cpp` - Implementation using 2-triangle intersection
- `RayTracer/rt_mesh_node.hpp` - Triangle mesh ray tracing node with AABB
- `RayTracer/rt_mesh_node.cpp` - Implementation with AABB culling and barycentric interpolation
- `RayTracer/functional_texture.hpp` - Function-based procedural texture class
- `RayTracer/functional_texture.cpp` - Implementation with static factory methods (checkerboard, stripes, gradient)
- `RayTracer/rt_transform_node.hpp` - Transform node for ray tracing with inverse matrix support
- `RayTracer/rt_transform_node.cpp` - Transforms rays to object space, stores inverse/normal matrices

## Texturing Architecture

**Scene graph structure for textured objects:**
```
MaterialNode → TextureNode → GeometryNode
```

**FunctionalTexture** - Flexible procedural texture using lambdas:
- `get_color(Point3)` - world-space procedural patterns
- `get_color(TextureCoord2)` - UV-based patterns
- Static factories: `checkerboard()`, `stripes()`, `gradient()`
- Easy to extend with custom patterns via lambda

**ImageTexture** (existing) - Loads image files with bilinear filtering

**Texture lookup in ray_tracer.cpp:**
1. Tries procedural (world-space) first
2. Falls back to UV-based if procedural returns black
3. Modulates computed color by texture color
