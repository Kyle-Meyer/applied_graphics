# Assignment 2: Design and Implementation Plan

## Project Overview

Extend the existing scene graph framework to support hierarchical bounding volumes, view frustum culling, and level of detail (LOD) selection. The project builds on the existing `605.667` codebase which already has partial infrastructure for bounding volumes and view frustum testing.

---

## Requirements Summary

### Required Implementations (Must Have)
1. **View Frustum Culling** (7 pts) - Extract frustum planes from camera, test BV intersections
2. **Level of Detail Selection** (5 pts) - Distance or projected-area based LOD switching

### Choose 2 of 3 Procedural Techniques (7 pts each)
- [ ] **Parametric Surfaces** - Subdivide and store in mesh
- [ ] **Parametric Curves** - Define motion paths, forward differencing animation
- [ ] **Subdivision Surfaces** - Render on-the-fly or pre-generate LOD levels

### Additional Requirements
- Scene Definition (2 pts)
- Documentation + Screenshots (2 pts)
- View control with debug output for culling/LOD

---

## Existing Infrastructure Analysis

### Already Implemented (Ready to Use)
| Component | Location | Status |
|-----------|----------|--------|
| `AABB` struct | `geometry/aabb.hpp` | Complete - merge, vertex-list construction |
| `BoundingSphere` struct | `geometry/bounding_sphere.hpp` | Complete - Ritter's algorithm, merge |
| `Plane` struct | `geometry/plane.hpp` | Complete - `solve()` for signed distance |
| `ViewFrustum` class | `scene/view_frustum.hpp` | Interface exists, methods stubbed |
| `AABBNode` | `scene/bounding_aabb_node.hpp` | Exists, ray-intersect done, culling stubbed |
| `BoundingSphereNode` | `scene/bounding_sphere_node.hpp` | Exists, culling stubbed |
| `CameraNode` | `scene/camera_node.hpp` | Full FPS camera with FOV, near/far access |

### Needs Implementation
| Component | Gap |
|-----------|-----|
| `ViewFrustum::construct()` | Extract 6 planes from camera matrices |
| `ViewFrustum::intersect(BoundingSphere)` | Sphere vs 6-plane test |
| `ViewFrustum::intersect(AABB)` | P-vertex/N-vertex plane test |
| `AABBNode::draw()` culling | Check frustum, skip children if OUTSIDE |
| `BoundingSphereNode::draw()` culling | Check frustum, skip children if OUTSIDE |
| `LODNode` class | New node type for LOD switching |
| Parametric/Subdivision surfaces | New geometry generators |

---

## Part 1: View Frustum Culling

### 1.1 Frustum Plane Extraction

**File:** `scene/view_frustum.cpp`

Implement `ViewFrustum::construct(std::shared_ptr<CameraNode> camera)`:

```
Algorithm: Extract Frustum Planes from View-Projection Matrix
1. Get combined VP = projection * view matrix from camera
2. Extract 6 planes using Gribb/Hartmann method:
   - Left:   row4 + row1
   - Right:  row4 - row1
   - Bottom: row4 + row2
   - Top:    row4 - row2
   - Near:   row4 + row3
   - Far:    row4 - row3
3. Normalize each plane (a,b,c,d) by sqrt(a^2+b^2+c^2)
```

### 1.2 Bounding Sphere-Frustum Intersection

**File:** `scene/view_frustum.cpp`

```
Algorithm: Sphere-Frustum Test
For each of 6 planes:
   distance = plane.solve(sphere.center)  // signed distance
   if distance < -sphere.radius:
      return OUTSIDE
   if distance < sphere.radius:
      result = INTERSECT  // partially inside
return result (INSIDE if never intersected)
```

### 1.3 AABB-Frustum Intersection

**File:** `scene/view_frustum.cpp`

```
Algorithm: AABB-Frustum Test (P-vertex/N-vertex)
For each of 6 planes:
   // P-vertex: corner most aligned with plane normal
   // N-vertex: corner least aligned with plane normal
   p_vertex = select corner based on plane normal signs
   n_vertex = opposite corner

   if plane.solve(p_vertex) < 0:
      return OUTSIDE
   if plane.solve(n_vertex) < 0:
      result = INTERSECT
return result
```

### 1.4 Integrate Culling into Scene Graph

**Files:** `scene/bounding_aabb_node.cpp`, `scene/bounding_sphere_node.cpp`

Modify `draw()` methods:
```cpp
void AABBNode::draw(SceneState& state) {
    FrustumIntersectType result = state.frustum->intersect(box_);

    if (result == OUTSIDE) {
        std::cout << "CULLED: " << name_ << std::endl;
        return;  // Skip this node and all children
    }

    // If INSIDE, can skip frustum tests for children (optimization)
    // If INTERSECT, must test children
    SceneNode::draw(state);
}
```

### 1.5 Add ViewFrustum to SceneState

**File:** `scene/scene_state.hpp`

Add member:
```cpp
std::shared_ptr<ViewFrustum> frustum;
```

Update `CameraNode::draw()` to construct frustum before traversal.

---

## Part 2: Level of Detail (LOD) System

### 2.1 LODNode Class Design

**New File:** `scene/lod_node.hpp`

```cpp
class LODNode : public SceneNode {
public:
    struct LODLevel {
        float max_distance;  // Switch to next level beyond this
        std::shared_ptr<SceneNode> geometry;
    };

    void add_level(float distance, std::shared_ptr<SceneNode> node);
    void draw(SceneState& state) override;

private:
    std::vector<LODLevel> levels_;  // Sorted by distance

    float compute_distance(const SceneState& state);
    int select_level(float distance);
};
```

### 2.2 LOD Selection Algorithm

```
Algorithm: Distance-Based LOD Selection
1. Get object center in world space (from model_matrix)
2. Compute distance to camera_position (from SceneState)
3. Iterate through LOD levels to find appropriate level
4. Draw only the selected level's geometry
5. Print debug: "LOD: <name> using level <n> (distance: <d>)"
```

### 2.3 Alternative: Projected Area Selection

```
Algorithm: Projected Area LOD Selection
1. Get bounding sphere radius and center
2. Transform center to view space
3. Compute projected radius: screen_radius = (radius / distance) * fov_factor
4. Select LOD based on pixel coverage thresholds
```

---

## Part 3: Procedural Techniques (Choose 2)

### Option A: Parametric Surfaces (Recommended)

**New File:** `scene/parametric_surface.hpp`

Implement surfaces like:
- **Bezier Patch** - 4x4 control points, de Casteljau evaluation
- **B-Spline Surface** - Uniform B-spline basis functions
- **NURBS Surface** - Weighted rational B-spline

```cpp
class BezierPatchSurface : public GeometryNode {
public:
    BezierPatchSurface(const std::array<Point3, 16>& control_points,
                       int u_subdivisions, int v_subdivisions);

    // Generates TriSurface mesh at specified subdivision level
    void generate_mesh(int subdivisions);

private:
    std::array<Point3, 16> control_points_;
    Point3 evaluate(float u, float v);  // de Casteljau
    Vector3 compute_normal(float u, float v);
};
```

**LOD Integration:** Generate multiple subdivision levels (e.g., 4x4, 8x8, 16x16, 32x32) and store in LODNode.

### Option B: Parametric Curves for Animation

**New File:** `scene/parametric_curve.hpp`

Implement curve types:
- **Bezier Curve** - Cubic Bezier with 4 control points
- **Catmull-Rom Spline** - Passes through control points
- **B-Spline** - Smooth approximating curve

```cpp
class BezierCurve {
public:
    BezierCurve(const std::array<Point3, 4>& control_points);

    // Direct evaluation
    Point3 evaluate(float t);

    // Forward differencing for animation
    void init_forward_difference(int steps);
    Point3 next_position();  // Returns next point using forward diff

private:
    std::array<Point3, 4> control_points_;

    // Forward differencing state
    Point3 p_, dp_, d2p_, d3p_;
};
```

**Animation Integration:**
```cpp
void AnimatedObject::update(SceneState& state) {
    Point3 new_pos = motion_curve_.next_position();
    transform_->set_translation(new_pos);
}
```

### Option C: Subdivision Surfaces

**New File:** `scene/subdivision_surface.hpp`

Implement Catmull-Clark or Loop subdivision:

```cpp
class SubdivisionSurface : public GeometryNode {
public:
    SubdivisionSurface(const std::vector<Point3>& vertices,
                       const std::vector<Face>& faces);

    // Generate mesh at specified subdivision level
    void subdivide(int levels);

    // Pre-generate multiple LOD levels
    void generate_lod_levels(int max_level);
    std::shared_ptr<TriSurface> get_level(int level);

private:
    // Half-edge or face-vertex data structure
    std::vector<Point3> vertices_;
    std::vector<Face> faces_;
    std::vector<std::shared_ptr<TriSurface>> lod_meshes_;
};
```

---

## Part 4: Scene Design

### Recommended Scene Layout

```
Scene Graph Structure:
├── CameraNode
│   └── LightNode (point light, follows camera)
├── SkyboxNode (optional)
├── GroundPlane
│   └── BoundingAABBNode
│       └── TexturedUnitSquare
├── ObjectCluster_1 (left side - for frustum culling demo)
│   └── BoundingSphereNode (parent BV)
│       ├── BoundingSphereNode (child 1)
│       │   └── TransformNode
│       │       └── LODNode (parametric surface with 4 LOD levels)
│       ├── BoundingSphereNode (child 2)
│       │   └── TransformNode
│       │       └── SubdivisionSurface
│       └── BoundingSphereNode (child 3)
│           └── TransformNode
│               └── ModelNode (loaded mesh)
├── ObjectCluster_2 (right side)
│   └── BoundingSphereNode
│       └── ... similar structure
├── AnimatedObject (if using parametric curves)
│   └── TransformNode (position updated by curve)
│       └── Geometry
└── StaticObjects (for visual interest)
    └── Various meshes
```

### Scene Requirements Checklist
- [ ] At least one hierarchical BV (parent BV with child BVs)
- [ ] Multiple objects that can be culled
- [ ] At least one LODNode with multiple detail levels
- [ ] Two procedural techniques integrated
- [ ] Camera controls working
- [ ] Debug output for culling/LOD decisions

---

## Part 5: Implementation Timeline

### Phase 1: Core Frustum Culling (Foundation)
**Tasks:**
1. Implement `ViewFrustum::construct()` - plane extraction
2. Implement `ViewFrustum::intersect(BoundingSphere)`
3. Implement `ViewFrustum::intersect(AABB)`
4. Add `ViewFrustum` to `SceneState`
5. Update `CameraNode::draw()` to construct frustum
6. Implement culling in `AABBNode::draw()`
7. Implement culling in `BoundingSphereNode::draw()`
8. Add debug print statements
9. Test with simple scene

### Phase 2: LOD System
**Tasks:**
1. Create `LODNode` class
2. Implement distance calculation
3. Implement level selection
4. Add debug print for LOD switches
5. Test LOD transitions

### Phase 3: Procedural Technique 1 (Parametric Surfaces)
**Tasks:**
1. Implement Bezier patch evaluation
2. Create mesh generation at multiple subdivision levels
3. Integrate with LODNode
4. Test surface quality at different levels

### Phase 4: Procedural Technique 2 (Parametric Curves OR Subdivision)
**Tasks (Curves):**
1. Implement Bezier curve evaluation
2. Implement forward differencing
3. Create animation system
4. Animate camera or object along path

**Tasks (Subdivision):**
1. Implement Catmull-Clark or Loop subdivision
2. Generate LOD levels
3. Integrate with LODNode

### Phase 5: Scene Construction
**Tasks:**
1. Design scene layout
2. Create hierarchical BV structure
3. Place objects for culling demonstration
4. Add visual interest objects
5. Set up camera path/controls

### Phase 6: Testing and Polish
**Tasks:**
1. Verify all culling works correctly
2. Verify LOD transitions are smooth
3. Test edge cases (camera inside BV, etc.)
4. Performance testing
5. Take screenshots
6. Write documentation

### Phase 7: Documentation
**Tasks:**
1. Write project description
2. Document scene structure
3. Describe techniques used
4. Include code structure explanation
5. Capture overview screenshot
6. Capture detail screenshots

---

## File Change Summary

### New Files to Create
| File | Purpose |
|------|---------|
| `scene/lod_node.hpp` | LOD selection node |
| `scene/lod_node.cpp` | LOD implementation |
| `scene/parametric_surface.hpp` | Bezier/B-spline surfaces |
| `scene/parametric_surface.cpp` | Surface implementation |
| `scene/parametric_curve.hpp` | Bezier curves for animation |
| `scene/parametric_curve.cpp` | Curve implementation |
| `scene/subdivision_surface.hpp` | (If chosen) Subdivision |
| `scene/subdivision_surface.cpp` | Subdivision implementation |

### Existing Files to Modify
| File | Changes |
|------|---------|
| `scene/view_frustum.cpp` | Implement stubbed methods |
| `scene/bounding_aabb_node.cpp` | Add frustum culling logic |
| `scene/bounding_sphere_node.cpp` | Add frustum culling logic |
| `scene/scene_state.hpp` | Add ViewFrustum pointer |
| `scene/camera_node.cpp` | Construct frustum in draw() |
| `SampleProject/main.cpp` | Build new scene |
| `scene/CMakeLists.txt` | Add new source files |

---

## Technical Notes

### Frustum Plane Extraction (Gribb-Hartmann Method)

Given the combined View-Projection matrix M with rows m1, m2, m3, m4:
```
Left:   (m4 + m1)  ->  (m[3]+m[0], m[7]+m[4], m[11]+m[8], m[15]+m[12])
Right:  (m4 - m1)  ->  (m[3]-m[0], m[7]-m[4], m[11]-m[8], m[15]-m[12])
Bottom: (m4 + m2)  ->  (m[3]+m[1], m[7]+m[5], m[11]+m[9], m[15]+m[13])
Top:    (m4 - m2)  ->  (m[3]-m[1], m[7]-m[5], m[11]-m[9], m[15]-m[13])
Near:   (m4 + m3)  ->  (m[3]+m[2], m[7]+m[6], m[11]+m[10], m[15]+m[14])
Far:    (m4 - m3)  ->  (m[3]-m[2], m[7]-m[6], m[11]-m[10], m[15]-m[14])
```

### Forward Differencing for Cubic Bezier

For a cubic Bezier curve B(t) evaluated at uniform steps of delta:
```
Initial values (at t=0):
p   = P0
dp  = delta^3*(-P0 + 3P1 - 3P2 + P3) + delta^2*(3P0 - 6P1 + 3P2) + delta*(-3P0 + 3P1)
d2p = 6*delta^3*(-P0 + 3P1 - 3P2 + P3) + 2*delta^2*(3P0 - 6P1 + 3P2)
d3p = 6*delta^3*(-P0 + 3P1 - 3P2 + P3)

Each step:
p   += dp
dp  += d2p
d2p += d3p
```

### Catmull-Clark Subdivision Rules

For each subdivision step:
1. **Face points:** Average of all vertices of face
2. **Edge points:** Average of edge endpoints and adjacent face points
3. **Vertex points:** (Q + 2R + (n-3)S) / n
   - Q = average of new face points
   - R = average of edge midpoints
   - S = original vertex
   - n = valence

---

## Grading Alignment

| Requirement | Points | Plan Section |
|-------------|--------|--------------|
| Technique 1 (Parametric/Subdivision) | 7 | Part 3, Phase 3 |
| Technique 2 (Parametric/Subdivision) | 7 | Part 3, Phase 4 |
| View Frustum Culling | 7 | Part 1, Phases 1 |
| Level of Detail | 5 | Part 2, Phase 2 |
| Scene Definition | 2 | Part 4, Phase 5 |
| Documentation/Screenshots | 2 | Phase 7 |
| **Total** | **30** | |
