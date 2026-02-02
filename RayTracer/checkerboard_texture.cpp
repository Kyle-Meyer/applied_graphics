#include "RayTracer/checkerboard_texture.hpp"

#include <cmath>

namespace cg
{

CheckerboardTexture::CheckerboardTexture(float scale)
    : color1_(1.0f, 1.0f, 1.0f),  // White
      color2_(0.0f, 0.0f, 0.0f),  // Black
      scale_(scale)
{
}

CheckerboardTexture::CheckerboardTexture(const Color3& color1, const Color3& color2, float scale)
    : color1_(color1),
      color2_(color2),
      scale_(scale)
{
}

Color3 CheckerboardTexture::get_color(const Point3& p)
{
    // Scale the coordinates
    float x = p.x / scale_;
    float y = p.y / scale_;
    float z = p.z / scale_;

    // Use floor to get integer grid cell, then check parity
    // Adding 0.5 before floor helps with numerical stability near zero
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    int iz = static_cast<int>(std::floor(z));

    // XOR pattern: alternate colors based on sum of coordinates
    if ((ix + iy + iz) % 2 == 0)
    {
        return color1_;
    }
    else
    {
        return color2_;
    }
}

} // namespace cg
