#include "RayTracer/rt_mesh_node.hpp"
#include "geometry/geometry.hpp"

#include <algorithm>
#include <cmath>

namespace cg
{

// Thread-local hit cache: each render thread stores its own last-hit info.
// Tagged with a mesh pointer so get_normal/get_texture_coord/get_tangent can
// assert they're reading data from the right mesh.
namespace {
struct HitCache {
    const RTMeshNode *mesh    = nullptr;
    uint32_t          face    = 0;
    float             bary_u  = 0.0f;
    float             bary_v  = 0.0f;
};
thread_local HitCache tl_hit;

// Max triangles per BVH leaf node. 4 is a good balance between tree depth and
// per-leaf work. Larger values (8) reduce memory but test more triangles per hit.
static constexpr uint32_t BVH_LEAF_SIZE = 4;

// Max BVH traversal stack depth. log2(65536/4) + a few levels of margin = 16+8 = 24;
// 64 is very conservative and safe for any realistic mesh.
static constexpr int BVH_STACK_SIZE = 64;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructors
// ─────────────────────────────────────────────────────────────────────────────

RTMeshNode::RTMeshNode(const std::vector<VertexAndNormal> &vertices,
                       const std::vector<uint16_t> &faces)
    : vertices_(vertices), faces_(faces)
{
    build_aabb();
    build_bvh();
}

RTMeshNode::RTMeshNode(const std::vector<Point3> &vertices,
                       const std::vector<uint16_t> &faces)
    : faces_(faces)
{
    // Convert Point3 to VertexAndNormal
    vertices_.resize(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        vertices_[i].vertex = vertices[i];
        vertices_[i].normal = Vector3(0, 0, 0);
    }
    compute_normals();
    build_aabb();
    build_bvh();
}

// ─────────────────────────────────────────────────────────────────────────────
// Normal / UV / tangent helpers
// ─────────────────────────────────────────────────────────────────────────────

void RTMeshNode::compute_normals()
{
    // Reset all normals to zero
    for (auto &v : vertices_)
        v.normal = Vector3(0, 0, 0);

    // Accumulate face normals to vertices (weighted by triangle area)
    for (size_t i = 0; i < faces_.size(); i += 3)
    {
        uint16_t i0 = faces_[i];
        uint16_t i1 = faces_[i + 1];
        uint16_t i2 = faces_[i + 2];

        Point3 &p0 = vertices_[i0].vertex;
        Point3 &p1 = vertices_[i1].vertex;
        Point3 &p2 = vertices_[i2].vertex;

        Vector3 e1(p0, p1);
        Vector3 e2(p0, p2);
        Vector3 face_normal = e1.cross(e2);

        vertices_[i0].normal = vertices_[i0].normal + face_normal;
        vertices_[i1].normal = vertices_[i1].normal + face_normal;
        vertices_[i2].normal = vertices_[i2].normal + face_normal;
    }

    for (auto &v : vertices_)
        v.normal.normalize();
}

void RTMeshNode::build_aabb()
{
    if (vertices_.empty()) return;

    Point3 min_pt = vertices_[0].vertex;
    Point3 max_pt = vertices_[0].vertex;

    for (const auto &v : vertices_)
    {
        min_pt.x = std::min(min_pt.x, v.vertex.x);
        min_pt.y = std::min(min_pt.y, v.vertex.y);
        min_pt.z = std::min(min_pt.z, v.vertex.z);

        max_pt.x = std::max(max_pt.x, v.vertex.x);
        max_pt.y = std::max(max_pt.y, v.vertex.y);
        max_pt.z = std::max(max_pt.z, v.vertex.z);
    }

    aabb_ = AABB(min_pt, max_pt);
}

// ─────────────────────────────────────────────────────────────────────────────
// BVH build
// ─────────────────────────────────────────────────────────────────────────────

void RTMeshNode::build_bvh()
{
    const uint32_t num_tris = static_cast<uint32_t>(faces_.size() / 3);
    if (num_tris == 0) return;

    // Initialise triangle index list (will be reordered during build)
    bvh_tris_.resize(num_tris);
    for (uint32_t i = 0; i < num_tris; ++i)
        bvh_tris_[i] = i;

    // Compute centroid of each triangle once; used for split-axis selection
    std::vector<Point3> centroids(num_tris);
    for (uint32_t i = 0; i < num_tris; ++i)
    {
        const Point3 &a = vertices_[faces_[i * 3    ]].vertex;
        const Point3 &b = vertices_[faces_[i * 3 + 1]].vertex;
        const Point3 &c = vertices_[faces_[i * 3 + 2]].vertex;
        centroids[i] = Point3((a.x + b.x + c.x) * (1.0f / 3.0f),
                              (a.y + b.y + c.y) * (1.0f / 3.0f),
                              (a.z + b.z + c.z) * (1.0f / 3.0f));
    }

    // A binary tree on num_tris leaves has at most 2*num_tris-1 nodes.
    // Reserve upfront so no reallocation occurs during recursion (the recursive
    // build accesses nodes by index after sub-calls, so stable storage matters).
    bvh_nodes_.reserve(num_tris * 2);
    build_bvh_recursive(centroids, 0, num_tris);
}

uint32_t RTMeshNode::build_bvh_recursive(const std::vector<Point3> &centroids,
                                          uint32_t start, uint32_t end)
{
    // Allocate a placeholder node; index is stable because we reserved enough capacity.
    const uint32_t node_idx = static_cast<uint32_t>(bvh_nodes_.size());
    bvh_nodes_.push_back(BVHNode{});

    // ── Compute tight AABB for all triangles in [start, end) ──────────────────
    AABB node_bounds;   // default: min=+inf, max=-inf (empty box)
    for (uint32_t i = start; i < end; ++i)
    {
        const uint32_t tri = bvh_tris_[i];
        const Point3  &a   = vertices_[faces_[tri * 3    ]].vertex;
        const Point3  &b   = vertices_[faces_[tri * 3 + 1]].vertex;
        const Point3  &c   = vertices_[faces_[tri * 3 + 2]].vertex;
        node_bounds.merge(AABB(a, a));
        node_bounds.merge(AABB(b, b));
        node_bounds.merge(AABB(c, c));
    }
    bvh_nodes_[node_idx].bounds = node_bounds;

    const uint32_t count = end - start;

    // ── Leaf node ─────────────────────────────────────────────────────────────
    if (count <= BVH_LEAF_SIZE)
    {
        bvh_nodes_[node_idx].left_child  = start;
        bvh_nodes_[node_idx].right_child = 0;      // unused for leaves
        bvh_nodes_[node_idx].tri_count   = count;
        return node_idx;
    }

    // ── Internal node: split on longest centroid-range axis ───────────────────
    // Find bounding box of triangle centroids
    Point3 cmin = centroids[bvh_tris_[start]];
    Point3 cmax = cmin;
    for (uint32_t i = start + 1; i < end; ++i)
    {
        const Point3 &ct = centroids[bvh_tris_[i]];
        cmin.x = std::min(cmin.x, ct.x);  cmax.x = std::max(cmax.x, ct.x);
        cmin.y = std::min(cmin.y, ct.y);  cmax.y = std::max(cmax.y, ct.y);
        cmin.z = std::min(cmin.z, ct.z);  cmax.z = std::max(cmax.z, ct.z);
    }

    const float dx = cmax.x - cmin.x;
    const float dy = cmax.y - cmin.y;
    const float dz = cmax.z - cmin.z;
    const int   axis = (dx >= dy && dx >= dz) ? 0 : (dy >= dz) ? 1 : 2;

    // Partition around median centroid on chosen axis (O(n), stable split)
    const uint32_t mid = (start + end) / 2;
    std::nth_element(
        bvh_tris_.begin() + start,
        bvh_tris_.begin() + mid,
        bvh_tris_.begin() + end,
        [&](uint32_t a, uint32_t b) {
            const float ca = (axis == 0) ? centroids[a].x
                           : (axis == 1) ? centroids[a].y : centroids[a].z;
            const float cb = (axis == 0) ? centroids[b].x
                           : (axis == 1) ? centroids[b].y : centroids[b].z;
            return ca < cb;
        });

    // Recurse — access by index after each call because the vector may shift
    // elements (but NOT reallocate, thanks to the upfront reserve)
    bvh_nodes_[node_idx].tri_count = 0;   // mark as internal
    const uint32_t left  = build_bvh_recursive(centroids, start, mid);
    const uint32_t right = build_bvh_recursive(centroids, mid,   end);
    bvh_nodes_[node_idx].left_child  = left;
    bvh_nodes_[node_idx].right_child = right;

    return node_idx;
}

// ─────────────────────────────────────────────────────────────────────────────
// BVH traversal — closest hit
// ─────────────────────────────────────────────────────────────────────────────

void RTMeshNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    // Reject the whole mesh early with the top-level AABB.
    // Only check `intersects` here — when the ray origin is inside the AABB
    // ray.intersect() returns t_max (the exit point), not t_min (entry), so a
    // distance comparison against closest.t_min would incorrectly prune the mesh.
    RayObjectIntersectResult aabb_result = ray.intersect(aabb_);
    if (!aabb_result.intersects)
        return;

    // Stack-based BVH traversal (avoids recursion overhead)
    uint32_t stack[BVH_STACK_SIZE];
    int top = 0;
    stack[top++] = 0;  // push root

    while (top > 0)
    {
        const BVHNode &node = bvh_nodes_[stack[--top]];

        if (node.tri_count > 0)
        {
            // ── Leaf: test each triangle ───────────────────────────────────
            const uint32_t tri_end = node.left_child + node.tri_count;
            for (uint32_t ti = node.left_child; ti < tri_end; ++ti)
            {
                const uint32_t tri = bvh_tris_[ti];
                const uint16_t i0  = faces_[tri * 3    ];
                const uint16_t i1  = faces_[tri * 3 + 1];
                const uint16_t i2  = faces_[tri * 3 + 2];

                RayTriangleIntersectResult result = ray.intersect(
                    vertices_[i0].vertex, vertices_[i1].vertex, vertices_[i2].vertex);

                if (result.intersects && result.distance > EPSILON
                                      && result.distance < closest.t_min)
                {
                    closest.t_min         = result.distance;
                    closest.geometry_node = this;
                    closest.material_node = current_state.material_node;
                    closest.texture_node  = current_state.texture_node;
                    closest.normal_map_node = current_state.normal_map_node;

                    tl_hit.mesh   = this;
                    tl_hit.face   = tri;
                    tl_hit.bary_u = result.barycentric_u;
                    tl_hit.bary_v = result.barycentric_v;

                    if (current_state.transform_required)
                    {
                        closest.transform_required = true;
                        closest.inverse_matrix     = current_state.inverse_matrix;
                        closest.normal_matrix      = current_state.normal_matrix;
                    }
                }
            }
        }
        else
        {
            // ── Internal: test both child AABBs, push closer child last ───
            // (stack is LIFO, so pushing the farther child first means the
            //  nearer child is popped and visited first)
            const uint32_t lc = node.left_child;
            const uint32_t rc = node.right_child;

            RayObjectIntersectResult rl = ray.intersect(bvh_nodes_[lc].bounds);
            RayObjectIntersectResult rr = ray.intersect(bvh_nodes_[rc].bounds);

            // Gate only on `intersects`, not distance: ray.intersect(AABB) returns
            // t_max when the origin is inside the AABB, which is not a valid entry-
            // point lower bound and would prune nodes that contain closer triangles.
            // We still use the distance to order pushes (nearer child popped first).
            if (rl.intersects && rr.intersects)
            {
                // Push farther first so nearer is popped first (front-to-back)
                if (rl.distance <= rr.distance)
                {
                    stack[top++] = rc;
                    stack[top++] = lc;
                }
                else
                {
                    stack[top++] = lc;
                    stack[top++] = rc;
                }
            }
            else if (rl.intersects) stack[top++] = lc;
            else if (rr.intersects) stack[top++] = rc;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// BVH traversal — any hit (shadow / occlusion test)
// ─────────────────────────────────────────────────────────────────────────────

bool RTMeshNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state)
{
    // Skip self-intersection
    if (this == current_state.geometry_node)
        return false;

    // Reject whole mesh
    RayObjectIntersectResult aabb_result = ray.intersect(aabb_);
    if (!aabb_result.intersects || aabb_result.distance > d)
        return false;

    // Stack-based BVH traversal with early exit on first hit
    uint32_t stack[BVH_STACK_SIZE];
    int top = 0;
    stack[top++] = 0;

    while (top > 0)
    {
        const BVHNode &node = bvh_nodes_[stack[--top]];

        if (node.tri_count > 0)
        {
            // ── Leaf: test each triangle, exit immediately on any hit ──────
            const uint32_t tri_end = node.left_child + node.tri_count;
            for (uint32_t ti = node.left_child; ti < tri_end; ++ti)
            {
                const uint32_t tri = bvh_tris_[ti];
                const uint16_t i0  = faces_[tri * 3    ];
                const uint16_t i1  = faces_[tri * 3 + 1];
                const uint16_t i2  = faces_[tri * 3 + 2];

                RayTriangleIntersectResult result = ray.intersect(
                    vertices_[i0].vertex, vertices_[i1].vertex, vertices_[i2].vertex);

                if (result.intersects && result.distance > EPSILON && result.distance < d)
                    return true;
            }
        }
        else
        {
            // ── Internal: push children whose AABBs are within range ──────
            const uint32_t lc = node.left_child;
            const uint32_t rc = node.right_child;

            RayObjectIntersectResult rl = ray.intersect(bvh_nodes_[lc].bounds);
            RayObjectIntersectResult rr = ray.intersect(bvh_nodes_[rc].bounds);

            // Shadow rays originate above surfaces so the ray origin is always
            // outside any occluder's AABB — distance is the valid entry bound here.
            if (rl.intersects && rl.distance < d) stack[top++] = lc;
            if (rr.intersects && rr.distance < d) stack[top++] = rc;
        }
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Attribute interpolation (normal, UV, tangent) — uses thread-local hit cache
// ─────────────────────────────────────────────────────────────────────────────

Vector3 RTMeshNode::get_normal(const Point3 &int_pt)
{
    const uint32_t face = (tl_hit.mesh == this) ? tl_hit.face : 0;
    const float    bu   = (tl_hit.mesh == this) ? tl_hit.bary_u : 0.0f;
    const float    bv   = (tl_hit.mesh == this) ? tl_hit.bary_v : 0.0f;

    const uint16_t i0 = faces_[face * 3];
    const uint16_t i1 = faces_[face * 3 + 1];
    const uint16_t i2 = faces_[face * 3 + 2];

    const float w0 = 1.0f - bu - bv;
    Vector3 n = vertices_[i0].normal * w0 +
                vertices_[i1].normal * bu  +
                vertices_[i2].normal * bv;
    n.normalize();
    return n;
}

Point2 RTMeshNode::get_texture_coord(const Point3 &int_pt)
{
    const uint32_t face = (tl_hit.mesh == this) ? tl_hit.face : 0;
    const float    bu   = (tl_hit.mesh == this) ? tl_hit.bary_u : 0.0f;
    const float    bv   = (tl_hit.mesh == this) ? tl_hit.bary_v : 0.0f;

    if (!tex_coords_.empty())
    {
        const uint16_t i0 = faces_[face * 3];
        const uint16_t i1 = faces_[face * 3 + 1];
        const uint16_t i2 = faces_[face * 3 + 2];

        const float w0 = 1.0f - bu - bv;
        return Point2(
            tex_coords_[i0].x * w0 + tex_coords_[i1].x * bu + tex_coords_[i2].x * bv,
            tex_coords_[i0].y * w0 + tex_coords_[i1].y * bu + tex_coords_[i2].y * bv);
    }

    return Point2(bu, bv);
}

Vector3 RTMeshNode::get_tangent(const Point3 &int_pt)
{
    const uint32_t face = (tl_hit.mesh == this) ? tl_hit.face : 0;
    const float    bu   = (tl_hit.mesh == this) ? tl_hit.bary_u : 0.0f;
    const float    bv   = (tl_hit.mesh == this) ? tl_hit.bary_v : 0.0f;

    if (!tangents_.empty())
    {
        const uint16_t i0 = faces_[face * 3];
        const uint16_t i1 = faces_[face * 3 + 1];
        const uint16_t i2 = faces_[face * 3 + 2];

        const float w0 = 1.0f - bu - bv;
        Vector3 t = tangents_[i0] * w0 + tangents_[i1] * bu + tangents_[i2] * bv;
        t.normalize();
        return t;
    }

    return Vector3(1.0f, 0.0f, 0.0f);
}

void RTMeshNode::set_tangents_and_uvs(const std::vector<Point2> &tex_coords,
                                      const std::vector<Vector3> &tangents)
{
    tex_coords_ = tex_coords;
    tangents_   = tangents;
}

bool RTMeshNode::is_convex(void) const
{
    return false;
}

} // namespace cg
