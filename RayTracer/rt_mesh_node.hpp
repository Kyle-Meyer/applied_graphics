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
     * Uses stored per-vertex UV coordinates when available; falls back to
     * barycentric coordinates otherwise.
     * @param  int_pt  Intersection point on the mesh surface
     * @return Returns the texture coordinate (s, t) at the intersection point
     */
    Point2 get_texture_coord(const Point3 &int_pt) override;

    /**
     * Get the surface tangent at an intersection point (for TBN normal mapping).
     * Barycentrically interpolates stored per-vertex tangents.
     * @param  int_pt  Intersection point on the mesh surface
     * @return Returns the interpolated tangent vector (object space)
     */
    Vector3 get_tangent(const Point3 &int_pt) override;

    /**
     * Supply per-vertex UV texture coordinates and tangent vectors.
     * Both vectors must be the same size as the vertex list.
     * @param  tex_coords  UV coordinates per vertex
     * @param  tangents    Tangent vectors per vertex (object space)
     */
    void set_tangents_and_uvs(const std::vector<Point2> &tex_coords,
                              const std::vector<Vector3> &tangents);

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

    // Optional per-vertex UV coords and tangents (set via set_tangents_and_uvs)
    std::vector<Point2>   tex_coords_;
    std::vector<Vector3>  tangents_;

    // Per-thread hit cache — see rt_mesh_node.cpp tl_hit_cache for storage.
    // (Removed mutable instance members to fix multi-thread race condition)

    // ---- BVH ----
    // Flat binary BVH over the mesh triangles.
    // Internal nodes: tri_count == 0, left_child/right_child are indices into bvh_nodes_.
    // Leaf nodes:     tri_count >  0, left_child is the start offset into bvh_tris_.
    struct BVHNode {
        AABB     bounds;
        uint32_t left_child;   // internal: left child idx; leaf: first tri in bvh_tris_
        uint32_t right_child;  // internal: right child idx; leaf: unused
        uint32_t tri_count;    // 0 = internal, >0 = leaf
    };
    std::vector<BVHNode>   bvh_nodes_; // flat node array, root at index 0
    std::vector<uint32_t>  bvh_tris_;  // triangle indices reordered for BVH leaves

    /**
     * Compute vertex normals from face normals (for simple constructor)
     */
    void compute_normals();

    /**
     * Build the whole-mesh AABB from vertices
     */
    void build_aabb();

    /**
     * Build the BVH over all triangles. Called once after geometry is loaded.
     */
    void build_bvh();

    /**
     * Recursive BVH build helper.
     * @param centroids  Per-triangle centroid array (indexed by triangle number)
     * @param start      First index in bvh_tris_ for this subtree
     * @param end        One-past-last index in bvh_tris_ for this subtree
     * @return           Index of the newly created node in bvh_nodes_
     */
    uint32_t build_bvh_recursive(const std::vector<Point3> &centroids,
                                  uint32_t start, uint32_t end);
};

} // namespace cg

#endif
