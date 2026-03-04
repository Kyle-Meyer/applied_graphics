#ifndef __SCENE_BEZIER_PATH_NODE_HPP__
#define __SCENE_BEZIER_PATH_NODE_HPP__

#include "scene/bezier_curve.hpp"
#include "scene/scene_node.hpp"

#include <cstdint>

namespace cg
{

/**
 * Scene node that renders a cubic Bezier curve as a GL_LINE_STRIP and
 * animates its child nodes along that curve.
 *
 * Each call to draw():
 *  1. Renders the full curve polyline using the current material state.
 *  2. Pushes a translation to the current curve position onto the model
 *     matrix stack and draws all children (e.g. a sphere) at that position.
 *  3. Advances the curve by one step via forward differencing.
 *     When the end of the curve is reached the curve resets to t = 0,
 *     producing a continuous looping animation.
 *
 * Usage example:
 * @code
 *   auto path = std::make_shared<BezierPathNode>(
 *       p0, p1, p2, p3, 120, position_loc, normal_loc);
 *
 *   // Wrap in a material for the line colour
 *   auto line_mat = std::make_shared<PresentationNode>(...yellow...);
 *   line_mat->add_child(path);
 *
 *   // Animated sphere sits inside the path node
 *   auto sphere_mat  = std::make_shared<PresentationNode>(...red...);
 *   auto sphere_xfrm = std::make_shared<TransformNode>();
 *   sphere_xfrm->scale(2.0f, 2.0f, 2.0f);
 *   auto sphere_geo  = std::make_shared<SphereSurface>(...);
 *   add_sub_tree(path, sphere_mat, sphere_xfrm, sphere_geo);
 * @endcode
 */
class BezierPathNode : public SceneNode
{
  public:
    /**
     * Construct the node, upload the curve polyline to the GPU.
     * @param p0..p3       Cubic Bezier control points
     * @param steps        Animation steps across t in [0, 1]; also the
     *                     number of polyline segments (steps+1 vertices)
     * @param position_loc Shader attribute location for vertex position
     * @param normal_loc   Shader attribute location for vertex normal
     */
    BezierPathNode(const Point3 &p0, const Point3 &p1,
                   const Point3 &p2, const Point3 &p3,
                   int steps,
                   int32_t position_loc, int32_t normal_loc);

    ~BezierPathNode();

    /**
     * Render the curve polyline, translate children to the current
     * curve position, draw them, then advance one step.
     */
    void draw(SceneState &scene_state) override;

  private:
    BezierCurve curve_;       // Forward-differencing curve stepper
    GLuint      vao_;         // VAO for the polyline
    GLuint      vbo_;         // VBO for the polyline vertices
    int32_t     sample_count_; // Number of vertices in the line strip
};

} // namespace cg

#endif
