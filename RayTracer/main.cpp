//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    RayTracer/main.cpp
//	Purpose: OpenGL program to draw polygon mesh objects.
//
//============================================================================

#include "common/event.hpp"
#include "common/logging.hpp"
#include "filesystem_support/file_locator.hpp"
#include "geometry/geometry.hpp"
#include "scene/graphics.hpp"
#include "scene/scene.hpp"

#include "RayTracer/framebuffer.hpp"
#include "RayTracer/functional_texture.hpp"
#include "RayTracer/lighting.hpp"
#include "RayTracer/material_node.hpp"
#include "RayTracer/ray_tracer.hpp"
#include "RayTracer/rt_sphere_node.hpp"
#include "RayTracer/rt_mesh_node.hpp"
#include "RayTracer/rt_transform_node.hpp"
#include "scene/bounding_aabb_node.hpp"
#include "scene/bezier_patch.hpp"
#include "scene/bezier_curve.hpp"
#include "scene/lod_node.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#define MULTITHREAD

// SDL Objects
SDL_Window   *g_sdl_window = nullptr;
SDL_GLContext g_gl_context;

// Window width and height. Developement trick: for faster ray tracing keep
// these values small until final image is ready
int32_t g_image_width = 1024;
int32_t g_image_height = 768;

// Constants. Set up the view plane a distance of 1.0 from the camera.
// Set a field of view angle of 60 degrees.
float g_near_plane_distance = 1.0f;
float g_far_plane_distance = 300.0f;
float g_field_of_view = 60.0;

// Camera support. This will be part of the scene graph but keep a global pointer
// so we can maniplulate it with GLUT
std::shared_ptr<cg::CameraNode> g_camera;

// Framebuffer
std::unique_ptr<cg::Framebuffer> g_frame_buffer;

// Maximum depth to trace and adaptive threshold.
int32_t     g_max_depth = 8;
const float DEPTH_THRESHOLD = 0.025f;

// Anti-aliasing toggle (supersampling + post-process AA)
bool g_anti_aliasing = true;

// Ray Tracer
cg::RayTracer *g_ray_tracer = 0;

// Lights (need to keep pointers to add to ray tracer)
std::vector<cg::LightNode *> g_lights;

// Scene construction
std::shared_ptr<cg::SceneNode> g_scene_root;

// ---------- Bezier curve animation state ----------
// Control points for a cubic Bezier arc through the scene (Y-up, camera looks toward +Z)
static const cg::Point3 CURVE_P0(-1.5f,  0.5f,  0.0f);
static const cg::Point3 CURVE_P1(-0.5f,  2.0f,  3.0f);
static const cg::Point3 CURVE_P2( 0.5f,  2.0f,  6.0f);
static const cg::Point3 CURVE_P3( 1.5f,  0.5f,  9.0f);
constexpr int CURVE_STEPS = 60;

// BezierCurve drives the animated sphere via forward differencing
std::unique_ptr<cg::BezierCurve> g_bezier_curve;

// Pointer to the animated sphere so we can move it with set_center() each step
cg::RTSphereNode *g_anim_sphere = nullptr;

// Helper: build a unit pyramid mesh (4 triangular sides + base)
// Base at y=0, apex at y=1, footprint -0.5..0.5 in x/z
static std::shared_ptr<cg::RTMeshNode> make_pyramid()
{
    std::vector<cg::Point3> verts = {
        cg::Point3(-0.5f, 0.0f, -0.5f),   // 0: base front-left
        cg::Point3( 0.5f, 0.0f, -0.5f),   // 1: base front-right
        cg::Point3( 0.5f, 0.0f,  0.5f),   // 2: base back-right
        cg::Point3(-0.5f, 0.0f,  0.5f),   // 3: base back-left
        cg::Point3( 0.0f, 1.0f,  0.0f)    // 4: apex
    };
    std::vector<uint16_t> faces = {
        1, 0, 4,   // front side
        2, 1, 4,   // right side
        3, 2, 4,   // back side
        0, 3, 4,   // left side
        0, 1, 2,  0, 2, 3  // base
    };
    return std::make_shared<cg::RTMeshNode>(verts, faces);
}

// Helper: build a unit cube mesh (12 triangles, 6 faces)
static std::shared_ptr<cg::RTMeshNode> make_cube()
{
    std::vector<cg::Point3> verts = {
        cg::Point3(-0.5f, -0.5f, -0.5f),
        cg::Point3( 0.5f, -0.5f, -0.5f),
        cg::Point3( 0.5f,  0.5f, -0.5f),
        cg::Point3(-0.5f,  0.5f, -0.5f),
        cg::Point3(-0.5f, -0.5f,  0.5f),
        cg::Point3( 0.5f, -0.5f,  0.5f),
        cg::Point3( 0.5f,  0.5f,  0.5f),
        cg::Point3(-0.5f,  0.5f,  0.5f)
    };
    std::vector<uint16_t> faces = {
        0, 3, 2,  0, 2, 1,   // front  (normal -Z)
        5, 6, 7,  5, 7, 4,   // back   (normal +Z)
        0, 4, 7,  0, 7, 3,   // left   (normal -X)
        1, 2, 6,  1, 6, 5,   // right  (normal +X)
        3, 7, 6,  3, 6, 2,   // top    (normal +Y)
        0, 1, 5,  0, 5, 4    // bottom (normal -Y)
    };
    return std::make_shared<cg::RTMeshNode>(verts, faces);
}

/**
 * Build an RTMeshNode from a tessellated Bezier patch at the given subdivision level.
 * @param cp           4x4 control points (row-major: cp[i*4+j] = P(u_i, v_j))
 * @param subdivisions Number of quad divisions per parametric axis
 */
static std::shared_ptr<cg::RTMeshNode>
make_bezier_patch_mesh(const std::array<cg::Point3, 16> &cp, uint32_t subdivisions)
{
    std::vector<cg::VertexAndNormal> verts;
    std::vector<uint16_t>            faces;
    cg::BezierPatchSurface::tessellate(cp, subdivisions, verts, faces);
    return std::make_shared<cg::RTMeshNode>(verts, faces);
}

std::shared_ptr<cg::SceneNode> construct_scene(std::shared_ptr<cg::CameraNode> camera)
{
    // ==========================================================
    // Scene designed to demonstrate VIEW FRUSTUM CULLING and LOD
    // Camera: (0, 0.5, -10), looking toward +Z
    //
    // LOD demo:
    //   NearLOD at (0, 0, -2): dist ~8  -> HIGH detail (mesh pyramid, level 0)
    //   FarLOD  at (0, 0, 10): dist ~20 -> LOW detail  (sphere,      level 1)
    //
    // AABB culling demo:
    //   "VisibleGroup" (x=-3..3, y=-2..2, z=-4..12) - in camera view, rays HIT
    //   "OffscreenGroup" (x=9..13, y=-2..2, z=-4..5) - far right, rays MISS -> prints RT CULLED
    // ==========================================================
    auto scene_node = std::make_shared<cg::SceneNode>();

    // ---------- FLOOR (large sphere trick, checkerboard) ----------
    auto floor_mat = std::make_shared<cg::MaterialNode>();
    floor_mat->set_ambient_and_diffuse(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    floor_mat->set_specular(cg::Color4(0.3f, 0.3f, 0.3f, 1.0f));
    floor_mat->set_shininess(16.0f);
    auto floor_tex = cg::FunctionalTexture::checkerboard(
        cg::Color3(0.85f, 0.85f, 0.85f), cg::Color3(0.2f, 0.2f, 0.2f), 1.0f);
    // Radius 1000, center at y=-1001: surface at y=-1
    auto floor_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(0.0f, -1001.0f, 0.0f), 1000.0f);
    floor_tex->add_child(floor_sphere);
    floor_mat->add_child(floor_tex);
    scene_node->add_child(floor_mat);

    // ---------- VISIBLE GROUP AABB (center of scene) ----------
    // Wraps the LOD objects + a marble sphere.
    // Rays from the camera pointing into the scene will HIT this AABB.
    auto visible_aabb = std::make_shared<cg::AABBNode>();
    visible_aabb->set_name("VisibleGroup");
    visible_aabb->set(
        cg::Point3(-3.0f, -1.5f, -4.0f),
        cg::Point3( 3.0f,  2.0f, 12.0f));

    // --- LOD Node 1: NEAR object (dist ~8 from camera -> HIGH detail) ---
    // High detail: gold pyramid mesh | Low detail: orange sphere
    // LOD threshold = 11 units: dist 8 uses level 0 (HIGH)
    auto near_lod = std::make_shared<cg::LODNode>("NearLOD");
    near_lod->set_position(cg::Point3(0.0f, 0.0f, -2.0f));  // world position for RT distance

    // Level 0 (HIGH): scaled & colored pyramid mesh
    auto near_high_mat = std::make_shared<cg::MaterialNode>();
    near_high_mat->set_ambient_and_diffuse(cg::Color4(0.9f, 0.7f, 0.1f, 1.0f));  // gold
    near_high_mat->set_specular(cg::Color4(1.0f, 0.9f, 0.4f, 1.0f));
    near_high_mat->set_shininess(32.0f);
    auto near_high_xform = std::make_shared<cg::RTTransformNode>();
    near_high_xform->translate(0.0f, -1.0f, -2.0f);
    near_high_xform->rotate_y(20.0f);
    near_high_xform->scale(1.2f, 1.8f, 1.2f);
    near_high_xform->add_child(make_pyramid());
    near_high_mat->add_child(near_high_xform);

    // Level 1 (LOW): simple sphere (same position, radius 0.7)
    auto near_low_mat = std::make_shared<cg::MaterialNode>();
    near_low_mat->set_ambient_and_diffuse(cg::Color4(0.9f, 0.5f, 0.1f, 1.0f));  // orange
    near_low_mat->set_specular(cg::Color4(0.6f, 0.6f, 0.6f, 1.0f));
    near_low_mat->set_shininess(16.0f);
    auto near_low_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(0.0f, 0.0f, -2.0f), 0.7f);
    near_low_mat->add_child(near_low_sphere);

    near_lod->add_level(11.0f, near_high_mat);   // dist <= 11 -> pyramid (HIGH)
    near_lod->add_level(1e30f, near_low_mat);    // dist >  11 -> sphere  (LOW)
    visible_aabb->add_child(near_lod);

    // --- LOD Node 2: FAR object (dist ~20 from camera -> LOW detail) ---
    // High detail: cyan cube mesh | Low detail: blue sphere
    // LOD threshold = 11: dist 20 uses level 1 (LOW)
    auto far_lod = std::make_shared<cg::LODNode>("FarLOD");
    far_lod->set_position(cg::Point3(0.0f, 0.0f, 10.0f));  // world position for RT distance

    // Level 0 (HIGH): reflective cyan cube mesh
    auto far_high_mat = std::make_shared<cg::MaterialNode>();
    far_high_mat->set_ambient_and_diffuse(cg::Color4(0.1f, 0.6f, 0.7f, 1.0f));  // cyan
    far_high_mat->set_specular(cg::Color4(0.8f, 0.8f, 0.8f, 1.0f));
    far_high_mat->set_shininess(64.0f);
    far_high_mat->set_global_reflectivity(0.25f, 0.25f, 0.25f);
    auto far_high_xform = std::make_shared<cg::RTTransformNode>();
    far_high_xform->translate(0.0f, -0.5f, 10.0f);
    far_high_xform->rotate_y(35.0f);
    far_high_xform->scale(1.0f, 1.0f, 1.0f);
    far_high_xform->add_child(make_cube());
    far_high_mat->add_child(far_high_xform);

    // Level 1 (LOW): simple sphere
    auto far_low_mat = std::make_shared<cg::MaterialNode>();
    far_low_mat->set_ambient_and_diffuse(cg::Color4(0.2f, 0.3f, 0.9f, 1.0f));  // blue
    far_low_mat->set_specular(cg::Color4(0.5f, 0.5f, 0.5f, 1.0f));
    far_low_mat->set_shininess(16.0f);
    auto far_low_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(0.0f, 0.0f, 10.0f), 0.7f);
    far_low_mat->add_child(far_low_sphere);

    far_lod->add_level(11.0f, far_high_mat);   // dist <= 11 -> cube   (HIGH)
    far_lod->add_level(1e30f, far_low_mat);    // dist >  11 -> sphere (LOW)
    visible_aabb->add_child(far_lod);

    // --- Mirror sphere in the visible group (bonus reflective object) ---
    auto mirror_mat = std::make_shared<cg::MaterialNode>();
    mirror_mat->set_ambient_and_diffuse(cg::Color4(0.05f, 0.05f, 0.05f, 1.0f));
    mirror_mat->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    mirror_mat->set_shininess(128.0f);
    mirror_mat->set_global_reflectivity(0.9f, 0.9f, 0.9f);
    auto mirror_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(-1.8f, 0.0f, 3.0f), 0.6f);
    mirror_mat->add_child(mirror_sphere);
    visible_aabb->add_child(mirror_mat);

    scene_node->add_child(visible_aabb);

    // ---------- TEST CUBE (winding verification) ----------
    // Plain green cube placed at (2, -0.5, 2), clearly in view from camera (0, 0.5, -10).
    // Remove once winding is confirmed correct.
    auto test_cube_mat = std::make_shared<cg::MaterialNode>();
    test_cube_mat->set_ambient_and_diffuse(cg::Color4(0.1f, 0.8f, 0.2f, 1.0f));  // green
    test_cube_mat->set_specular(cg::Color4(0.6f, 0.6f, 0.6f, 1.0f));
    test_cube_mat->set_shininess(32.0f);
    auto test_cube_xform = std::make_shared<cg::RTTransformNode>();
    test_cube_xform->translate(2.0f, -0.5f, 2.0f);
    test_cube_xform->rotate_y(25.0f);
    test_cube_xform->scale(0.8f, 0.8f, 0.8f);
    test_cube_xform->add_child(make_cube());
    test_cube_mat->add_child(test_cube_xform);
    scene_node->add_child(test_cube_mat);

    // ---------- OFF-SCREEN GROUP AABB (far right, culling demo) ----------
    // This group sits well outside the camera's main view.
    // Most rays will miss its AABB -> "RT CULLED (AABB): OffscreenGroup" printed.
    auto offscreen_aabb = std::make_shared<cg::AABBNode>();
    offscreen_aabb->set_name("OffscreenGroup");
    offscreen_aabb->set(
        cg::Point3(9.0f, -1.5f, -4.0f),
        cg::Point3(13.0f, 2.0f,  5.0f));

    // Red sphere - rarely visible but inside the AABB
    auto red_mat = std::make_shared<cg::MaterialNode>();
    red_mat->set_ambient_and_diffuse(cg::Color4(0.8f, 0.15f, 0.15f, 1.0f));
    red_mat->set_specular(cg::Color4(0.6f, 0.6f, 0.6f, 1.0f));
    red_mat->set_shininess(32.0f);
    auto red_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(11.0f, 0.0f, 0.0f), 0.8f);
    red_mat->add_child(red_sphere);
    offscreen_aabb->add_child(red_mat);

    // Purple sphere beside the red one
    auto purple_mat = std::make_shared<cg::MaterialNode>();
    purple_mat->set_ambient_and_diffuse(cg::Color4(0.5f, 0.1f, 0.7f, 1.0f));
    purple_mat->set_specular(cg::Color4(0.4f, 0.4f, 0.4f, 1.0f));
    purple_mat->set_shininess(16.0f);
    auto purple_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(11.0f, 0.0f, 3.0f), 0.8f);
    purple_mat->add_child(purple_sphere);
    offscreen_aabb->add_child(purple_mat);

    scene_node->add_child(offscreen_aabb);

    // ---------- BEZIER PATCH (parametric surface with LOD) ----------
    // Control points orient the dome in +Y (upward) to match the scene's Y-up
    // convention.  u varies in Z, v varies in X, height is Y.
    // With this layout dS/du x dS/dv points in +Y so normals face the camera.
    //
    // LOD levels (distance from camera ~8.5 to world-centre (0, 0, 4)):
    //   HIGH (16 subdivs) – distance <= 11
    //   LOW  ( 4 subdivs) – distance >  11
    const std::array<cg::Point3, 16> patch_cp = {{
        // i=0 (u=0): z=-1.5
        {-1.5f, 0.0f, -1.5f}, {-0.5f, 0.0f, -1.5f}, { 0.5f, 0.0f, -1.5f}, { 1.5f, 0.0f, -1.5f},
        // i=1 (u=1/3): z=-0.5
        {-1.5f, 0.5f, -0.5f}, {-0.5f, 2.0f, -0.5f}, { 0.5f, 2.0f, -0.5f}, { 1.5f, 0.5f, -0.5f},
        // i=2 (u=2/3): z=+0.5
        {-1.5f, 0.5f,  0.5f}, {-0.5f, 2.0f,  0.5f}, { 0.5f, 2.0f,  0.5f}, { 1.5f, 0.5f,  0.5f},
        // i=3 (u=1): z=+1.5
        {-1.5f, 0.0f,  1.5f}, {-0.5f, 0.0f,  1.5f}, { 0.5f, 0.0f,  1.5f}, { 1.5f, 0.0f,  1.5f},
    }};

    // Teal material for the patch
    auto patch_mat = std::make_shared<cg::MaterialNode>();
    patch_mat->set_ambient_and_diffuse(cg::Color4(0.1f, 0.6f, 0.6f, 1.0f));
    patch_mat->set_specular(cg::Color4(0.7f, 0.9f, 0.9f, 1.0f));
    patch_mat->set_shininess(48.0f);

    // Place the patch to the right of the yellow pyramid (pyramid is at (0,-1,-2)).
    // Scale 0.7 keeps it a similar size to the pyramid.
    auto patch_xform_high = std::make_shared<cg::RTTransformNode>();
    patch_xform_high->translate(2.5f, -1.0f, -2.0f);
    patch_xform_high->scale(0.7f, 0.7f, 0.7f);
    patch_xform_high->add_child(make_bezier_patch_mesh(patch_cp, 16));

    auto patch_xform_low = std::make_shared<cg::RTTransformNode>();
    patch_xform_low->translate(2.5f, -1.0f, -2.0f);
    patch_xform_low->scale(0.7f, 0.7f, 0.7f);
    patch_xform_low->add_child(make_bezier_patch_mesh(patch_cp, 4));

    // Camera (0, 0.5, -10) to (2.5, -1, -2) ≈ 8.5 → HIGH detail
    auto patch_lod = std::make_shared<cg::LODNode>("BezierPatch");
    patch_lod->set_position(cg::Point3(2.5f, -1.0f, -2.0f));
    patch_lod->add_level(11.0f, patch_xform_high);   // HIGH when close
    patch_lod->add_level(1e30f, patch_xform_low);    // LOW  when far

    patch_mat->add_child(patch_lod);
    scene_node->add_child(patch_mat);

    // ---------- BEZIER CURVE ANIMATION (parametric curve, forward differencing) ----------
    // Small grey marker spheres at evenly-spaced t values visualise the path.
    // One bright orange sphere sits at the current curve position (advanced with 'N').
    //
    // The curve sweeps from close-right to far-left in world space:
    //   P0 (-1.5, 0.5, 0.0) → P1 (-0.5, 2.0, 3.0) → P2 (0.5, 2.0, 6.0) → P3 (1.5, 0.5, 9.0)
    // (screen-right = -X, so P0 appears on screen-right and P3 on screen-left)
    {
        // --- Marker spheres (7 samples: t = 0, 1/6 ... 1) ---
        auto marker_mat = std::make_shared<cg::MaterialNode>();
        marker_mat->set_ambient_and_diffuse(cg::Color4(0.6f, 0.7f, 0.8f, 1.0f));
        marker_mat->set_specular(cg::Color4(0.3f, 0.3f, 0.3f, 1.0f));
        marker_mat->set_shininess(8.0f);

        constexpr int MARKERS = 7;
        for(int i = 0; i < MARKERS; ++i)
        {
            float      t   = static_cast<float>(i) / static_cast<float>(MARKERS - 1);
            cg::Point3 pos = cg::BezierCurve::evaluate(t, CURVE_P0, CURVE_P1, CURVE_P2, CURVE_P3);
            marker_mat->add_child(
                std::make_shared<cg::RTSphereNode>(pos, 0.06f));
        }
        scene_node->add_child(marker_mat);

        // --- Animated sphere (starts at P0, moved with 'N') ---
        // RTSphereNode stores world-space coordinates directly — no transform wrapper needed.
        auto anim_mat = std::make_shared<cg::MaterialNode>();
        anim_mat->set_ambient_and_diffuse(cg::Color4(0.9f, 0.5f, 0.05f, 1.0f)); // orange
        anim_mat->set_specular(cg::Color4(1.0f, 0.8f, 0.4f, 1.0f));
        anim_mat->set_shininess(48.0f);

        cg::Point3 init_pos = g_bezier_curve->current_point();
        auto anim_sphere = std::make_shared<cg::RTSphereNode>(init_pos, 0.18f);
        anim_mat->add_child(anim_sphere);
        scene_node->add_child(anim_mat);

        // Expose sphere so handle_key_event can call set_center() each step
        g_anim_sphere = anim_sphere.get();
    }

    // ---------- LIGHT ----------
    auto light = std::make_shared<cg::LightNode>(0);
    light->set_position(cg::HPoint3(3.0f, 8.0f, -4.0f, 1.0f));   // high, slightly in front
    light->set_diffuse(cg::Color4(1.2f, 1.2f, 1.1f, 1.0f));
    light->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    light->enable();
    scene_node->add_child(light);
    g_lights.push_back(light.get());

    return scene_node;
}

void render_rows(int32_t start_row, int32_t end_row, int32_t block_size)
{
    // Sub-pixel offsets for 2x2 supersampling (stratified within pixel)
    static const float offsets[4][2] = {
        {0.25f, 0.25f}, {0.75f, 0.25f},
        {0.25f, 0.75f}, {0.75f, 0.75f}
    };

    // Only supersample on the final pass (block_size == 1) and if AA is enabled.
    const bool supersample = g_anti_aliasing && (block_size == 1);

    for(int32_t row = start_row; row <= end_row; ++row)
    {
        int32_t y = row * block_size;
        for(int32_t x = 0; x < g_image_width; x += block_size)
        {
            // Check if framebuffer has been set for this pixel
            if(!g_frame_buffer->set(x, y))
            {
                float fx = static_cast<float>(x);
                float fy = static_cast<float>(y);

                if(supersample)
                {
                    // Trace 4 rays per pixel at sub-pixel positions and average
                    float r = 0.0f, g = 0.0f, b = 0.0f;
                    for(int s = 0; s < 4; ++s)
                    {
                        cg::Ray3 ray = g_camera->construct_ray(
                            fx - 0.5f + offsets[s][0],
                            fy - 0.5f + offsets[s][1]);
                        cg::Color3 c = g_ray_tracer->trace_ray(ray, g_max_depth, DEPTH_THRESHOLD);
                        r += c.r; g += c.g; b += c.b;
                    }
                    cg::Color3 color(r * 0.25f, g * 0.25f, b * 0.25f);
                    g_frame_buffer->set(x, y, color, block_size);
                }
                else
                {
                    // Single ray for preview passes
                    cg::Ray3 ray = g_camera->construct_ray(fx, fy);
                    cg::Color3 color = g_ray_tracer->trace_ray(ray, g_max_depth, DEPTH_THRESHOLD);
                    g_frame_buffer->set(x, y, color, block_size);
                }
            }
        }
    }
}

#ifdef MULTITHREAD

/**
 * Display function, multi-threaded
 */
void display()
{
    int32_t num_threads = std::thread::hardware_concurrency() - 1;
    num_threads = num_threads > 0 ? num_threads : 1;

    // Reset culling/LOD print throttles so each render shows fresh output
    cg::AABBNode::reset_rt_print_count();
    cg::LODNode::reset_rt_print_count();

    // Clear the memory framebuffer
    g_frame_buffer->clear();

    for(int32_t block_size = cg::FB_BLOCK_SIZE; block_size > 0; block_size /= 2)
    {
        std::thread *threads = new std::thread[num_threads];
        int32_t      num_rows = g_image_height / block_size;
        int32_t      start_row = 0;

        for(int32_t t_i = 0; t_i < num_threads; ++t_i)
        {
            int32_t end_row = std::min((t_i + 1) * num_rows / num_threads, num_rows - 1);
            threads[t_i] = std::thread(render_rows, start_row, end_row, block_size);
            start_row = end_row + 1;
        }

        // Wait for threads to complete
        for(int32_t t_i = 0; t_i < num_threads; ++t_i) { threads[t_i].join(); }

        g_frame_buffer->render();
        SDL_GL_SwapWindow(g_sdl_window);
    }

    // Final scene - simple anti-aliasing (without shooting additional rays)
    if (g_anti_aliasing) g_frame_buffer->anti_alias();
    SDL_GL_SwapWindow(g_sdl_window);
}

#else

/**
 * Display function, single-threaded
 */
void display()
{
    // Reset culling/LOD print throttles so each render shows fresh output
    cg::AABBNode::reset_rt_print_count();
    cg::LODNode::reset_rt_print_count();

    // Clear the memory framebuffer
    g_frame_buffer->clear();

    for(int32_t block_size = cg::FB_BLOCK_SIZE; block_size > 0; block_size /= 2)
    {
        render_rows(0, g_image_height+1, block_size);
        
        g_frame_buffer->render();
        SDL_GL_SwapWindow(g_sdl_window);
    }

    // Final scene - simple anti-aliasing (without shooting additional rays)
    if (g_anti_aliasing) g_frame_buffer->anti_alias();
    SDL_GL_SwapWindow(g_sdl_window);
}

#endif

/**
 * Reshape method.
 */
void reshape(int32_t width, int32_t height)
{
    // Update the view colume
    g_camera->set_view_volume(
        width, height, g_field_of_view, g_near_plane_distance, g_far_plane_distance);
    g_image_width = width;
    g_image_height = height;

    // Create framebuffer (old pointer is automatically deleted)
    g_frame_buffer = std::make_unique<cg::Framebuffer>(width, height);

    glViewport(0, 0, width, height);
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
 * Keyboard event handler.
 */
cg::EventType handle_key_event(const SDL_Event &event)
{
    cg::EventType result = cg::EventType::NONE;

    bool upper_case = (event.key.mod & SDL_KMOD_SHIFT) || (event.key.mod & SDL_KMOD_CAPS);

    switch(event.key.key)
    {
        case SDLK_ESCAPE: result = cg::EventType::EXIT; break;

        // Roll the camera by +/-5 degrees (no need to update spotlight)
        case SDLK_R:
            if(upper_case) g_camera->roll(-5);
            else g_camera->roll(5);
            result = cg::EventType::REDRAW;
            break;

        // Change the pitch of the camera by +/-5 degrees
        case SDLK_P:
            if(upper_case) g_camera->pitch(-5);
            else g_camera->pitch(5);
            result = cg::EventType::REDRAW;
            break;

        // Change the heading of the camera by +/-5 degrees
        case SDLK_H:
            if(upper_case) g_camera->heading(-5);
            else g_camera->heading(5);
            result = cg::EventType::REDRAW;
            break;

        // Toggle anti-aliasing
        case SDLK_A:
            g_anti_aliasing = !g_anti_aliasing;
            std::cout << "Anti-aliasing: " << (g_anti_aliasing ? "ON" : "OFF") << std::endl;
            result = cg::EventType::REDRAW;
            break;

        // Advance Bezier curve one step (forward differencing) and re-render
        case SDLK_N:
            if(g_bezier_curve && g_anim_sphere)
            {
                g_bezier_curve->step();
                if(g_bezier_curve->is_done()) g_bezier_curve->reset();

                cg::Point3 pos = g_bezier_curve->current_point();
                g_anim_sphere->set_center(pos);

                std::cout << "Bezier step " << g_bezier_curve->total_steps()
                          << "  pos = (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
                result = cg::EventType::REDRAW;
            }
            break;

        default: break;
    }

    return result;
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

            case SDL_EVENT_KEY_UP: break;
            case SDL_EVENT_KEY_DOWN: result |= handle_key_event(e); break;

            default: break;
        }
    }
    return result;
}

// Sleep function to help run a reasonable timer
void sleep(int32_t milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

/**
 * Main
 */
int main(int argc, char **argv)
{
    cg::set_root_paths(argv[0]);
    cg::init_logging("RayTracer.log");

    // Print options
    std::cout << "Transforms:" << std::endl;
    std::cout << "r,R - Change camera roll\n";
    std::cout << "p,P - Change camera pitch\n";
    std::cout << "h,H - Change camera heading\n";
    std::cout << "a   - Toggle anti-aliasing\n";
    std::cout << "n   - Advance Bezier curve one step (forward differencing)\n";

    // Initialize SDL
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "Error initializing SDL: " << SDL_GetError() << '\n';
        exit(1);
    }

    // Initialize display mode and window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_PropertiesID props = SDL_CreateProperties();
    if(props == 0)
    {
        std::cout << "Error creating SDL Window Properties: " << SDL_GetError() << '\n';
        exit(1);
    }

    // Initialize display mode and window
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Ray Tracer Mesh Objects");
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, g_image_width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, g_image_height);
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
        std::cout << "GLEW Error: " << glewGetErrorString(glew_init_result) << std::endl;
        exit(EXIT_FAILURE);
    }
#endif

    g_frame_buffer = std::make_unique<cg::Framebuffer>(g_image_width, g_image_height);

    // 605.767 - Student to define. Set up camera parameters for initial view.
    g_camera = std::make_shared<cg::CameraNode>();

    // Initialize camera position and orientation
    // Camera at (0, 0.5, -10), looking toward the scene center.
    // NearLOD (~8 units away) -> HIGH detail; FarLOD (~20 units away) -> LOW detail.
    g_camera->set_position_and_look_at_pt(cg::Point3(0.0f, 0.5f, -10.0f), cg::Point3(0.0f, 0.0f, 3.0f));
    g_camera->set_view_up(cg::Vector3(0.0f, 1.0f, 0.0f));  // Y is up
    // Initialize view volume for ray tracing
    g_camera->set_view_volume(
        g_image_width, g_image_height, g_field_of_view, g_near_plane_distance, g_far_plane_distance);

    // Initialise Bezier curve (forward-difference state) before scene construction
    // so construct_scene() can read the initial position for the animated sphere.
    g_bezier_curve = std::make_unique<cg::BezierCurve>(
        CURVE_P0, CURVE_P1, CURVE_P2, CURVE_P3, CURVE_STEPS);

    // Construct the scene, pass in the camera node to add to the root node.
    auto scene_root = construct_scene(g_camera);

    // Construct ray tracer. Pass in the scene graph root node
    g_ray_tracer = new cg::RayTracer(scene_root);

    // Add lights to the ray tracer
    for(cg::LightNode *light : g_lights) { g_ray_tracer->add_light(light); }

    // Set view position for lighting calculations
    g_ray_tracer->set_view_position(g_camera->get_position());

    // Main loop
    cg::EventType event_result = cg::EventType::NONE;

    // Flush any pending events (e.g., window resize on startup) before initial render
    SDL_Event e;
    while (SDL_PollEvent(&e)) {}

    // Trigger initial render
    display();
    while(true)
    {
        event_result = handle_events();

        if(event_result & cg::EventType::EXIT) break;

        if(event_result & cg::EventType::REDRAW) display();
    }

    // Destroy OpenGL Context, SDL Window and SDL
    SDL_GL_DestroyContext(g_gl_context);
    SDL_DestroyWindow(g_sdl_window);
    SDL_Quit();
    return 0;
}
