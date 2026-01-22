
#include "PolygonMesh/object_creation.hpp"

#include "scene/transform_node.hpp"
#include "scene/unit_square.hpp"

namespace cg
{

namespace
{
// Convenience method to form a vertex list from a list of triangle vertices
std::vector<Point3> form_triangle(const Point3 &v0, const Point3 &v1, const Point3 &v2)
{
    return {v0, v1, v2};
}
} // namespace

std::vector<Point3> form_pentagon(
    const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &v3, const Point3 &v4)
{
    return {v0, v1, v2, v3, v4};
}

Point3 dual(const Point3 &v0, const Point3 &v1, const Point3 &v2)
{
    return Point3((v0.x + v1.x + v2.x) * 0.33333f,
                  (v0.y + v1.y + v2.y) * 0.33333f,
                  (v0.z + v1.z + v2.z) * 0.33333f);
}

std::shared_ptr<SceneNode>
    construct_cube(int32_t n, int32_t position_loc, int32_t normal_loc, int32_t texture_loc)
{
    // Create flat surface
    auto square =
        std::make_shared<TexturedUnitSquareSurface>(5, 1.0f, position_loc, normal_loc, texture_loc);

    // Top
    auto top = std::make_shared<TransformNode>();
    top->translate(0.0f, 0.0f, 0.5f);
    top->add_child(square);

    // Bottom
    auto bottom = std::make_shared<TransformNode>();
    bottom->translate(0.0f, 0.0f, -0.5f);
    bottom->rotate_x(180.0f);
    bottom->add_child(square);

    // Left
    auto left = std::make_shared<TransformNode>();
    left->translate(-0.5f, 0.0f, 0.0f);
    left->rotate_y(-90.0f);
    left->add_child(square);

    // Right
    auto right = std::make_shared<TransformNode>();
    right->translate(0.5f, 0.0f, 0.0f);
    right->rotate_y(90.0f);
    right->add_child(square);

    // Back
    auto back = std::make_shared<TransformNode>();
    back->translate(0.0f, 0.5f, 0.0f);
    back->rotate_x(-90.0f);
    back->add_child(square);

    // Front
    auto front = std::make_shared<TransformNode>();
    front->translate(0.0f, -0.5f, 0.0f);
    front->rotate_x(90.0f);
    front->add_child(square);

    auto cube = std::make_shared<SceneNode>();
    cube->add_child(top);
    cube->add_child(back);
    cube->add_child(front);
    cube->add_child(right);
    cube->add_child(left);
    cube->add_child(bottom);
    return cube;
}

std::shared_ptr<TriSurface> construct_tetrahedron(int32_t position_loc, int32_t normal_loc)
{
    // Vertices of the tetrahedron
    Point3 v0(1.0f, 1.0f, 1.0f);
    Point3 v1(1.0f, -1.0f, -1.0f);
    Point3 v2(-1.0f, -1.0f, 1.0f);
    Point3 v3(-1.0f, 1.0f, -1.0f);

    // Create faces as children of a base scene node
    auto tetrahedron = std::make_shared<TriSurface>();
    tetrahedron->add_polygon(form_triangle(v2, v3, v1));
    tetrahedron->add_polygon(form_triangle(v2, v0, v3));
    tetrahedron->add_polygon(form_triangle(v0, v1, v3));
    tetrahedron->add_polygon(form_triangle(v1, v0, v2));
    tetrahedron->create_vertex_buffers(position_loc, normal_loc);
    return tetrahedron;
}

std::shared_ptr<TriSurface> construct_octahedron(int32_t position_loc, int32_t normal_loc)
{
    // 6 vertices of the octahedron
    Point3 v0(0.0f, 0.0f, 0.5f);
    Point3 v1(0.0f, 0.0f, -0.5f);
    Point3 v2(-0.5f, 0.0f, 0.0f);
    Point3 v3(0.5f, 0.0f, 0.0f);
    Point3 v4(0.0f, 0.5f, 0.0f);
    Point3 v5(0.0f, -0.5f, 0.0f);

    // Create faces as children of a base scene node
    auto octahedron = std::make_shared<TriSurface>();
    octahedron->add_polygon(form_triangle(v0, v3, v4));
    octahedron->add_polygon(form_triangle(v0, v5, v3));
    octahedron->add_polygon(form_triangle(v0, v2, v5));
    octahedron->add_polygon(form_triangle(v0, v4, v2));
    octahedron->add_polygon(form_triangle(v4, v3, v1));
    octahedron->add_polygon(form_triangle(v3, v5, v1));
    octahedron->add_polygon(form_triangle(v5, v2, v1));
    octahedron->add_polygon(form_triangle(v2, v4, v1));
    octahedron->create_vertex_buffers(position_loc, normal_loc);
    return octahedron;
}

std::shared_ptr<TriSurface> construct_icosahedron(int32_t position_loc, int32_t normal_loc)
{
    // Vertices of the icosahedron
    Point3 v0(0.0f, 1.0f, PHI_INV);
    Point3 v1(0.0f, 1.0f, -PHI_INV);
    Point3 v2(1.0f, PHI_INV, 0.0f);
    Point3 v3(1.0f, -PHI_INV, 0.0f);
    Point3 v4(0.0f, -1.0f, -PHI_INV);
    Point3 v5(0.0f, -1.0f, PHI_INV);
    Point3 v6(PHI_INV, 0.0f, 1.0f);
    Point3 v7(-PHI_INV, 0.0f, 1.0f);
    Point3 v8(PHI_INV, 0.0f, -1.0f);
    Point3 v9(-PHI_INV, 0.0f, -1.0f);
    Point3 v10(-1.0f, PHI_INV, 0.0f);
    Point3 v11(-1.0f, -PHI_INV, 0.0f);

    // Create faces as children of a base scene node
    auto icosahedron = std::make_shared<TriSurface>();
    icosahedron->add_polygon(form_triangle(v0, v6, v2));
    icosahedron->add_polygon(form_triangle(v6, v3, v2));
    icosahedron->add_polygon(form_triangle(v6, v5, v3));
    icosahedron->add_polygon(form_triangle(v7, v5, v6));
    icosahedron->add_polygon(form_triangle(v7, v6, v0));
    icosahedron->add_polygon(form_triangle(v2, v3, v8));
    icosahedron->add_polygon(form_triangle(v1, v2, v8));
    icosahedron->add_polygon(form_triangle(v0, v2, v1));
    icosahedron->add_polygon(form_triangle(v10, v0, v1));
    icosahedron->add_polygon(form_triangle(v10, v1, v9));
    icosahedron->add_polygon(form_triangle(v1, v8, v9));
    icosahedron->add_polygon(form_triangle(v3, v4, v8));
    icosahedron->add_polygon(form_triangle(v5, v4, v3));
    icosahedron->add_polygon(form_triangle(v11, v4, v5));
    icosahedron->add_polygon(form_triangle(v11, v7, v10));
    icosahedron->add_polygon(form_triangle(v7, v0, v10));
    icosahedron->add_polygon(form_triangle(v9, v4, v11));
    icosahedron->add_polygon(form_triangle(v9, v8, v4));
    icosahedron->add_polygon(form_triangle(v11, v5, v7));
    icosahedron->add_polygon(form_triangle(v11, v10, v9));
    icosahedron->create_vertex_buffers(position_loc, normal_loc);
    return icosahedron;
}

std::shared_ptr<TriSurface> construct_dodecahedron(int32_t position_loc, int32_t normal_loc)
{
    // Vertices of the icosahedron
    Point3 iv0(0.0f, 1.0f, PHI_INV);
    Point3 iv1(0.0f, 1.0f, -PHI_INV);
    Point3 iv2(1.0f, PHI_INV, 0.0f);
    Point3 iv3(1.0f, -PHI_INV, 0.0f);
    Point3 iv4(0.0f, -1.0f, -PHI_INV);
    Point3 iv5(0.0f, -1.0f, PHI_INV);
    Point3 iv6(PHI_INV, 0.0f, 1.0f);
    Point3 iv7(-PHI_INV, 0.0f, 1.0f);
    Point3 iv8(PHI_INV, 0.0f, -1.0f);
    Point3 iv9(-PHI_INV, 0.0f, -1.0f);
    Point3 iv10(-1.0f, PHI_INV, 0.0f);
    Point3 iv11(-1.0f, -PHI_INV, 0.0f);

    // Vertices of the dodecahecdron are formed by the dual of the icosahedron
    Point3 v0 = dual(iv0, iv6, iv2);
    Point3 v1 = dual(iv6, iv3, iv2);
    Point3 v2 = dual(iv6, iv5, iv3);
    Point3 v3 = dual(iv7, iv5, iv6);
    Point3 v4 = dual(iv7, iv6, iv0);
    Point3 v5 = dual(iv2, iv3, iv8);
    Point3 v6 = dual(iv1, iv2, iv8);
    Point3 v7 = dual(iv0, iv2, iv1);
    Point3 v8 = dual(iv10, iv0, iv1);
    Point3 v9 = dual(iv10, iv1, iv9);
    Point3 v10 = dual(iv1, iv8, iv9);
    Point3 v11 = dual(iv3, iv4, iv8);
    Point3 v12 = dual(iv5, iv4, iv3);
    Point3 v13 = dual(iv11, iv4, iv5);
    Point3 v14 = dual(iv11, iv7, iv10);
    Point3 v15 = dual(iv7, iv0, iv10);
    Point3 v16 = dual(iv9, iv4, iv11);
    Point3 v17 = dual(iv9, iv8, iv4);
    Point3 v18 = dual(iv11, iv5, iv7);
    Point3 v19 = dual(iv11, iv10, iv9);

    // Form pentagonal faces
    auto dodecahedron = std::make_shared<TriSurface>();
    dodecahedron->add_polygon(form_pentagon(v0, v7, v8, v15, v4));
    dodecahedron->add_polygon(form_pentagon(v6, v10, v9, v8, v7));
    dodecahedron->add_polygon(form_pentagon(v5, v6, v7, v0, v1));
    dodecahedron->add_polygon(form_pentagon(v12, v11, v5, v1, v2));
    dodecahedron->add_polygon(form_pentagon(v13, v16, v17, v11, v12));
    dodecahedron->add_polygon(form_pentagon(v2, v3, v18, v13, v12));
    dodecahedron->add_polygon(form_pentagon(v1, v0, v4, v3, v2));
    dodecahedron->add_polygon(form_pentagon(v3, v4, v15, v14, v18));
    dodecahedron->add_polygon(form_pentagon(v11, v17, v10, v6, v5));
    dodecahedron->add_polygon(form_pentagon(v16, v19, v9, v10, v17));
    dodecahedron->add_polygon(form_pentagon(v15, v8, v9, v19, v14));
    dodecahedron->add_polygon(form_pentagon(v18, v14, v19, v16, v13));
    dodecahedron->create_vertex_buffers(position_loc, normal_loc);
    return dodecahedron;
}

std::vector<Point3> bucky_pentagon(const Point3 &v0,
                                   const Point3 &v1,
                                   const Point3 &v2,
                                   const Point3 &v3,
                                   const Point3 &v4,
                                   const Point3 &v5)
{
    std::vector<Point3> vlist;
    vlist.push_back(v0.affine_combination(0.666f, 0.334f, v1));
    vlist.push_back(v0.affine_combination(0.666f, 0.334f, v2));
    vlist.push_back(v0.affine_combination(0.666f, 0.334f, v3));
    vlist.push_back(v0.affine_combination(0.666f, 0.334f, v4));
    vlist.push_back(v0.affine_combination(0.666f, 0.334f, v5));
    vlist.push_back(v0.affine_combination(0.666f, 0.334f, v1));
    return vlist;
}

std::vector<Point3> divide_face(const Point3 &v0, const Point3 &v1, const Point3 &v2)
{
    std::vector<Point3> vlist;
    vlist.push_back(v0.affine_combination(0.666f, 0.334f, v1));
    vlist.push_back(v0.affine_combination(0.334f, 0.666f, v1));
    vlist.push_back(v1.affine_combination(0.666f, 0.334f, v2));
    vlist.push_back(v1.affine_combination(0.334f, 0.666f, v2));
    vlist.push_back(v2.affine_combination(0.666f, 0.334f, v0));
    vlist.push_back(v2.affine_combination(0.334f, 0.666f, v0));
    return vlist;
}

std::shared_ptr<TriSurface> construct_buckyball(int32_t position_loc, int32_t normal_loc)
{
    // Vertices of the icosahedron
    Point3 v0(0.0f, 1.0f, PHI_INV);
    Point3 v1(0.0f, 1.0f, -PHI_INV);
    Point3 v2(1.0f, PHI_INV, 0.0f);
    Point3 v3(1.0f, -PHI_INV, 0.0f);
    Point3 v4(0.0f, -1.0f, -PHI_INV);
    Point3 v5(0.0f, -1.0f, PHI_INV);
    Point3 v6(PHI_INV, 0.0f, 1.0f);
    Point3 v7(-PHI_INV, 0.0f, 1.0f);
    Point3 v8(PHI_INV, 0.0f, -1.0f);
    Point3 v9(-PHI_INV, 0.0f, -1.0f);
    Point3 v10(-1.0f, PHI_INV, 0.0f);
    Point3 v11(-1.0f, -PHI_INV, 0.0f);

    // Replace each triangular face with a hexagon
    auto buckyball = std::make_shared<TriSurface>();
    buckyball->add_polygon(divide_face(v0, v6, v2));
    buckyball->add_polygon(divide_face(v6, v3, v2));
    buckyball->add_polygon(divide_face(v6, v5, v3));
    buckyball->add_polygon(divide_face(v7, v5, v6));
    buckyball->add_polygon(divide_face(v7, v6, v0));
    buckyball->add_polygon(divide_face(v2, v3, v8));
    buckyball->add_polygon(divide_face(v1, v2, v8));
    buckyball->add_polygon(divide_face(v0, v2, v1));
    buckyball->add_polygon(divide_face(v10, v0, v1));
    buckyball->add_polygon(divide_face(v10, v1, v9));
    buckyball->add_polygon(divide_face(v1, v8, v9));
    buckyball->add_polygon(divide_face(v3, v4, v8));
    buckyball->add_polygon(divide_face(v5, v4, v3));
    buckyball->add_polygon(divide_face(v11, v4, v5));
    buckyball->add_polygon(divide_face(v11, v7, v10));
    buckyball->add_polygon(divide_face(v7, v0, v10));
    buckyball->add_polygon(divide_face(v9, v4, v11));
    buckyball->add_polygon(divide_face(v9, v8, v4));
    buckyball->add_polygon(divide_face(v11, v5, v7));
    buckyball->add_polygon(divide_face(v11, v10, v9));

    // Add pentagon surfaces
    buckyball->add_polygon(bucky_pentagon(v0, v6, v2, v1, v10, v7));
    buckyball->add_polygon(bucky_pentagon(v1, v2, v8, v9, v10, v0));
    buckyball->add_polygon(bucky_pentagon(v2, v3, v8, v1, v0, v6));
    buckyball->add_polygon(bucky_pentagon(v3, v5, v4, v8, v2, v6));
    buckyball->add_polygon(bucky_pentagon(v4, v3, v5, v11, v9, v8));
    buckyball->add_polygon(bucky_pentagon(v5, v6, v7, v11, v4, v3));
    buckyball->add_polygon(bucky_pentagon(v6, v3, v2, v0, v7, v5));
    buckyball->add_polygon(bucky_pentagon(v7, v11, v5, v6, v0, v10));
    buckyball->add_polygon(bucky_pentagon(v8, v4, v9, v1, v2, v3));
    buckyball->add_polygon(bucky_pentagon(v9, v8, v4, v11, v10, v1));
    buckyball->add_polygon(bucky_pentagon(v10, v0, v1, v9, v11, v7));
    buckyball->add_polygon(bucky_pentagon(v11, v5, v7, v10, v9, v4));
    buckyball->create_vertex_buffers(position_loc, normal_loc);
    return buckyball;
}

} // namespace cg
