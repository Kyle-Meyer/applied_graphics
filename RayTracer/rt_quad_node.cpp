#include "RayTracer/rt_quad_node.hpp"
#include "geometry/geometry.hpp"

namespace cg
{

RTQuadNode::RTQuadNode(const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &v3)
    : v0_(v0), v1_(v1), v2_(v2), v3_(v3)
{
    // Compute edges for triangle intersection
    edge1_ = Vector3(v0_, v1_);
    edge2_ = Vector3(v0_, v3_);

    // Compute normal using cross product of two edges
    normal_ = edge1_.cross(edge2_);
    normal_.normalize();
}

bool RTQuadNode::intersect(const Ray3 &ray, float &t) const
{
    // Split quad into 2 triangles and test both
    // Triangle 1: v0, v1, v2
    // Triangle 2: v0, v2, v3

    // Test triangle 1 (v0, v1, v2) using Möller-Trumbore
    RayTriangleIntersectResult result1 = ray.intersect(v0_, v1_, v2_);
    if (result1.intersects && result1.distance > EPSILON)
    {
        t = result1.distance;
        return true;
    }

    // Test triangle 2 (v0, v2, v3)
    RayTriangleIntersectResult result2 = ray.intersect(v0_, v2_, v3_);
    if (result2.intersects && result2.distance > EPSILON)
    {
        t = result2.distance;
        return true;
    }

    return false;
}

Vector3 RTQuadNode::get_normal(const Point3 &int_pt)
{
    return normal_;
}

Point2 RTQuadNode::get_texture_coord(const Point3 &int_pt)
{
    // Project intersection point onto the quad's local coordinate system
    // Use bilinear interpolation
    Vector3 v0_to_pt(v0_, int_pt);

    // Compute basis vectors along edges
    Vector3 u_axis(v0_, v1_);
    Vector3 v_axis(v0_, v3_);

    float u_len = u_axis.norm();
    float v_len = v_axis.norm();

    u_axis.normalize();
    v_axis.normalize();

    // Project point onto axes
    float s = v0_to_pt.dot(u_axis) / u_len;
    float t = v0_to_pt.dot(v_axis) / v_len;

    // Clamp to [0, 1]
    s = std::max(0.0f, std::min(1.0f, s));
    t = std::max(0.0f, std::min(1.0f, t));

    return Point2(s, t);
}

void RTQuadNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    float t;
    if (intersect(ray, t))
    {
        if (t < closest.t_min)
        {
            closest.t_min = t;
            closest.geometry_node = this;
            closest.material_node = current_state.material_node;
            closest.texture_node = current_state.texture_node;

            if (current_state.transform_required)
            {
                closest.transform_required = true;
                closest.inverse_matrix = current_state.inverse_matrix;
                closest.normal_matrix = current_state.normal_matrix;
            }
        }
    }
}

bool RTQuadNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state)
{
    // Skip self-intersection for convex objects
    if (this == current_state.geometry_node)
    {
        return false;
    }

    float t;
    if (intersect(ray, t))
    {
        return (t > EPSILON && t < d);
    }
    return false;
}

bool RTQuadNode::is_convex(void) const
{
    return true;
}

} // namespace cg
