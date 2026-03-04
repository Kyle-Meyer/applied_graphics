#include "scene/bezier_path_node.hpp"

#include "geometry/matrix.hpp"
#include "geometry/types.hpp"

#include <vector>

namespace cg
{

BezierPathNode::BezierPathNode(const Point3 &p0, const Point3 &p1,
                                const Point3 &p2, const Point3 &p3,
                                int steps,
                                int32_t position_loc, int32_t normal_loc)
    : curve_(p0, p1, p2, p3, steps),
      vao_(0),
      vbo_(0),
      sample_count_(steps + 1)
{
    // Pre-sample the curve at steps+1 evenly-spaced t values and store as
    // VertexAndNormal so we can reuse the same VAO layout as TriSurface.
    // Normals are set to (0,0,1) — an up-vector that keeps the line visible
    // under typical scene lighting regardless of orientation.
    std::vector<VertexAndNormal> pts;
    pts.reserve(static_cast<size_t>(sample_count_));

    for(int i = 0; i <= steps; ++i)
    {
        float  t   = static_cast<float>(i) / static_cast<float>(steps);
        Point3 pos = BezierCurve::evaluate(t, p0, p1, p2, p3);

        VertexAndNormal vn;
        vn.vertex = pos;
        vn.normal = Vector3(0.0f, 0.0f, 1.0f);
        pts.push_back(vn);
    }

    // Upload to GPU: one VBO, no element buffer (uses glDrawArrays)
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(pts.size() * sizeof(VertexAndNormal)),
                 pts.data(),
                 GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE,
                          sizeof(VertexAndNormal), (void *)0);
    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE,
                          sizeof(VertexAndNormal), (void *)(sizeof(Point3)));
    glEnableVertexAttribArray(position_loc);
    glEnableVertexAttribArray(normal_loc);

    glBindVertexArray(0);
}

BezierPathNode::~BezierPathNode()
{
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

void BezierPathNode::draw(SceneState &scene_state)
{
    // ------------------------------------------------------------------
    // 1. Render the full curve as a polyline (uses current material state)
    // ------------------------------------------------------------------
    glBindVertexArray(vao_);
    glDrawArrays(GL_LINE_STRIP, 0, sample_count_);
    glBindVertexArray(0);

    // ------------------------------------------------------------------
    // 2. Translate children to the current curve position and draw them
    // ------------------------------------------------------------------
    Point3 pos = curve_.current_point();

    scene_state.push_transforms();

    // Build a pure-translation matrix and right-multiply into model matrix
    Matrix4x4 T;
    T.set_identity();
    T.translate(pos.x, pos.y, pos.z);
    scene_state.model_matrix *= T;

    // Update the three transform-related uniforms (same pattern as TransformNode)
    glUniformMatrix4fv(scene_state.model_matrix_loc, 1, GL_FALSE,
                       scene_state.model_matrix.get());

    Matrix4x4 normal_matrix = scene_state.model_matrix.get_inverse().transpose();
    glUniformMatrix4fv(scene_state.normal_matrix_loc, 1, GL_FALSE,
                       normal_matrix.get());

    Matrix4x4 pvm = scene_state.pv * scene_state.model_matrix;
    glUniformMatrix4fv(scene_state.pvm_matrix_loc, 1, GL_FALSE, pvm.get());

    // Draw all children (e.g. sphere) at the translated position
    SceneNode::draw(scene_state);

    scene_state.pop_transforms();

    // ------------------------------------------------------------------
    // 3. Advance the animation by one step; loop when the end is reached
    // ------------------------------------------------------------------
    curve_.step();
    if(curve_.is_done())
        curve_.reset();
}

} // namespace cg
