#include "geometry/types.hpp"

namespace cg
{

VertexAndNormal::VertexAndNormal()
{
    vertex.set(0.0f, 0.0f, 0.0f);
    normal.set(0.0f, 0.0f, 0.0f);
}

VertexAndNormal::VertexAndNormal(const Point3 &v)
{
    vertex = v;
    normal.set(0.0f, 0.0f, 0.0f);
}

TextureCoord2::TextureCoord2() : s(0.0f), t(0.0f) {}

TextureCoord2::TextureCoord2(float s_in, float t_in) : s(s_in), t(t_in) {}

PNTVertex::PNTVertex() : vertex(0.0f, 0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f), texture(0.0f, 0.0f) {}

PNTVertex::PNTVertex(const Point3 &v) : vertex(v), normal(0.0f, 0.0f, 0.0f), texture(0.0f, 0.0f) {}

PNTVertex::PNTVertex(const Point3 &v, const Vector3 &n, const Point2 &tex) :
    vertex(v), normal(n), texture(tex)
{
}

} // namespace cg
