#include "scene/view_frustum.hpp"

namespace cg
{

ViewFrustum::ViewFrustum() {}

void ViewFrustum::construct(std::shared_ptr<CameraNode> camera)
{
    // Complete in 605.767 - Project 2
}

FrustumIntersectType ViewFrustum::intersect(const BoundingSphere &sphere) const
{
    // Complete in 605.767 - Project 2
    return FrustumIntersectType::INTERSECT;
}

FrustumIntersectType ViewFrustum::intersect(const AABB &box) const
{
    // Complete in 605.767 - Project 2
    return FrustumIntersectType::INTERSECT;
}

} // namespace cg
