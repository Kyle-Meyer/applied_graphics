//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//
//	File:    normal_map_node.hpp
//	Purpose: Scene graph node that applies a tangent-space normal map to child
//           geometry during ray tracing.
//============================================================================

#ifndef __RAY_TRACER_NORMAL_MAP_NODE_HPP__
#define __RAY_TRACER_NORMAL_MAP_NODE_HPP__

#include "geometry/point2.hpp"
#include "geometry/point3.hpp"
#include "geometry/vector3.hpp"
#include "scene/scene_node.hpp"

namespace cg
{

/**
 * Scene graph normal map node. Loads a tangent-space normal map image and
 * pushes itself into SceneState so that ray_tracer.cpp can apply TBN
 * perturbation at each hit point.
 *
 * Usage:
 *   auto nm = std::make_shared<NormalMapNode>("stone_normal.png");
 *   nm->add_child(mesh_node);
 *   parent->add_child(nm);
 *
 * The normal map image is expected to be an RGB image in [0,255] where
 * (128,128,255) encodes the unperturbed surface normal (0,0,1) in tangent space.
 */
class NormalMapNode : public SceneNode
{
  public:
    /**
     * Constructor. Loads the normal map from the given filename.
     * @param  filename  Path to an RGB normal map image (PNG, JPG, etc.)
     * @param  strength  Perturbation strength in [0,1]; 1.0 = full effect
     */
    NormalMapNode(const char *filename, float strength = 1.0f);

    ~NormalMapNode();

    /**
     * Draw. Not used for ray tracing.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Push this node as the current normal map in scene state, traverse
     * children, then pop.
     */
    void find_closest_intersect(Ray3 ray, SceneState &current_state,
                                SceneState &closest) override;

    /**
     * Sample the bump normal at a surface point.
     * @param  uv        UV texture coordinate in [0,1]^2
     * @param  world_pos World-space intersection point (available for
     *                   procedural shaders that prefer world-space coords)
     * @return Tangent-space normal vector (not necessarily unit length)
     */
    virtual Vector3 sample(const Point2 &uv, const Point3 &world_pos, float cam_dist = 0.0f) const;

    float strength() const { return strength_; }

  protected:
    /**
     * Constructor for procedural subclasses that don't load a texture file.
     * Leaves pixel_map_ null; subclasses must override sample().
     */
    explicit NormalMapNode(float strength);

  private:
    uint8_t *pixel_map_;
    int32_t  width_;
    int32_t  height_;
    float    strength_;

    // Bilinear texel fetch returning float RGB in [0,1]
    void fetch_texel(int32_t row, int32_t col,
                     float &r, float &g, float &b) const;
};

} // namespace cg

#endif
