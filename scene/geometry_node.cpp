#include "scene/geometry_node.hpp"

#include "geometry/point2.hpp"
#include "geometry/vector3.hpp"

namespace cg
{

GeometryNode::GeometryNode() { node_type_ = SceneNodeType::GEOMETRY; }

GeometryNode::~GeometryNode() {}

void GeometryNode::draw(SceneState &scene_state) {}

Vector3 GeometryNode::get_normal(const Point3 &int_pt)
{
    // Default implementation - ray tracing geometry nodes should override this
    return Vector3(0.0f, 1.0f, 0.0f);
}

Point2 GeometryNode::get_texture_coord(const Point3 &int_pt)
{
    // Default implementation - ray tracing geometry nodes should override this
    return Point2(0.0f, 0.0f);
}

} // namespace cg
