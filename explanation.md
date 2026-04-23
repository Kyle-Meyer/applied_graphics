# JHU 605.767 Applied Computer Graphics — Final Project
## Moonlit Desert Ruins: Technique Explanations

---

## 1. Ray Casting (Core Rendering)

Every rendered pixel fires a **primary ray** from the camera through the view plane. `CameraNode::construct_ray(px, py)` computes the world-space ray origin and direction from the pixel's normalized device coordinates, the camera position, and the view volume parameters (vertical FOV, near plane distance, and aspect ratio). The ray tracer then traverses the scene graph recursively, calling `find_closest_intersect` on every node until the nearest hit point is found.

**Reflections and refractions** extend this framework recursively. When a hit surface is reflective, the reflection direction is computed as `R = D - 2(D·N)N` and a new ray is spawned from the hit point. Refractive surfaces apply Snell's law to compute the transmitted ray direction and test for total internal reflection. Recursion is capped at a maximum depth (`g_max_depth = 8`) and also terminated early when accumulated color attenuation drops below a threshold, avoiding wasted computation on rays that contribute negligibly to the final image.

---

## 2. Möller–Trumbore Triangle Intersection

Triangle meshes are intersected using the **Möller–Trumbore algorithm** (`geometry/ray3.cpp`), which uses the cross product of the ray direction and one triangle edge to compute barycentric coordinates in a single pass without explicitly constructing the triangle's plane.

A key implementation detail was making this **double-sided**: the standard algorithm rejects triangles with a negative determinant (backface culling). Instead, we compute `sign(det)` and use it to normalize the `u`, `v`, and `t` checks so both CCW (det > 0) and CW (det < 0) wound triangles are hit correctly. This was necessary because OBJ meshes from external sources can have inconsistent winding order across faces.

At the shading stage, an additional fix handles inward-pointing normals: if the surface normal faces away from the camera (`normal · (−ray.d) < 0`), it is flipped before lighting. This gives correct two-sided shading without breaking closed meshes.

Smooth normals are generated at load time via Assimp flags `aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals`. The join step is essential — without it, no vertices are shared between faces, so Assimp cannot average normals across edges and every face gets a flat per-face normal instead.

The last-hit barycentric coordinates used for interpolating normals and UVs are stored in a `thread_local` cache inside `RTMeshNode`, not as instance members. The original instance-member design caused a race condition under multi-threaded rendering where one thread would overwrite the cache before another thread finished reading it, producing "spider web" shading artifacts across mesh surfaces.

---

## 3. Hierarchical Bounding Volumes and LOD

The scene is organized in a tree of bounding volume nodes that short-circuit intersection traversal, avoiding intersection tests against geometry that cannot possibly be hit.

**`AABBNode`** wraps a group of objects in an axis-aligned bounding box. During ray traversal, if the ray misses the AABB using a slab-method intersection test, the entire subtree is skipped without examining any child geometry. Three AABB groups divide the scene: NearRocks (rocks 1–3, z: −5 to 4), MidRocks (rocks 4–6, z: 5 to 14), and FarRocks (rocks 7–9, z: 20 to 33). The obelisk also has its own AABB.

**`BoundingSphereNode`** wraps individual objects in a bounding sphere for an even cheaper preliminary test — a sphere intersection requires only one quadratic solve versus the six slab tests of an AABB.

**`LODNode`** selects a child geometry node based on camera distance at render time. Near and mid rocks use full OBJ mesh geometry below their LOD distance threshold (30–32 world units) and degrade to a bounding sphere primitive beyond it. Far rocks are always represented as spheres. This avoids tracing thousands of triangles for distant objects that ultimately contribute only a handful of pixels. The LOD distance is measured from the node's world-space position, set via `set_position()` on the LOD node before each render.

---

## 4. Soft Shadows via Area Light Sampling

Rather than a binary in-shadow / not-in-shadow result, `RayTracer::shadow_factor()` returns a float in `[0, 1]` representing the fraction of the light source visible from the hit point. The moon is modelled as a **disc area light** centered at the light position with a 2 world-unit radius.

For each shadow query, **N = 8 stratified samples** are distributed across the disc:

- An orthonormal basis (`disc_u`, `disc_v`) is constructed perpendicular to the direction from the hit point to the light center.
- Samples use **stratified jittered polar coordinates**: sector index `i` determines the base angle (`angle = (i + rand) × 2π/N`) and the radius is drawn as `sqrt((i + rand) / N) × disc_radius`. The square-root mapping gives **uniform area distribution** across the disc, preventing samples from clustering near the center.
- Each sample point on the disc is converted to a world-space position and a shadow ray is fired toward it.
- The fraction of the N rays that reach the light unoccluded becomes the shadow factor, which then scales the diffuse and specular contribution at that point.

The `s` key toggles between N = 8 (soft) and N = 1 (hard binary) for comparison. The random number generator is `thread_local`, making the shadow computation safe under the multi-threaded render path.

---

## 5. Normal Mapping with TBN Transform

Surface normals can be perturbed at each hit point using a normal map stored either as an RGB image or computed procedurally. The pipeline in `ray_tracer.cpp`:

1. Retrieve the UV coordinate and object-space tangent vector `T` from the geometry at the hit point.
2. Transform `T` to world space using the same normal matrix applied to the geometric normal.
3. **Gram-Schmidt re-orthogonalize** `T` against the world-space normal (`T = T − N(N·T)`, then normalize) to remove drift introduced by non-uniform scaling in the transform.
4. Compute the bitangent `B = N × T`.
5. Call `NormalMapNode::sample(uv, world_pos, cam_dist)` to get a tangent-space normal vector `tn` from the normal map or procedural shader.
6. Rotate the tangent-space normal into world space via the TBN matrix: `n' = T·tn.x + B·tn.y + N·tn.z`, then normalize.

This perturbed normal replaces the geometric normal in all subsequent lighting calculations, giving the illusion of fine surface detail (granite texture on the obelisk) without adding any geometry. The `m` key toggles normal mapping on and off for comparison.

---

## 6. Procedural Sand Bump Shader (`SandBumpNode`)

Rather than loading a texture image, the sand surface normal is computed entirely from world-space XZ position using `SandBumpNode::sample()`. This is a subclass of `NormalMapNode` that overrides `sample()` and uses world-space coordinates instead of UVs, so ripples are uniform across the large floor sphere regardless of its UV parameterization (which compresses badly near the sphere poles).

**Domain warp**: Before evaluating the ripple, the input coordinates `(u, v)` are distorted by a sum of low-frequency sine and cosine waves. This curves the otherwise straight parallel ripple lines into organic, wind-formed meanders. The warp is defined as two offset sine pairs to avoid too-regular repetition.

**Aeolian ripples**: A sine wave in warped space provides the primary ripple height field `h`. Its partial derivatives `∂h/∂wu` and `∂h/∂wv` are computed analytically, then mapped back to world-space slopes through the warp Jacobian (`∂wu/∂u`, `∂wu/∂v`, `∂wv/∂u`, `∂wv/∂v`) via the chain rule. These slopes define the tangent-space normal perturbation directly, without needing finite differences.

**Grain noise**: A high-frequency hash-based noise layer adds fine sand grain texture on top of the wind ripples.

**Distance LOD**: The ray tracer passes the camera distance `cam_dist` into `sample()`. A `smoothstep` function fades out grain noise over 5–25 world units (highest frequency, disappears first) and fades ripple amplitude to 15% of its full value over 25–65 world units, so the sand surface reads smooth at the horizon without visible aliasing artifacts.

---

## 7. Procedural Rock and Obelisk Material (`DesertRockTexture`)

`DesertRockTexture` is a `TextureNode` subclass that ports the colour path from a GLSL shader into C++, providing a unique procedural albedo for every rock and the obelisk without texture image files.

**Value noise** (`vnoise`): The building block is trilinear interpolation of a hashed integer lattice with Hermite smoothstep blending (`3t² − 2t³`), equivalent to Blender's "smooth" noise texture. The hash function mirrors GLSL's `fract(sin(dot(p, k)) * large)` pattern.

**Fractional Brownian Motion** (`fbm`): Multiple octaves of value noise are accumulated with doubling frequency (lacunarity = 2) and halving amplitude. An optional coordinate distortion before sampling produces the vein-like stripes visible in the rock surface.

**Two-layer blending**: Two separate fBm evaluations — one for base colour variation and one for stripes — are linearly interpolated to produce the final albedo. The obelisk uses a dark near-black granite palette; the rocks use a blue-grey palette to match the moonlit scene. Since **world-space hit position** is used as the texture coordinate, each rock gets a distinct noise pattern rather than the same repeating texture mapped onto every object.

---

## 8. Sand Sparkle: Micro-facet Glint

A hash-based micro-facet sparkle effect is added additively on top of the standard shading for sand and similar materials, simulating moonlight reflecting off quartz grains.

- A hash of the UV cell determines which cells contain a glinting grain (controlled by a density threshold on the material).
- Two additional hashes per cell determine the grain's **tilt axis** and **tilt magnitude** (up to ~31°). The tilt is applied by rotating the shading normal within the surface tangent plane, producing a micro-mirror oriented independently from the macroscopic surface.
- The grain glints when the **half-vector** `H = normalize(V + L)` aligns with the tilted grain normal — this is the core micro-facet specular condition. The intensity function uses two Blinn-Phong-like lobes: a broad dim lobe (`spec^3 × 0.08`) for soft off-angle glow and a tight bright lobe (`spec^80`) for the sharp specular peak.
- The effect is **view-dependent**: moving the camera changes which grains satisfy the half-vector condition, giving the impression of sparkling, shifting light on wet or crystalline sand. The `k` key toggles sparkle on and off.

---

## 9. Adaptive Super-sampling

Three anti-aliasing modes cycle with the `a` key to trade render time against image quality:

**Off**: One ray per pixel center.

**Uniform 2×2**: Four stratified sub-pixel samples always taken at fixed offsets within the pixel, averaged to produce the final color. Eliminates aliasing uniformly but quadruples ray count.

**Adaptive**: Attempts to spend rays only where the image is changing rapidly.
- The four pixel corner colors are traced first (one ray each).
- The luminance range `max_lum − min_lum` across the four corners is computed using the perceptual weighting `0.299R + 0.587G + 0.114B`.
- If the range is below the threshold (0.1), the pixel is smooth and the four corners are averaged — no additional rays fired.
- If above threshold, the pixel is subdivided into four quadrants. Each quadrant requires only one new interior sample (the center and two edge midpoints), because the other corners are inherited from the parent or shared with siblings. This avoids retracing shared boundary points.
- Subdivision recurses to a maximum depth of 2, giving up to 16 samples/pixel in high-contrast regions such as edges and shadow boundaries.

After each adaptive render, the average samples-per-pixel across the image is printed to the console.

---

## 10. Procedural Night Sky with Moon Disc

`sky_color(dir)` in `ray_tracer.cpp` is called for every ray that misses all scene geometry — primary rays hitting open sky, reflected rays, etc. — and returns a procedural background color.

**Sky gradient**: The background color is a quadratic function of `dir.y` (the elevation of the ray), blending from a near-black horizon to a deep blue zenith.

**Stars**: The ray direction is converted to spherical coordinates `(phi, theta)` and mapped to a 200×100 angular grid, giving roughly 15,700 upper-hemisphere cells. Each cell is hashed; approximately 1% of cells contain a star. The star's position within the cell and brightness are determined by additional hash values. The two-tier hash (cell selection then position/brightness) keeps the star field dense but irregular.

**Moon disc**: The ray direction is dot-producted against a precomputed `MOON_DIR` unit vector. If the dot product exceeds a threshold corresponding to a 4.5° half-angle, the pixel is inside the moon disc. Brightness uses mild limb darkening (`limb = 0.80 + 0.20 × moon_t`, where `moon_t = 0` at the disc edge and `1` at center) with a soft edge blend (`min(1, moon_t × 8)`) to avoid a hard aliased boundary. An atmospheric halo extends to 12°; `halo_t` is clamped to `[0, 1]` to prevent it from over-brightening inside the disc. All output channels are clamped to `[0, 1]` before returning to prevent integer overflow wrap-around in the pixel write path (which would otherwise produce red and yellow-green bands instead of white).

The moon disc direction matches the scene's directional light position, so the visible moon disc and the source of scene lighting are geometrically consistent.
