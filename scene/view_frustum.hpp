//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    viewfrustam.hpp
//	Purpose: Methods to construct the view frustum and check whether
//          bounding volumes are in the view frustum.
//
//============================================================================

#ifndef __SCENE_VIEW_FRUSTUM_HPPP__
#define __SCENE_VIEW_FRUSTUM_HPPP__

#include "geometry/aabb.hpp"
#include "geometry/bounding_sphere.hpp"
#include "geometry/plane.hpp"
#include "scene/camera_node.hpp"

#include <array>
#include <memory>

namespace cg
{

constexpr uint16_t NUM_PLANES = 6;

enum class FrustumPlane
{
    LEFT = 0,
    RIGHT = 1,
    BOTTOM = 2,
    TOP = 3,
    NEAR_PLANE = 4, // "NEAR" causes a compiler error in some Windows architectures
    FAR_PLANE = 5   // "FAR" causes a compiler error in some Windows architectures
};

enum class FrustumIntersectType
{
    OUTSIDE,
    INSIDE,
    INTERSECT
};

/**
 * View frustum class. Data includes the 6 frustum planes. Methods include
 * tests of BVs against the frustum to determine the intersection "state"
 * of the BV and the frustum. A method to construct the frustum given the
 * camera is provided.
 */
class ViewFrustum
{
  public:
    ViewFrustum();

    /**
     * Construct a view frustum from the definition of the view held within
     * the CameraNode class. NOTE - this method is defined in scene.h due
     * to dependencies and include order.
     * @param  camera   Camera / view class.
     */
    void construct(std::shared_ptr<CameraNode> camera);

    /**
     * Determine the intersection 'state" of a bounding sphere and the
     * frustum planes
     * @param   sphere   Bounding sphere
     * @return  Returns the intersection type: outside, inside, or intersect.
     */
    FrustumIntersectType intersect(const BoundingSphere &sphere) const;

    /**
     * Determine the intersection 'state" of an AABB and the
     * frustum planes
     * @param   box   AABB
     * @return  Returns the intersection type: outside, inside, or intersect.
     */
    FrustumIntersectType intersect(const AABB &box) const;

  private:
    // View frustum planes
    std::array<Plane, NUM_PLANES> planes_;
};

} // namespace cg

#endif
