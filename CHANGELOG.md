# Changelog

## [0aeac81] - 2026-02-09 - perhaps 100%?

**Image texturing on surfaces via UV coordinates.**

- Added UV-based texture lookup to `FunctionalTexture` (`get_color(TextureCoord2)`) so textures can be applied using geometry-provided UV coordinates rather than only world-space position
- Applied `floor_tiles.jpg` image texture to the back wall via `ImageTexture` + `TextureNode`, completing the image texturing requirement
- Adjusted scene layout (sphere positions, wall placement) for better composition in the final render

**Why:** The assignment requires at least one image-textured object. 
The back wall was the natural candidate since planar surfaces have straightforward UV mapping. 
Adding UV support to `FunctionalTexture` also made the texture system more flexible overall.

---

## [faef691] - 2026-02-08 - mostly done

**Modeling transformations and bounding volume hierarchy for mesh objects.**

- Created `RTTransformNode` (`rt_transform_node.cpp/.hpp`) to support modeling transformations in ray tracing -- transforms rays into object space using the inverse modeling matrix, and transforms normals back to world space using the normal matrix (transpose of inverse)
- Implemented `BoundingAABBNode::find_closest_intersect` and `does_intersect_exist` in `scene/bounding_aabb_node.cpp` for axis-aligned bounding box ray intersection testing
- Wrapped all triangle mesh objects in an `AABBNode` hierarchy for early rejection, and each mesh in its own `RTTransformNode` for scale/rotate/translate
- Added left wall (tan-colored `RTQuadNode`) as a second planar surface
- Expanded scene with a purple wedge mesh and reorganized object placement

**Why:** Mesh objects need modeling transformations (scale, rotate, translate) to be positioned in the scene, but rays must be transformed to object space for correct intersection. The inverse matrix approach avoids transforming every triangle -- instead, one ray is transformed per object. The AABB hierarchy provides an optimization layer: if a ray misses the bounding box, all enclosed triangles are skipped, which is critical for performance with mesh objects.

---

## [1bd1045] - 2026-02-03 - coolio

**Replaced ad-hoc checkerboard with a general-purpose procedural texture system.**

- Created `FunctionalTexture` (`functional_texture.cpp/.hpp`) -- a lambda-based procedural texture class with static factory methods for `checkerboard()`, `stripes()`, and `gradient()` patterns
- Replaced the earlier `CheckerboardTexture` class with `FunctionalTexture::checkerboard()`, making procedural textures composable and easy to extend
- Improved texture lookup in `ray_tracer.cpp` to try procedural (world-space) first, then fall back to UV-based lookup

**Why:** The initial checkerboard implementation was a one-off class. Switching to a lambda-based approach means new procedural 
patterns can be added as one-liners without creating new classes. 
This architecture also cleanly separates world-space patterns (like checkerboard on an infinite floor) 
from UV-mapped patterns (like image textures on walls).

---

## [1b4bcf4] - 2026-02-01 - added checkered floor

**Procedural texturing via checkerboard pattern on the floor.**

- Created `CheckerboardTexture` (`checkerboard_texture.cpp/.hpp`) as a `ProceduralTexture` subclass that generates a black-and-white checkerboard based on world-space coordinates
- Extended `ProceduralTexture` with a `get_color(Point3)` virtual method for world-space procedural patterns
- Added texture color modulation in `ray_tracer.cpp` -- the computed lighting color is multiplied by the texture color
- Applied the checkerboard to the large floor sphere via the `MaterialNode -> TextureNode -> GeometryNode` scene graph pattern

**Why:** Procedural textures generate patterns mathematically rather than from image files, 
which is required by the assignment. A checkerboard on the floor is the classic demonstration: 
it provides visual grounding, helps judge reflections, and clearly shows the pattern is computed from 
world-space position (not UV coordinates).

---

## [d8fd0e2] - 2026-02-01 - added complex geometry, fixed some bugs

**Triangle mesh objects, planar surfaces, and refraction.**

- Created `RTQuadNode` (`rt_quad_node.cpp/.hpp`) for ray-quad intersection using two-triangle decomposition, supporting textured planar surfaces
- Created `RTMeshNode` (`rt_mesh_node.cpp/.hpp`) for triangle mesh ray tracing with per-triangle intersection, barycentric coordinate interpolation for normals/UVs, and built-in AABB culling
- Implemented `refract()` in `geometry/ray3.cpp` using Snell's law with total internal reflection detection
- Implemented `get_refracted_ray()` in `RayTracer/ray.cpp` to construct refracted rays at material boundaries, handling inside/outside transitions via normal flipping and IOR ratio calculation
- Added refraction spawning in `ray_tracer.cpp` for transparent materials, alongside the existing reflection code
- Added a glass sphere (IOR 1.5), back wall with image texture, and yellow pyramid mesh to the test scene
- Increased EPSILON from 0.00001 to 0.0001 to fix self-intersection artifacts with reflection/refraction rays
- Created initial `CLAUDE.md` documenting architecture and progress

**Why:** This was a major milestone adding three assignment requirements at once. Quad nodes provide the planar surfaces needed for walls/floors. 
Mesh nodes enable arbitrary triangle geometry (pyramids, boxes, wedges). 
Refraction completes the transparency effect -- Snell's law governs how light bends at material boundaries, and total internal 
reflection must be handled when the angle exceeds the critical angle. 
The EPSILON bump was needed because reflection/refraction rays originate very close to surfaces, 
and too-small epsilon causes rays to re-intersect the surface they just left.

---

## [505f127] - 2026-01-29 - added reflectivity

**Mirror reflections via recursive ray tracing.**

- Added reflection ray spawning in `ray_tracer.cpp` -- when a material has reflectivity > 0, a reflected ray is cast and its color is blended with the local illumination
- Added a mirror sphere to the test scene to demonstrate reflections
- Fixed ray direction normalization issue

**Why:** Reflections are a core ray tracing feature. The approach is recursive: at each intersection, if the material is reflective, 
compute the reflection direction (incident ray mirrored about the surface normal), spawn a new ray, and blend the result. 
Recursion depth is bounded to prevent infinite mirror-to-mirror bouncing.

---

## [b311cab] - 2026-01-28 - added shadow and light, added floor too

**Shadow rays, proper lighting integration, and a ground plane.**

- Implemented `in_shadow()` in `ray_tracer.cpp` -- casts a shadow ray from the intersection point toward each light source and checks for occluding geometry
- Reworked `trace_ray()` to properly accumulate ambient + diffuse + specular contributions per light, skipping diffuse/specular when the point is in shadow
- Registered lights with the ray tracer via `add_light()` and integrated `LightNode` data (position, colors) into shading calculations
- Added a large sphere as a ground plane (radius 1000, centered far below the scene)
- Added a `.gitignore`

**Why:** Without shadows, the scene looks flat and unrealistic -- every surface is lit uniformly. 
Shadow rays are the standard ray tracing solution: before computing diffuse/specular for a light, cast a ray toward it. 
If something blocks the path, that light contributes no diffuse or specular at that point (only ambient). 
The floor sphere is a common trick -- a very large sphere approximates an infinite plane while reusing the existing sphere intersection code.

---

## [a554a9e] - 2026-01-27 - part 2 mostly done

**Phong-Blinn shading model and sphere normals/texture coordinates.**

- Implemented Phong-Blinn lighting in `ray_tracer.cpp` with ambient, diffuse (N dot L), and specular (N dot H) components
- Added `get_normal()` to `RTSphereNode` -- returns the normalized vector from sphere center to intersection point
- Added `get_texture_coord()` to `RTSphereNode` using spherical mapping (theta/phi to s/t)
- Added base class default implementations in `GeometryNode` for `get_normal` and `get_texture_coord`
- Expanded the test scene with multiple colored spheres and a light source

**Why:** Shading transforms flat colored circles into convincing 3D spheres. 
Phong Blinn was chosen as the lighting model because it's the assignment standard: 
ambient provides base illumination, diffuse gives directional shading based on the surface-to-light angle, 
and specular adds highlights using the half-vector between view and light directions. 
Sphere normals and texture coordinates are analytical (no mesh data needed), making spheres ideal first-pass geometry.

---

## [e16ed50] - 2026-01-27 - working camera?

**Camera ray construction for perspective projection.**

- Implemented `construct_ray()` in `CameraNode` to generate rays from the camera through each pixel on the image plane
- Fixed `set_view_volume` and `set_image_plane_dimensions` to correctly define the view frustum and pixel-to-world mapping
- Set up basic ray tracing loop in `main.cpp` to cast one ray per pixel and write results to the framebuffer

**Why:** The camera is the entry point for ray tracing -- every pixel's color is determined by casting a ray from the eye through 
that pixel's position on the image plane into the scene. Getting the coordinate mapping right 
(pixel coordinates to world-space ray direction) is essential before any intersection or shading code can be tested.

---

## [c2794a5] - 2026-01-22 - mostly working???

**Initial project setup with framework code.**

- Added complete project framework: geometry library (vectors, matrices, points, rays, planes, bounding volumes), scene graph (nodes for camera, lights, materials, textures, transforms, geometry), shader support, polygon mesh rendering, and build system (CMake)
- Included third party dependencies: SDL3, Assimp, stb_image, GLEW
- Added ray tracer skeleton: `RayTracer`, `Ray`, `RTSphereNode`, `MaterialNode`, `TextureNode`, `Framebuffer`, `Lighting` classes with basic structure but minimal implementation
- Included test scenes, models (A10, bug), and texture assets

**Why:** This is the course provided framework plus initial skeleton code. 
The scene graph, geometry library, and rendering infrastructure are shared across assignments. 
The ray tracer module was scaffolded with the classes that would be filled in over subsequent commits.
