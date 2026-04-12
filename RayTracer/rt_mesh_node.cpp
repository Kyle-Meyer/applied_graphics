#include "RayTracer/rt_mesh_node.hpp"
#include "geometry/geometry.hpp"

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
}

RTMeshNode::RTMeshNode(const std::vector<VertexAndNormal> &vertices,
                       const std::vector<uint16_t> &faces)
    : vertices_(vertices), faces_(faces)
{
    build_aabb();
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
}

void RTMeshNode::compute_normals()
{
    // Reset all normals to zero
    for (auto &v : vertices_)
    {
        v.normal = Vector3(0, 0, 0);
    }

    // Accumulate face normals to vertices
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
        // Don't normalize - weight by triangle area

        vertices_[i0].normal = vertices_[i0].normal + face_normal;
        vertices_[i1].normal = vertices_[i1].normal + face_normal;
        vertices_[i2].normal = vertices_[i2].normal + face_normal;
    }

    // Normalize all vertex normals
    for (auto &v : vertices_)
    {
        v.normal.normalize();
    }
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

Vector3 RTMeshNode::get_normal(const Point3 &int_pt)
{
    // Use this thread's cached hit (set by find_closest_intersect on the same thread).
    uint32_t face = (tl_hit.mesh == this) ? tl_hit.face : 0;
    float    bu   = (tl_hit.mesh == this) ? tl_hit.bary_u : 0.0f;
    float    bv   = (tl_hit.mesh == this) ? tl_hit.bary_v : 0.0f;

    uint16_t i0 = faces_[face * 3];
    uint16_t i1 = faces_[face * 3 + 1];
    uint16_t i2 = faces_[face * 3 + 2];

    float w0 = 1.0f - bu - bv;
    Vector3 n = vertices_[i0].normal * w0 +
                vertices_[i1].normal * bu +
                vertices_[i2].normal * bv;
    n.normalize();
    return n;
}

Point2 RTMeshNode::get_texture_coord(const Point3 &int_pt)
{
    uint32_t face = (tl_hit.mesh == this) ? tl_hit.face : 0;
    float    bu   = (tl_hit.mesh == this) ? tl_hit.bary_u : 0.0f;
    float    bv   = (tl_hit.mesh == this) ? tl_hit.bary_v : 0.0f;

    if (!tex_coords_.empty())
    {
        uint16_t i0 = faces_[face * 3];
        uint16_t i1 = faces_[face * 3 + 1];
        uint16_t i2 = faces_[face * 3 + 2];

        float w0 = 1.0f - bu - bv;
        return Point2(
            tex_coords_[i0].x * w0 + tex_coords_[i1].x * bu + tex_coords_[i2].x * bv,
            tex_coords_[i0].y * w0 + tex_coords_[i1].y * bu + tex_coords_[i2].y * bv);
    }

    return Point2(bu, bv);
}

Vector3 RTMeshNode::get_tangent(const Point3 &int_pt)
{
    uint32_t face = (tl_hit.mesh == this) ? tl_hit.face : 0;
    float    bu   = (tl_hit.mesh == this) ? tl_hit.bary_u : 0.0f;
    float    bv   = (tl_hit.mesh == this) ? tl_hit.bary_v : 0.0f;

    if (!tangents_.empty())
    {
        uint16_t i0 = faces_[face * 3];
        uint16_t i1 = faces_[face * 3 + 1];
        uint16_t i2 = faces_[face * 3 + 2];

        float w0 = 1.0f - bu - bv;
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

void RTMeshNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    // First check AABB
    RayObjectIntersectResult aabb_result = ray.intersect(aabb_);
    if (!aabb_result.intersects)
    {
        return;  // Ray misses bounding box, skip all triangles
    }

    // Test all triangles
    for (size_t i = 0; i < faces_.size(); i += 3)
    {
        uint16_t i0 = faces_[i];
        uint16_t i1 = faces_[i + 1];
        uint16_t i2 = faces_[i + 2];

        const Point3 &v0 = vertices_[i0].vertex;
        const Point3 &v1 = vertices_[i1].vertex;
        const Point3 &v2 = vertices_[i2].vertex;

        RayTriangleIntersectResult result = ray.intersect(v0, v1, v2);

        if (result.intersects && result.distance > EPSILON && result.distance < closest.t_min)
        {
            closest.t_min = result.distance;
            closest.geometry_node = this;
            closest.material_node = current_state.material_node;
            closest.texture_node = current_state.texture_node;
            closest.normal_map_node = current_state.normal_map_node;

            // Store barycentric coords in the per-thread cache (thread-safe).
            tl_hit.mesh   = this;
            tl_hit.face   = i / 3;
            tl_hit.bary_u = result.barycentric_u;
            tl_hit.bary_v = result.barycentric_v;

            if (current_state.transform_required)
            {
                closest.transform_required = true;
                closest.inverse_matrix = current_state.inverse_matrix;
                closest.normal_matrix = current_state.normal_matrix;
            }
        }
    }
}

bool RTMeshNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state)
{
    // Skip self-intersection
    if (this == current_state.geometry_node)
    {
        return false;
    }

    // First check AABB
    RayObjectIntersectResult aabb_result = ray.intersect(aabb_);
    if (!aabb_result.intersects || aabb_result.distance > d)
    {
        return false;  // Ray misses bounding box or box is beyond distance
    }

    // Test all triangles
    for (size_t i = 0; i < faces_.size(); i += 3)
    {
        uint16_t i0 = faces_[i];
        uint16_t i1 = faces_[i + 1];
        uint16_t i2 = faces_[i + 2];

        const Point3 &v0 = vertices_[i0].vertex;
        const Point3 &v1 = vertices_[i1].vertex;
        const Point3 &v2 = vertices_[i2].vertex;

        RayTriangleIntersectResult result = ray.intersect(v0, v1, v2);

        if (result.intersects && result.distance > EPSILON && result.distance < d)
        {
            return true;  // Found an intersection, early exit
        }
    }

    return false;
}

bool RTMeshNode::is_convex(void) const
{
    return false;  // Meshes are generally not convex
}

} // namespace cg
