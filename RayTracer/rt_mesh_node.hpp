//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//
//	File:    rt_mesh_node.hpp
//	Purpose: Triangle mesh for use in ray tracing with AABB bounding volume.
//============================================================================

#ifndef __RAY_TRACER_RT_MESH_NODE_HPP__
#define __RAY_TRACER_RT_MESH_NODE_HPP__

#include "geometry/point3.hpp"
#include "geometry/vector3.hpp"
#include "geometry/ray3.hpp"
#include "geometry/aabb.hpp"
#include "geometry/types.hpp"
#include "scene/geometry_node.hpp"

#include <vector>

namespace cg
{

/**
 * Triangle mesh for ray tracing with AABB bounding volume.
 */
class RTMeshNode : public GeometryNode
{
  public:
    /**
     * Constructor for a ray traced mesh
     * @param  vertices  List of vertices (position and normal)
     * @param  faces     Index list for triangles (3 indices per triangle)
     */
    RTMeshNode(const std::vector<VertexAndNormal> &vertices,
               const std::vector<uint16_t> &faces);

    /**
     * Constructor for a simple mesh from points (computes normals)
     * @param  vertices  List of vertex positions
     * @param  faces     Index list for triangles (3 indices per triangle)
     */
    RTMeshNode(const std::vector<Point3> &vertices,
               const std::vector<uint16_t> &faces);

    /**
     * Gets the normal at the intersection point.
     * Uses the stored intersection face to interpolate vertex normals.
     * @param   int_pt  Intersection point
     * @return  Returns a unit length normal at the intersection point.
     */
    Vector3 get_normal(const Point3 &int_pt) override;

    /**
     * Get the texture coordinate at an intersection point.
     * Uses barycentric interpolation.
     * @param  int_pt  Intersection point on the mesh surface
     * @return Returns the texture coordinate (s, t) at the intersection point
     */
    Point2 get_texture_coord(const Point3 &int_pt) override;

    /**
     * Ray tracing intersect method - finds closest intersection
     */
    void find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest) override;

    /**
     * Ray tracing intersect method - checks if intersection exists within distance d
     */
    bool does_intersect_exist(Ray3 ray, float d, SceneState &current_state) override;

    /**
     * Meshes are generally not convex.
     */
    bool is_convex(void) const;

    /**
     * Get the AABB for this mesh
     */
    const AABB& get_aabb() const { return aabb_; }

  private:
    std::vector<VertexAndNormal> vertices_;
    std::vector<uint16_t> faces_;
    AABB aabb_;

    // Store last intersection info for normal/texcoord computation
    mutable uint32_t last_face_index_;
    mutable float last_bary_u_, last_bary_v_;

    /**
     * Compute vertex normals from face normals (for simple constructor)
     */
    void compute_normals();

    /**
     * Build the AABB from vertices
     */
    void build_aabb();
};

} // namespace cg

#endif
