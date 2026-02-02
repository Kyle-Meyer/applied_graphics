//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	File:    checkerboard_texture.hpp
//	Purpose: Procedural checkerboard texture
//
//============================================================================

#ifndef __RAY_TRACER_CHECKERBOARD_TEXTURE_HPP__
#define __RAY_TRACER_CHECKERBOARD_TEXTURE_HPP__

#include "RayTracer/procedural_texture.hpp"

namespace cg
{

/**
 * Procedural checkerboard texture. Creates alternating colored squares
 * based on 3D world coordinates.
 */
class CheckerboardTexture : public ProceduralTexture
{
  public:
    /**
     * Constructor with default black and white colors.
     * @param scale  Size of each checker square (default 1.0)
     */
    CheckerboardTexture(float scale = 1.0f);

    /**
     * Constructor with custom colors.
     * @param color1  First checker color
     * @param color2  Second checker color
     * @param scale   Size of each checker square
     */
    CheckerboardTexture(const Color3& color1, const Color3& color2, float scale = 1.0f);

    /**
     * Get the color based on a 3D point using checkerboard pattern.
     * @param   p   Point to compute color
     * @return  Returns color1 or color2 based on checker pattern
     */
    Color3 get_color(const Point3& p) override;

  private:
    Color3 color1_;
    Color3 color2_;
    float  scale_;
};

} // namespace cg

#endif
