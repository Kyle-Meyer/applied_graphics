//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    PolygonMesh/main.cpp
//	Purpose: OpenGL program to draw polygon mesh objects.
//
//============================================================================

#include "common/event.hpp"
#include "common/logging.hpp"
#include "filesystem_support/file_locator.hpp"
#include "geometry/geometry.hpp"
#include "scene/graphics.hpp"
#include "scene/scene.hpp"

// Local objects
#include "PolygonMesh/bilinear_patch.hpp"
#include "PolygonMesh/geodesic_dome.hpp"
#include "PolygonMesh/lighting_shader_node.hpp"
#include "PolygonMesh/object_creation.hpp"
#include "PolygonMesh/shader_src.hpp"
#include "PolygonMesh/unit_subdivided_sphere.hpp"
#include "PolygonMesh/unit_trough.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>

namespace cg
{

// Simple "selector" node to allow selection of one node from an array
// We may want to make this more general later!

template <typename T>
class NodeSelectorT : public SceneNode
{
  public:
    NodeSelectorT(const std::vector<std::shared_ptr<T>> &nodes)
    {
        m_nodes = nodes;
        m_current = 4;
    }

    void draw(SceneState &scene_state) override { m_nodes[m_current]->draw(scene_state); }

    void set_current(const uint16_t current)
    {
        if(current < m_nodes.size()) m_current = current;
    }

  protected:
    uint32_t                        m_current;
    std::vector<std::shared_ptr<T>> m_nodes;
};

using NodeSelector = NodeSelectorT<SceneNode>;
using MaterialSelector = NodeSelectorT<PresentationNode>;

} // namespace cg

namespace
{
template <typename T>
uint16_t e_val(T t)
{
    return static_cast<uint16_t>(t);
}

// Light types
enum class LightType
{
    FIXED_WORLD,
    MOVING_LIGHT,
    MINERS_LIGHT
};

enum class MatType
{
    BLACK_PLASTIC = 0,
    BRASS = 1,
    BRONZE = 2,
    CHROME = 3,
    COPPER = 4,
    GOLD = 5,
    PEWTER = 6,
    SILVER = 7,
    POLISHED_SILVER = 8,
    EARTH_TEXTURE = 9,
    COKE_TEXTURE = 10,
    WOOD_TEXTURE = 11
};

// Geometric objects - switched based on key commands
enum class ObjType
{
    CYLINDER = 0,
    CONE = 1,
    TRUNCATED_CONE = 2,
    TORUS = 3,
    CUBE = 4,
    ICOSAHEDRON = 5,
    OCTAHEDRON = 6,
    TETRAHEDRON = 7,
    DODECAHEDRON = 8,
    GEODESIC_DOME = 9,
    BILINEAR_PATCH = 10,
    REV_SURFACE = 11,
    BUCKYBALL = 12,
    SPHERE2 = 13,
    TEAPOT = 14,
    EARTH = 15,
    TROUGH = 16,
    A10 = 17,
    BUG = 18
};

} // namespace
// SDL Objects
SDL_Window       *g_sdl_window = nullptr;
SDL_GLContext     g_gl_context;
constexpr int32_t DRAWS_PER_SECOND = 30;
constexpr int32_t DRAW_INTERVAL_MILLIS =
    static_cast<int32_t>(1000.0 / static_cast<double>(DRAWS_PER_SECOND));

// Default lighting: fixed world coordinate
int32_t       g_light_rotation = 0;
cg::HPoint3   g_world_light_position;
cg::Matrix4x4 g_light_transform;
LightType     g_current_light = LightType::FIXED_WORLD;

// Global scene state
// SceneState MySceneState;

// While mouse button is down, the view will be updated
bool    g_animate_light = false;
bool    g_animate_camera = false;
bool    g_forward = true;
int32_t g_mouse_x, g_mouse_y;
int32_t g_render_width = 640;
int32_t g_render_height = 480;

// Scene graph elements
std::shared_ptr<cg::SceneNode>  g_scene_root; // Root of the scene graph
std::shared_ptr<cg::CameraNode> g_camera;     // Camera

std::shared_ptr<cg::TransformNode> g_object_rotation; // Transform node for rotating the objects
std::shared_ptr<cg::LightNode>     g_miners_light;    // View dependent light
std::shared_ptr<cg::LightNode>     g_world_light;     // World coordiante light (fixed or moving)
std::vector<std::shared_ptr<cg::PresentationNode>> g_mat_list(12); // Material Nodes
std::shared_ptr<cg::MaterialSelector>              g_mat_select;

// Set of defined object types
std::vector<std::shared_ptr<cg::SceneNode>> g_geo_list(19);
std::shared_ptr<cg::NodeSelector>           g_geo_select;

// Sleep function to help run a reasonable timer
void sleep(int32_t milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

/**
 * Reshape method.  Reset the perpective projection so the field of
 * view matches the window's aspect ratio.
 */
void reshape(int32_t width, int32_t height)
{
    g_render_width = width;
    g_render_height = height;

    // Reset the viewport
    glViewport(0, 0, width, height);

    // Reset the perspective projection to reflect the change of the aspect ratio
    g_camera->change_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
}

/**
 * Display function
 */
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the scene graph
    cg::SceneState scene_state;
    scene_state.init();
    g_scene_root->draw(scene_state);

    // Swap buffers
    SDL_GL_SwapWindow(g_sdl_window);
}

void enable_texture_filtering()
{
    // Turn on filtering for each texture material
    for(uint32_t i = 0; i < g_mat_list.size(); i++)
        g_mat_list[i]->update_texture_filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
}

void disable_texture_filtering()
{
    // Turn on filtering for each texture material
    for(uint32_t i = 0; i < g_mat_list.size(); i++)
        g_mat_list[i]->update_texture_filters(GL_NEAREST, GL_NEAREST);
}

// Update spotlight given camera position and orientation
void update_spotlight()
{
    // Update spotlight/miners light
    cg::Point3 pos = g_camera->get_position();
    g_miners_light->set_position(cg::HPoint3(pos.x, pos.y, pos.z, 1.0f));
    cg::Vector3 dir = g_camera->get_view_plane_normal() * -1.0f;
    g_miners_light->set_spotlight(dir, 64.0f, 45.0f);
}

/**
 * Updates the view given the mouse position and whether to move
 * forard or backward
 */
void update_view(const int x, const int y, bool forward)
{
    const float step = 1.0;
    float       dx = 4.0f * ((x - (static_cast<float>(g_render_width) * 0.5f)) /
                       static_cast<float>(g_render_width));
    float       dy = 4.0f * (((static_cast<float>(g_render_height) * 0.5f) - y) /
                       static_cast<float>(g_render_height));
    float       dz = (forward) ? step : -step;
    g_camera->move_and_turn(dx * step, dy * step, dz);
    update_spotlight();
}

/**
 * Method to update the moving light or update the camera position.
 */
void animate_view()
{
    if(g_animate_light)
    {
        g_world_light_position = g_light_transform * g_world_light_position;
        g_world_light->set_position(g_world_light_position);
        g_light_rotation += 5;
        if(g_light_rotation > 360) g_light_rotation = 0;
    }

    // If mouse button is down, generate another view
    if(g_animate_camera) { update_view(g_mouse_x, g_mouse_y, g_forward); }
}

/**
 * Build the material list.
 */
void build_materials()
{
    // Material values taken from FS Hill. Note that ambient reflection is
    // lower than diffuse.

    // Material 0: Black plastic
    auto black_plastic = std::make_shared<cg::PresentationNode>();
    black_plastic->set_material_ambient(cg::Color4(0.0f, 0.0f, 0.0f));
    black_plastic->set_material_diffuse(cg::Color4(0.01f, 0.01f, 0.01f));
    black_plastic->set_material_specular(cg::Color4(0.5f, 0.5f, 0.5f));
    black_plastic->set_material_shininess(32.0f);
    g_mat_list[e_val(MatType::BLACK_PLASTIC)] = black_plastic;

    // Material 1: Brass
    auto brass = std::make_shared<cg::PresentationNode>();
    brass->set_material_ambient(cg::Color4(0.329412f, 0.223529f, 0.027451f));
    brass->set_material_diffuse(cg::Color4(0.780392f, 0.568627f, 0.113725f));
    brass->set_material_specular(cg::Color4(0.992157f, 0.941176f, 0.807843f));
    brass->set_material_shininess(27.8974f);
    g_mat_list[e_val(MatType::BRASS)] = brass;

    // Material 2: Bronze
    auto bronze = std::make_shared<cg::PresentationNode>();
    bronze->set_material_ambient(cg::Color4(0.2125f, 0.1275f, 0.054f));
    bronze->set_material_diffuse(cg::Color4(0.714f, 0.4284f, 0.18144f));
    bronze->set_material_specular(cg::Color4(0.393548f, 0.271906f, 0.166721f));
    bronze->set_material_shininess(25.6f);
    g_mat_list[e_val(MatType::BRONZE)] = bronze;

    // Material 3: Chrome
    auto chrome = std::make_shared<cg::PresentationNode>();
    chrome->set_material_ambient(cg::Color4(0.25f, 0.25f, 0.25f));
    chrome->set_material_diffuse(cg::Color4(0.4f, 0.4f, 0.4f));
    chrome->set_material_specular(cg::Color4(0.774597f, 0.774597f, 0.774597f));
    chrome->set_material_shininess(76.8f);
    g_mat_list[e_val(MatType::CHROME)] = chrome;

    // Material 4: Copper
    auto copper = std::make_shared<cg::PresentationNode>();
    copper->set_material_ambient(cg::Color4(0.19125f, 0.0735f, 0.0225f));
    copper->set_material_diffuse(cg::Color4(0.7038f, 0.27048f, 0.0828f));
    copper->set_material_specular(cg::Color4(0.256777f, 0.137622f, 0.086014f));
    copper->set_material_shininess(12.8f);
    g_mat_list[e_val(MatType::COPPER)] = copper;

    // Material 5: Gold
    auto gold = std::make_shared<cg::PresentationNode>();
    gold->set_material_ambient(cg::Color4(0.24725f, 0.1995f, 0.0745f));
    gold->set_material_diffuse(cg::Color4(0.75164f, 0.60648f, 0.22648f));
    gold->set_material_specular(cg::Color4(0.628281f, 0.555802f, 0.366065f));
    gold->set_material_shininess(51.2f);
    g_mat_list[e_val(MatType::GOLD)] = gold;

    // Material 6: Pewter
    auto pewter = std::make_shared<cg::PresentationNode>();
    pewter->set_material_ambient(cg::Color4(0.10588f, 0.058824f, 0.113725f));
    pewter->set_material_diffuse(cg::Color4(0.427451f, 0.470588f, 0.541176f));
    pewter->set_material_specular(cg::Color4(0.3333f, 0.3333f, 0.521569f));
    pewter->set_material_shininess(9.84615f);
    g_mat_list[e_val(MatType::PEWTER)] = pewter;

    // Material 7: Silver
    auto silver = std::make_shared<cg::PresentationNode>();
    silver->set_material_ambient(cg::Color4(0.19225f, 0.19225f, 0.19225f));
    silver->set_material_diffuse(cg::Color4(0.50754f, 0.50754f, 0.50754f));
    silver->set_material_specular(cg::Color4(0.508273f, 0.508273f, 0.508273f));
    silver->set_material_shininess(51.2f);
    g_mat_list[e_val(MatType::SILVER)] = silver;

    // Material 8: Polished Silver
    auto polished_silver = std::make_shared<cg::PresentationNode>();
    polished_silver->set_material_ambient(cg::Color4(0.23125f, 0.23125f, 0.23125f));
    polished_silver->set_material_diffuse(cg::Color4(0.2775f, 0.2775f, 0.2775f));
    polished_silver->set_material_specular(cg::Color4(0.773911f, 0.773911f, 0.773911f));
    polished_silver->set_material_shininess(89.6f);
    g_mat_list[e_val(MatType::POLISHED_SILVER)] = polished_silver;

    // Material 9: earth texture
    auto earth_texture = std::make_shared<cg::PresentationNode>();
    earth_texture->set_material_ambient(cg::Color4(0.2f, 0.2f, 0.2f));
    earth_texture->set_material_diffuse(cg::Color4(0.5f, 0.5f, 0.5f));
    earth_texture->set_material_specular(cg::Color4(0.6f, 0.6f, 0.6f));
    earth_texture->set_material_shininess(50);
    earth_texture->set_texture(
        "earthp2.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    g_mat_list[e_val(MatType::EARTH_TEXTURE)] = earth_texture;

    // Material 10 ('-'): coke texture
    auto coke_texture = std::make_shared<cg::PresentationNode>();
    coke_texture->set_material_ambient(cg::Color4(0.2f, 0.2f, 0.2f));
    coke_texture->set_material_diffuse(cg::Color4(0.5f, 0.5f, 0.5f));
    coke_texture->set_material_specular(cg::Color4(0.6f, 0.6f, 0.6f));
    coke_texture->set_material_shininess(50);
    coke_texture->set_texture(
        "cokecan.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    g_mat_list[e_val(MatType::COKE_TEXTURE)] = coke_texture;

    // Material 11 ('-'): grainy wood texture
    auto wood_texture = std::make_shared<cg::PresentationNode>();
    wood_texture->set_material_ambient(cg::Color4(0.2f, 0.2f, 0.2f));
    wood_texture->set_material_diffuse(cg::Color4(0.5f, 0.5f, 0.5f));
    wood_texture->set_material_specular(cg::Color4(0.6f, 0.6f, 0.6f));
    wood_texture->set_material_shininess(50);
    wood_texture->set_texture(
        "grainy_wood.jpg", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    g_mat_list[e_val(MatType::WOOD_TEXTURE)] = wood_texture;

    g_mat_select = std::make_shared<cg::MaterialSelector>(g_mat_list);
    g_mat_select->set_current(e_val(MatType::GOLD));
}

/**
 * Build geometry list.
 */
void build_geometry(int32_t position_loc, int32_t normal_loc, int32_t texture_loc)
{
    // Create a unit circle as a TriSurface (for caps on conic surfaces)
    std::vector<cg::Point3> v_list;
    v_list.push_back(cg::Point3(0.0f, 0.0f, 0.0f)); // Center
    float theta = 0.0f;
    float d_theta = (2.0f * cg::PI) / 36.0f;
    for(uint32_t i = 0; i <= 36; i++, theta += d_theta)
        v_list.push_back(cg::Point3(std::cos(theta), std::sin(theta), 0.0f));
    auto unitCircle = std::make_shared<cg::TriSurface>();
    unitCircle->add_polygon(v_list);
    unitCircle->create_vertex_buffers(position_loc, normal_loc);

    // Cylinder with texture coordinates (include scaling)
    auto cylinder_transform = std::make_shared<cg::TransformNode>();
    cylinder_transform->scale(20.0f, 20.0f, 60.0f);
    auto cylinder = std::make_shared<cg::TexturedConicSurface>(
        1.0f, 1.0f, 36, 10, position_loc, normal_loc, texture_loc);
    cylinder_transform->add_child(cylinder);
    auto bottom_cap = std::make_shared<cg::TransformNode>();
    bottom_cap->translate(0.0f, 0.0f, -0.5f);
    bottom_cap->rotate_x(180.0f);
    bottom_cap->add_child(unitCircle);
    cylinder_transform->add_child(bottom_cap);
    auto top_cap = std::make_shared<cg::TransformNode>();
    top_cap->translate(0.0f, 0.0f, 0.5f);
    top_cap->add_child(unitCircle);
    cylinder_transform->add_child(top_cap);
    g_geo_list[e_val(ObjType::CYLINDER)] = cylinder_transform;

    // Cone (include scaling)
    auto cone_transform = std::make_shared<cg::TransformNode>();
    cone_transform->scale(20.0f, 20.0f, 40.0f);
    auto cone = std::make_shared<cg::TexturedConicSurface>(
        1.0f, 0.0f, 36, 10, position_loc, normal_loc, texture_loc);
    cone_transform->add_child(cone);
    auto cone_bottom_cap = std::make_shared<cg::TransformNode>();
    cone_bottom_cap->translate(0.0f, 0.0f, -0.5f);
    cone_bottom_cap->rotate_x(180.0f);
    cone_bottom_cap->add_child(unitCircle);
    cone_transform->add_child(cone_bottom_cap);
    g_geo_list[e_val(ObjType::CONE)] = cone_transform;

    // Truncated cone (include scaling)
    auto cone_2_transform = std::make_shared<cg::TransformNode>();
    cone_2_transform->scale(20.0f, 20.0f, 40.0f);
    auto truncatedCone = std::make_shared<cg::TexturedConicSurface>(
        1.0f, 0.5f, 36, 10, position_loc, normal_loc, texture_loc);
    cone_2_transform->add_child(truncatedCone);
    auto cone_2_bottom_cap = std::make_shared<cg::TransformNode>();
    cone_2_bottom_cap->translate(0.0f, 0.0f, -0.5f);
    cone_2_bottom_cap->rotate_x(180.0f);
    cone_2_bottom_cap->add_child(unitCircle);
    cone_2_transform->add_child(cone_2_bottom_cap);
    auto cone_2_top_cap = std::make_shared<cg::TransformNode>();
    cone_2_top_cap->translate(0.0f, 0.0f, 0.5f);
    cone_2_top_cap->scale(0.5f, 0.5f, 1.0f);
    cone_2_top_cap->add_child(unitCircle);
    cone_2_transform->add_child(cone_2_top_cap);
    g_geo_list[e_val(ObjType::TRUNCATED_CONE)] = cone_2_transform;

    // Torus (no object scaling needed)
    auto torus = std::make_shared<cg::TexturedTorusSurface>(
        30.0f, 10.0f, 36, 18, position_loc, normal_loc, texture_loc);
    g_geo_list[e_val(ObjType::TORUS)] = torus;

    // Cube
    auto cube = cg::construct_cube(4, position_loc, normal_loc, texture_loc);
    auto cube_transform = std::make_shared<cg::TransformNode>();
    cube_transform->scale(40.0f, 40.0f, 40.0f);
    cube_transform->add_child(cube);
    g_geo_list[e_val(ObjType::CUBE)] = cube_transform;

    // Icosahedron
    auto icosahedron = cg::construct_icosahedron(position_loc, normal_loc);
    auto ico_transform = std::make_shared<cg::TransformNode>();
    ico_transform->scale(30.0f, 30.0f, 30.0f);
    ico_transform->add_child(icosahedron);
    g_geo_list[e_val(ObjType::ICOSAHEDRON)] = ico_transform;

    // Octahedron
    auto octahedron = cg::construct_octahedron(position_loc, normal_loc);
    auto octa_transform = std::make_shared<cg::TransformNode>();
    octa_transform->scale(40.0f, 40.0f, 40.0f);
    octa_transform->add_child(octahedron);
    g_geo_list[e_val(ObjType::OCTAHEDRON)] = octa_transform;

    // Tetrahedron
    auto tetrahedron = cg::construct_tetrahedron(position_loc, normal_loc);
    auto tetra_transform = std::make_shared<cg::TransformNode>();
    tetra_transform->scale(20.0f, 20.0f, 20.0f);
    tetra_transform->add_child(tetrahedron);
    g_geo_list[e_val(ObjType::TETRAHEDRON)] = tetra_transform;

    // Dodecahedron
    auto dodecahedron = cg::construct_dodecahedron(position_loc, normal_loc);
    auto dodeca_transform = std::make_shared<cg::TransformNode>();
    dodeca_transform->scale(40.0f, 40.0f, 40.0f);
    dodeca_transform->add_child(dodecahedron);
    g_geo_list[e_val(ObjType::DODECAHEDRON)] = dodeca_transform;

    // Geodesic dome
    auto dome = std::make_shared<cg::GeodesicDome>(position_loc, normal_loc);
    auto dome_transform = std::make_shared<cg::TransformNode>();
    dome_transform->scale(30.0f, 30.0f, 30.0f);
    dome_transform->add_child(dome);
    g_geo_list[e_val(ObjType::GEODESIC_DOME)] = dome_transform;

    // Bilinear patch
    cg::Point3 p0(-10.0, 0.0, 0.0);
    cg::Point3 p1(10.0, 25.0, 0.0);
    cg::Point3 p2(-10.0, 25.0, 20.0);
    cg::Point3 p3(10.0, 0.0, 20.0);
    auto patch = std::make_shared<cg::BilinearPatch>(p0, p1, p2, p3, 10, position_loc, normal_loc);
    auto patch_transform = std::make_shared<cg::TransformNode>();
    patch_transform->scale(2.0f, 2.0f, 2.0f);
    patch_transform->add_child(patch);
    g_geo_list[e_val(ObjType::BILINEAR_PATCH)] = patch_transform;

    // Surface of revolution
    std::vector<cg::Point3> v;
    v.push_back(cg::Point3(0.0f, 0.0f, 0.0f));
    v.push_back(cg::Point3(0.8f, 0.0f, 0.0f));
    v.push_back(cg::Point3(0.9f, 0.0f, 0.2f));
    v.push_back(cg::Point3(0.63f, 0.0f, 0.48f));
    v.push_back(cg::Point3(0.77f, 0.0f, 0.53f));
    v.push_back(cg::Point3(0.83f, 0.0f, 0.64f));
    v.push_back(cg::Point3(0.55f, 0.0f, 0.9f));
    v.push_back(cg::Point3(0.3f, 0.0f, 1.6f));
    v.push_back(cg::Point3(0.23f, 0.0f, 2.3f));
    v.push_back(cg::Point3(0.5f, 0.0f, 2.52f));
    v.push_back(cg::Point3(0.4f, 0.0f, 2.62f));
    v.push_back(cg::Point3(0.6f, 0.0f, 2.74f));
    v.push_back(cg::Point3(0.62f, 0.0f, 2.92f));
    v.push_back(cg::Point3(0.51f, 0.0f, 3.06f));
    v.push_back(cg::Point3(0.3f, 0.0f, 3.28f));
    v.push_back(cg::Point3(0.43f, 0.0f, 3.5f));
    v.push_back(cg::Point3(0.43f, 0.0f, 3.72f));
    v.push_back(cg::Point3(0.33f, 0.0f, 3.95f));
    v.push_back(cg::Point3(0.18f, 0.0f, 4.10f));
    v.push_back(cg::Point3(0.0f, 0.0f, 4.15f));

    auto rev = std::make_shared<cg::TexturedSurfaceOfRevolution>(
        v, 36, position_loc, normal_loc, texture_loc);
    auto rev_transform = std::make_shared<cg::TransformNode>();
    rev_transform->scale(10.0f, 10.0f, 10.0f);
    rev_transform->add_child(rev);
    g_geo_list[e_val(ObjType::REV_SURFACE)] = rev_transform;

    // Buckyball
    auto buckyball = cg::construct_buckyball(position_loc, normal_loc);
    auto bucky_transform = std::make_shared<cg::TransformNode>();
    bucky_transform->scale(30.0f, 30.0f, 30.0f);
    bucky_transform->add_child(buckyball);
    g_geo_list[e_val(ObjType::BUCKYBALL)] = bucky_transform;

    // Subdivided sphere
    auto sphere_2 = std::make_shared<cg::UnitSubdividedSphere>(4, position_loc, normal_loc);
    auto sphere_2_transform = std::make_shared<cg::TransformNode>();
    sphere_2_transform->scale(30.0f, 30.0f, 30.0f);
    sphere_2_transform->add_child(sphere_2);
    g_geo_list[e_val(ObjType::SPHERE2)] = sphere_2_transform;

    // Mesh teapot
    auto teapot = std::make_shared<cg::MeshTeapot>(4, position_loc, normal_loc);
    auto teapot_transform = std::make_shared<cg::TransformNode>();
    teapot_transform->scale(8.0f, 8.0f, 8.0f);
    teapot_transform->add_child(teapot);
    g_geo_list[e_val(ObjType::TEAPOT)] = teapot_transform;

    // Sphere using stacks and slices. Enable texture coordinates.
    auto earth_sphere = std::make_shared<cg::TexturedSphereSection>(
        -90.0f, 90.0f, 36, -180.0f, 180.0f, 72, 1.0f, position_loc, normal_loc, texture_loc);
    auto sphere_transform = std::make_shared<cg::TransformNode>();
    sphere_transform->scale(30.0f, 30.0f, 30.0f);
    sphere_transform->add_child(earth_sphere);
    g_geo_list[e_val(ObjType::EARTH)] = sphere_transform;

    // Trough
    auto trough = std::make_shared<cg::UnitTrough>(18, 10, position_loc, normal_loc);
    auto trough_transform = std::make_shared<cg::TransformNode>();
    trough_transform->scale(10.0f, 10.0f, 50.0f);
    trough_transform->add_child(trough);
    g_geo_list[e_val(ObjType::TROUGH)] = trough_transform;

    // A10 Model
    auto model_node_1 =
        std::make_shared<cg::ModelNode>(position_loc, normal_loc, texture_loc, "A10/A10.3ds");
    auto model_pres_1 = std::make_shared<cg::PresentationNode>();
    model_pres_1->set_material_ambient_and_diffuse(cg::Color4(1.0f, 1.0f, 1.0f));
    model_pres_1->set_material_specular(cg::Color4(0.2f, 0.2f, 0.2f));
    model_pres_1->set_material_shininess(70.0f);
    auto model_transform_1 = std::make_shared<cg::TransformNode>();
    model_transform_1->scale(3.0f, 3.0f, 3.0f);
    model_transform_1->add_child(model_node_1);
    model_pres_1->add_child(model_transform_1);
    g_geo_list[e_val(ObjType::A10)] = model_pres_1;

    // Bug Model
    auto model_node_2 =
        std::make_shared<cg::ModelNode>(position_loc, normal_loc, texture_loc, "bug/bug.3ds");
    auto model_pres_2 = std::make_shared<cg::PresentationNode>();
    model_pres_2->set_material_ambient_and_diffuse(cg::Color4(1.0f, 1.0f, 1.0f));
    model_pres_2->set_material_specular(cg::Color4(0.2f, 0.2f, 0.2f));
    model_pres_2->set_material_shininess(70.0f);
    auto model_transform_2 = std::make_shared<cg::TransformNode>();
    model_transform_2->scale(30.0f, 30.0f, 30.0f);
    model_transform_2->add_child(model_node_2);
    model_pres_2->add_child(model_transform_2);
    g_geo_list[e_val(ObjType::BUG)] = model_pres_2;

    // Add geometry list to the selector
    g_geo_select = std::make_shared<cg::NodeSelector>(g_geo_list);
    g_geo_select->set_current(e_val(ObjType::CYLINDER));
}

/**
 * Construct scene including camera, lights, geometry nodes
 */
void construct_scene()
{
    // Construct the lighting shader node
    auto shader = std::make_shared<cg::LightingShaderNode>();
    // if(!shader->create_from_source(vertex_shader, fragment_shader) ||
    //    !shader->get_locations())
    if(!shader->create_from_source(pixel_lighting_tex_vert, pixel_lighting_tex_frag) ||
       !shader->get_locations())
        exit(-1);

    int32_t position_loc = shader->GetPositionLoc();
    int32_t normal_loc = shader->GetNormalLoc();
    int32_t texture_loc = shader->GetTextureLoc();

    // Initialize the view and set a perspective projection
    g_camera = std::make_shared<cg::CameraNode>();
    g_camera->set_position(cg::Point3(0.0f, -100.0f, 0.0f));
    g_camera->set_look_at_pt(cg::Point3(0.0f, 0.0f, 0.0f));
    g_camera->set_view_up(cg::Vector3(0.0, 0.0, 1.0));
    g_camera->set_perspective(50.0, 1.0, 1.0, 300.0);

    // Set world light default position and a light rotation matrix
    g_world_light_position = {50.0f, -50.0f, 50.f, 1.0f};
    g_light_transform.rotate(2.0f, 0.0f, 0.0f, 1.0f);

    // Set a white light - use for fixed and moving light
    // Positional light at 45 degree angle to the upper right
    // front corner
    g_world_light = std::make_shared<cg::LightNode>(0);
    g_world_light->set_diffuse(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    g_world_light->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    g_world_light->set_position(g_world_light_position);
    g_world_light->enable();

    // Light 1 - Miner's light - use a yellow spotlight
    g_miners_light = std::make_shared<cg::LightNode>(1);
    g_miners_light->set_diffuse(cg::Color4(1.0f, 1.0f, 0.0f, 1.0f));
    g_miners_light->set_specular(cg::Color4(1.0f, 1.0f, 0.0f, 1.0f));
    cg::Point3 pos = g_camera->get_position();
    g_miners_light->set_position(cg::HPoint3(pos.x, pos.y, pos.z, 1.0f));
    cg::Vector3 dir = g_camera->get_view_plane_normal() * -1.0f;
    g_miners_light->set_spotlight(dir, 64.0f, 45.0f);
    g_miners_light->disable();

    // Construct the object rotation TransformNode
    g_object_rotation = std::make_shared<cg::TransformNode>();

    // Build the material list and initialize the material selector
    build_materials();

    // Build the geometry list and initialize the material selector
    build_geometry(position_loc, normal_loc, texture_loc);

    // Construct scene graph
    g_scene_root = std::make_shared<cg::SceneNode>();
    g_scene_root->add_child(shader);
    shader->add_child(g_camera);
    g_camera->add_child(g_world_light);
    g_world_light->add_child(g_miners_light);
    g_miners_light->add_child(g_mat_select);

    // Set all material nodes to point to the object rotation (TransformNode)
    for(uint32_t i = 0; i < g_mat_list.size(); i++) g_mat_list[i]->add_child(g_object_rotation);

    g_object_rotation->add_child(g_geo_select);
}

/**
 * Window event handler.
 */
cg::EventType handle_window_event(const SDL_Event &event)
{
    cg::EventType result = cg::EventType::NONE;

    switch(event.type)
    {
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            reshape(event.window.data1, event.window.data2);
            result = cg::EventType::REDRAW;
            break;
        default: break;
    }

    return result;
}

/**
 * Mouse button handler (called when a mouse button state changes). Starts a
 * new draggable line when the left button is down. When left button up
 * the line is cleared.
 */
cg::EventType handle_mouse_event(const SDL_Event &event)
{
    cg::EventType result = cg::EventType::NONE;

    switch(event.button.button)
    {
        case SDL_BUTTON_LEFT:
            // On a left button up event MoveAndTurn the view with forward motion
            g_forward = true;
            if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                g_animate_camera = true;
                result = cg::EventType::REDRAW;
            }
            // Disable animation when the button is released
            else g_animate_camera = false;
            break;
        case SDL_BUTTON_RIGHT:
            // On a right button up event MoveAndTurn the view with reverse motion
            g_forward = false;
            if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                g_animate_camera = true;
                result = cg::EventType::REDRAW;
            }
            // Disable animation when the button is released
            else g_animate_camera = false;
            break;
    }

    return result;
}

/**
 * Mouse motion handler (called when mouse button is depressed)
 */
cg::EventType handle_mouse_motion_event(const SDL_Event &event)
{
    // Update position used for changing the view and force a new view
    g_mouse_x = event.motion.x;
    g_mouse_y = event.motion.y;

    if(g_animate_camera) return cg::EventType::REDRAW;
    else return cg::EventType::NONE;
}

/**
 * Keyboard event handler.
 */
cg::EventType handle_key_event(const SDL_Event &event)
{
    bool upper_case = (event.key.mod & SDL_KMOD_SHIFT) || (event.key.mod & SDL_KMOD_CAPS);

    switch(event.key.key)
    {
        case SDLK_ESCAPE: return cg::EventType::EXIT;

        case SDLK_0: g_mat_select->set_current(e_val(MatType::BLACK_PLASTIC)); break;
        case SDLK_1: g_mat_select->set_current(e_val(MatType::BRASS)); break;
        case SDLK_2: g_mat_select->set_current(e_val(MatType::BRONZE)); break;
        case SDLK_3: g_mat_select->set_current(e_val(MatType::CHROME)); break;
        case SDLK_4: g_mat_select->set_current(e_val(MatType::COPPER)); break;
        case SDLK_5: g_mat_select->set_current(e_val(MatType::GOLD)); break;
        case SDLK_6: g_mat_select->set_current(e_val(MatType::PEWTER)); break;
        case SDLK_7: g_mat_select->set_current(e_val(MatType::SILVER)); break;
        case SDLK_8: g_mat_select->set_current(e_val(MatType::POLISHED_SILVER)); break;
        case SDLK_9: g_mat_select->set_current(e_val(MatType::EARTH_TEXTURE)); break;
        case SDLK_MINUS: g_mat_select->set_current(e_val(MatType::COKE_TEXTURE)); break;
        case SDLK_EQUALS: g_mat_select->set_current(e_val(MatType::WOOD_TEXTURE)); break;

        // Roll the camera by +/-5 degrees (no need to update spotlight)
        case SDLK_R:
            if(upper_case) g_camera->roll(-5);
            else g_camera->roll(5);
            break;

        // Change the pitch of the camera by +/-5 degrees
        case SDLK_P:
            if(upper_case) g_camera->pitch(-5);
            else g_camera->pitch(5);
            update_spotlight();
            break;

        // Change the heading of the camera by +/-5 degrees
        case SDLK_H:
            if(upper_case) g_camera->heading(-5);
            else g_camera->heading(5);
            update_spotlight();
            break;

        // Cube
        case SDLK_C:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::CUBE));
            break;

        // Octahedron
        case SDLK_O:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::OCTAHEDRON));
            break;

        // Icosahedron
        // Reset the view
        case SDLK_I:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::ICOSAHEDRON));
            else
            {
                g_camera->set_position(cg::Point3(0.0f, -100.0f, 0.0f));
                g_camera->set_look_at_pt(cg::Point3(0.0f, 0.0f, 0.0f));
                g_camera->set_view_up(cg::Vector3(0.0, 0.0, 1.0));
                update_spotlight();
            }
            break;

        // Buckyball and update light
        case SDLK_B:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::BUCKYBALL));
            else
            {
                g_camera->set_position(cg::Point3(0.0f, 100.0f, 0.0f));
                g_camera->set_look_at_pt(cg::Point3(0.0f, 0.0f, 0.0f));
                g_camera->set_view_up(cg::Vector3(0.0, 0.0, 1.0));
                update_spotlight();
            }
            break;

        // Tetrahedron
        case SDLK_Y:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::TETRAHEDRON));
            break;

        // Dodecahedron
        case SDLK_D:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::DODECAHEDRON));
            break;

        // Earth
        case SDLK_E:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::EARTH));
            break;

        // A10 Model and enable_texture_filtering
        case SDLK_F:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::A10));
            else enable_texture_filtering();
            break;

        // Bug Model and Moving Light
        case SDLK_M:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::BUG));
            else
            {
                g_current_light = LightType::MOVING_LIGHT;
                g_animate_light = true;
                g_light_rotation = 0;
                g_world_light->enable();
                g_world_light_position = {50.0f, -50.0f, 50.f, 1.0f};
                g_world_light->set_position(g_world_light_position);
                g_miners_light->disable();
            }
            break;

        // Trough and Light at view direction
        case SDLK_V:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::TROUGH));
            else
            {
                g_current_light = LightType::MINERS_LIGHT;
                g_animate_light = false;
                g_miners_light->enable();
                g_world_light->disable();
            }
            break;

        // Bug Subdivided sphere
        case SDLK_A:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::SPHERE2));
            break;

        // Torus
        case SDLK_T:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::TORUS));
            break;

        // Cylinder
        case SDLK_L:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::CYLINDER));
            break;

        // Cone and disable_texture_filtering
        case SDLK_N:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::CONE));
            else disable_texture_filtering();
            break;

        // Teapot
        case SDLK_S:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::TEAPOT));
            break;

        // Truncated cone
        case SDLK_U:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::TRUNCATED_CONE));
            break;

        // Surface of Revolution
        case SDLK_Q:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::REV_SURFACE));
            break;

        // Bilinear Patch
        case SDLK_J:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::BILINEAR_PATCH));
            break;

        // Bilinear Patch
        case SDLK_G:
            if(upper_case) g_geo_select->set_current(e_val(ObjType::GEODESIC_DOME));
            break;

        // X rotation
        case SDLK_X:
            if(upper_case) g_object_rotation->rotate_x(-10.0f);
            else g_object_rotation->rotate_x(10.0f);
            break;

        // Z rotation
        case SDLK_Z:
            if(upper_case) g_object_rotation->rotate_z(-10.0f);
            else g_object_rotation->rotate_z(10.0f);
            break;

        // Change to fixed world light
        case SDLK_W:
            if(!upper_case)
            {
                g_current_light = LightType::FIXED_WORLD;
                g_animate_light = false;
                g_world_light->enable();
                g_world_light_position = {50.0f, -50.0f, 50.f, 1.0f};
                g_world_light->set_position(g_world_light_position);
                g_miners_light->disable();
            }
            break;

        default: break;
    }

    return cg::EventType::REDRAW;
}

/**
 * Handle Events function.
 */
cg::EventType handle_events()
{
    cg::EventType result = cg::EventType::NONE;
    SDL_Event     e;
    while(SDL_PollEvent(&e))
    {
        switch(e.type)
        {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: result |= cg::EventType::EXIT; break;

            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: result |= handle_window_event(e); break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: result |= handle_mouse_event(e); break;

            case SDL_EVENT_MOUSE_MOTION: result |= handle_mouse_motion_event(e); break;

            case SDL_EVENT_KEY_UP: break;
            case SDL_EVENT_KEY_DOWN: result |= handle_key_event(e); break;

            default: break;
        }
    }
    return result;
}

/**
 * Main
 */
int main(int argc, char **argv)
{
    cg::set_root_paths(argv[0]);
    cg::init_logging("PolygonMesh.log");

    // Print options
    std::cout << "Transforms:\n";
    std::cout << "x,X - rotate object about x axis\n";
    std::cout << "z,Z - rotate object about z axis\n";
    std::cout << "r,R - Change camera roll\n";
    std::cout << "p,P - Change camera pitch\n";
    std::cout << "h,H - Change camera heading\n";
    std::cout << "b   - View back of object\n";
    std::cout << "i   - Initialize view\n\n";
    std::cout << "Lighting:\n";
    std::cout << "m - Moving   w - Fixed world position  v - Miner's helmet\n";
    std::cout << "Materials:\n";
    std::cout << "0 - Black Plastic  1 - Brass   2 - Bronze\n";
    std::cout << "3 - Chrome         4 - Copper  5 - Gold\n";
    std::cout << "6 - Pewter         7 - Silver  8 - Polished Silver\n\n";
    std::cout << "9 - Earth texture  - - Coke Texture  =  - Wood Texture\n";
    std::cout << "f - Filtering      n - Nearest Texel\n";
    std::cout << "Objects:\n";
    std::cout << "C - Cube     G - Geodesic Sphere O - Octahedron\n";
    std::cout << "E - Earth    B - Buckyball       I - Icosahedron\n";
    std::cout << "T - Torus    Y - Tetrahedron     D - Dodecahedron\n";
    std::cout << "L - Cylinder N - Cone            U - Truncated Cylinder\n";
    std::cout << "S - Teapot   J - Bilinear Patch  Q - Surface of Revolution\n";
    std::cout << "A - Sphere2  F - A 10 Model      M - Bug Model\n";
    std::cout << "V - Trough\n";

    // Initialize SDL
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "Error initializing SDL: " << SDL_GetError() << '\n';
        exit(1);
    }

    // Initialize display mode and window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_PropertiesID props = SDL_CreateProperties();
    if(props == 0)
    {
        std::cout << "Error creating SDL Window Properties: " << SDL_GetError() << '\n';
        exit(1);
    }

    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Polygon Mesh Objects");
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 640);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 480);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 100);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 100);

    g_sdl_window = SDL_CreateWindowWithProperties(props);
    if(g_sdl_window == nullptr)
    {
        std::cout << "Error initializing SDL Window" << SDL_GetError() << '\n';
        exit(1);
    }

    // Initialize OpenGL
    g_gl_context = SDL_GL_CreateContext(g_sdl_window);

    std::cout << "OpenGL  " << glGetString(GL_VERSION) << ", GLSL "
              << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

#if BUILD_WINDOWS
    int32_t glew_init_result = glewInit();
    if(GLEW_OK != glew_init_result)
    {
        std::cout << "GLEW Error: " << glewGetErrorString(glew_init_result) << '\n';
        exit(EXIT_FAILURE);
    }
#endif

    // Set the clear color to white
    glClearColor(0.3f, 0.3f, 0.5f, 0.0f);

    // Enable the depth buffer
    glEnable(GL_DEPTH_TEST);

    // Enable back face polygon removal
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    // Enable multisample anti-aliasing
    glEnable(GL_MULTISAMPLE);

    // Construct the scene.
    construct_scene();

    reshape(640, 480);

    update_view(g_mouse_x, g_mouse_y, g_forward);

    display();

    // Main loop
    cg::EventType event_result = cg::EventType::NONE;
    while(true)
    {
        event_result = handle_events();

        if(event_result & cg::EventType::EXIT) break;

        if(g_animate_camera || g_animate_light)
        {
            animate_view();
            display();
        }
        else if(event_result & cg::EventType::REDRAW) { display(); }

        sleep(DRAW_INTERVAL_MILLIS);
    }

    // Destroy OpenGL Context, SDL Window and SDL
    SDL_GL_DestroyContext(g_gl_context);
    SDL_DestroyWindow(g_sdl_window);
    SDL_Quit();
    return 0;
}
