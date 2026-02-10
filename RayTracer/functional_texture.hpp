//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//
//	File:    functional_texture.hpp
//	Purpose: Function-based procedural texture for ray tracing
//============================================================================

#ifndef __RAY_TRACER_FUNCTIONAL_TEXTURE_HPP__
#define __RAY_TRACER_FUNCTIONAL_TEXTURE_HPP__

#include "RayTracer/texture_node.hpp"

#include <cmath>
#include <functional>

namespace cg
{

/**
 * Function-based procedural texture.
 * Accepts lambdas or function pointers for flexible pattern generation.
 */
class FunctionalTexture : public TextureNode
{
  public:
    using PointPatternFn = std::function<Color3(const Point3 &)>;
    using UVPatternFn = std::function<Color3(const TextureCoord2 &)>;

    /**
     * Constructor with world-space pattern function.
     * @param point_pattern Function that returns color given a 3D point
     */
    FunctionalTexture(PointPatternFn point_pattern);

    /**
     * Constructor with both pattern functions.
     * @param point_pattern Function for world-space coordinates
     * @param uv_pattern    Function for UV texture coordinates
     */
    FunctionalTexture(PointPatternFn point_pattern, UVPatternFn uv_pattern);

    /**
     * Get color based on 3D world-space position.
     */
    Color3 get_color(const Point3 &p) override;

    /**
     * Get color based on texture coordinates.
     */
    Color3 get_color(const TextureCoord2 &tex_coord) override;

    // ========== Static factory methods for common patterns ==========

    /**
     * Create a checkerboard pattern texture.
     * @param color1 First color
     * @param color2 Second color
     * @param scale  Checker size (higher = smaller checkers)
     */
    static std::shared_ptr<FunctionalTexture> checkerboard(
        const Color3 &color1, const Color3 &color2, float scale = 1.0f);

    /**
     * Create a stripe pattern texture.
     * @param color1    First color
     * @param color2    Second color
     * @param scale     Stripe width
     * @param axis      0=X, 1=Y, 2=Z axis for stripes
     */
    static std::shared_ptr<FunctionalTexture> stripes(
        const Color3 &color1, const Color3 &color2, float scale = 1.0f, int axis = 1);

    /**
     * Create a gradient pattern based on Y height.
     * @param bottom_color Color at y=0
     * @param top_color    Color at y=1
     */
    static std::shared_ptr<FunctionalTexture> gradient(
        const Color3 &bottom_color, const Color3 &top_color);

    /**
     * Create a marble pattern using Perlin noise turbulence.
     * @param base_color  Base marble color (e.g., white)
     * @param vein_color  Vein/streak color (e.g., dark gray or dark red)
     * @param scale       Frequency of the marble pattern
     * @param turbulence_strength  How much turbulence distorts the veins
     */
    static std::shared_ptr<FunctionalTexture> marble(
        const Color3 &base_color, const Color3 &vein_color,
        float scale = 1.0f, float turbulence_strength = 5.0f);

  private:
    PointPatternFn point_pattern_;
    UVPatternFn    uv_pattern_;
};

} // namespace cg

#endif
