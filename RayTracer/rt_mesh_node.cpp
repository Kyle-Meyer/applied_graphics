#include "RayTracer/rt_mesh_node.hpp"
#include "geometry/geometry.hpp"

#include <cmath>

namespace cg
{

RTMeshNode::RTMeshNode(const std::vector<VertexAndNormal> &vertices,
                       const std::vector<uint16_t> &faces)
    : vertices_(vertices), faces_(faces), last_face_index_(0),
      last_bary_u_(0), last_bary_v_(0)
{
    build_aabb();
}

RTMeshNode::RTMeshNode(const std::vector<Point3> &vertices,
                       const std::vector<uint16_t> &faces)
    : faces_(faces), last_face_index_(0), last_bary_u_(0), last_bary_v_(0)
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
    // Interpolate vertex normals using barycentric coordinates
    uint16_t i0 = faces_[last_face_index_ * 3];
    uint16_t i1 = faces_[last_face_index_ * 3 + 1];
    uint16_t i2 = faces_[last_face_index_ * 3 + 2];

    float w0 = 1.0f - last_bary_u_ - last_bary_v_;
    float w1 = last_bary_u_;
    float w2 = last_bary_v_;

    Vector3 n = vertices_[i0].normal * w0 +
                vertices_[i1].normal * w1 +
                vertices_[i2].normal * w2;
    n.normalize();
    return n;
}

Point2 RTMeshNode::get_texture_coord(const Point3 &int_pt)
{
    // Simple planar mapping using barycentric coordinates
    // Maps u,v barycentric to s,t texture coords
    return Point2(last_bary_u_, last_bary_v_);
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

            // Store barycentric coords for normal/texcoord interpolation
            last_face_index_ = i / 3;
            last_bary_u_ = result.barycentric_u;
            last_bary_v_ = result.barycentric_v;

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
