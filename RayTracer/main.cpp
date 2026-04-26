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
#include "RayTracer/height_field_floor_node.hpp"
#include "RayTracer/normal_map_node.hpp"
#include "RayTracer/sand_bump_node.hpp"
#include "RayTracer/desert_rock_texture.hpp"
#include "RayTracer/ray_tracer.hpp"
#include "RayTracer/rt_sphere_node.hpp"
#include "RayTracer/rt_mesh_node.hpp"
#include "RayTracer/rt_transform_node.hpp"
#include "scene/bounding_aabb_node.hpp"
#include "scene/bezier_patch.hpp"
#include "scene/lod_node.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
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
int32_t     g_max_depth = 3;
const float DEPTH_THRESHOLD = 0.025f;

// Anti-aliasing mode: 0=off, 1=uniform 2x2, 2=adaptive
int g_aa_mode = 0;

// Adaptive AA sample statistics (reset each render)
static std::atomic<int64_t> g_aa_samples{0};
static std::atomic<int64_t> g_aa_pixels{0};

// Soft shadow toggle (area light disc sampling vs. hard binary shadows)
bool g_soft_shadows = true;

// Normal map toggle — 'm' key
bool g_normal_map_enabled = true;

// Sparkle toggle — 'k' key
bool g_sparkle_enabled = true;

// Atmospheric glow toggle — 'g' key
bool g_glow_enabled = true;

// Ray Tracer
cg::RayTracer *g_ray_tracer = 0;

// Lights (need to keep pointers to add to ray tracer)
std::vector<cg::LightNode *> g_lights;

// Scene construction
std::shared_ptr<cg::SceneNode> g_scene_root;

// ---------- Camera presets ----------
// Desert scene camera presets
struct CameraPreset { cg::Point3 pos; cg::Point3 target; };
static const CameraPreset PRESETS[] = {
    { cg::Point3(0.0f,  3.0f, -15.0f), cg::Point3(0.0f, 1.5f,  5.0f) }, // 1: wide shot
    { cg::Point3(3.0f,  0.2f,  -8.0f), cg::Point3(0.0f, 0.0f,  5.0f) }, // 2: ground level
    { cg::Point3(-3.0f, 1.5f,  -2.0f), cg::Point3(0.0f, 0.5f,  5.0f) }, // 3: close rock
    // Desert scene presets (restore when switching back):
    // { cg::Point3(0.0f,  3.0f, -15.0f), cg::Point3(0.0f, 1.5f,  5.0f) },
    // { cg::Point3(3.0f,  0.2f,  -8.0f), cg::Point3(0.0f, 0.0f,  5.0f) },
    // { cg::Point3(-5.0f, 1.5f,  -5.0f), cg::Point3(-3.5f,-1.0f,-3.0f) },
};

/**
 * Load an OBJ file via Assimp and return it as an RTMeshNode.
 * Returns nullptr and prints an error if the file is not found or fails to load.
 * @param flip_winding  Add aiProcess_FlipWindingOrder — use when the OBJ has CW faces
 *                      (e.g. Stanford bunny from Meshlab) so backface culling works correctly.
 */
static std::shared_ptr<cg::RTMeshNode> load_obj_mesh(const std::string &filename,
                                                      bool flip_winding = false)
{
    cg::FileInfo info = cg::locate_path_for_filename(filename);
    if (!info.found)
    {
        std::cerr << "load_obj_mesh: could not find '" << filename << "'\n";
        return nullptr;
    }

    // Load flags: triangulate quads, join duplicate vertices so smooth normals
    // can average across shared edges, then generate smooth normals for meshes
    // that don't have them (e.g. Stanford bunny).
    unsigned int flags = aiProcess_Triangulate |
                         aiProcess_JoinIdenticalVertices |
                         aiProcess_GenSmoothNormals;
    if (flip_winding)
        flags |= aiProcess_FlipWindingOrder;

    Assimp::Importer importer;
    const aiScene *ai = importer.ReadFile(info.file_path, flags);

    if (!ai || !ai->mNumMeshes)
    {
        std::cerr << "load_obj_mesh: failed to load '" << info.file_path << "': "
                  << importer.GetErrorString() << "\n";
        return nullptr;
    }

    std::cout << "load_obj_mesh: '" << filename << "' — "
              << ai->mNumMeshes << " submesh(es)\n";

    // Merge all submeshes into one vertex + face list.
    // Face indices are offset by the running vertex count so they remain valid.
    std::vector<cg::VertexAndNormal> vertices;
    std::vector<uint16_t>            faces;
    std::vector<cg::Point2>          uvs;
    std::vector<cg::Vector3>         tangents;
    bool has_uvs_and_tangents = true;

    for (uint32_t m = 0; m < ai->mNumMeshes; ++m)
    {
        const aiMesh *mesh = ai->mMeshes[m];
        const uint16_t vertex_offset = static_cast<uint16_t>(vertices.size());

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            cg::VertexAndNormal v;
            v.vertex = cg::Point3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            v.normal = cg::Vector3(0.0f, 0.0f, 0.0f);
            if (mesh->HasNormals())
                v.normal = cg::Vector3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            vertices.push_back(v);

            if (mesh->HasTextureCoords(0) && mesh->HasTangentsAndBitangents())
            {
                uvs.emplace_back(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                tangents.emplace_back(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            }
            else
            {
                has_uvs_and_tangents = false;
            }
        }

        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            const aiFace &f = mesh->mFaces[i];
            if (f.mNumIndices == 3)
            {
                faces.push_back(vertex_offset + static_cast<uint16_t>(f.mIndices[0]));
                faces.push_back(vertex_offset + static_cast<uint16_t>(f.mIndices[1]));
                faces.push_back(vertex_offset + static_cast<uint16_t>(f.mIndices[2]));
            }
        }
    }

    std::cout << "  total: " << vertices.size() << " vertices, "
              << faces.size() / 3 << " triangles\n";

    auto node = std::make_shared<cg::RTMeshNode>(vertices, faces);

    if (has_uvs_and_tangents && !uvs.empty())
        node->set_tangents_and_uvs(uvs, tangents);

    return node;
}

/**
 * Build an obelisk mesh — tapered rectangular shaft with a pyramidion cap.
 * Base centre at y=0; shaft tapers from bw×bd at base to tw×td at y=h;
 * pyramidion apex at y=ph. All faces wound CCW for outward normals.
 */
static std::shared_ptr<cg::RTMeshNode> make_obelisk()
{
    const float bw = 0.6f,  bd = 0.4f;  // base half-extents x, z
    const float tw = 0.18f, td = 0.12f; // shaft-top half-extents
    const float h  = 7.0f;              // shaft height
    const float ph = 8.5f;              // pyramidion apex height

    // Vertex layout
    // Base ring (y=0):   0=front-left  1=front-right  2=back-right  3=back-left
    // Top ring  (y=h):   4=front-left  5=front-right  6=back-right  7=back-left
    // Apex      (y=ph):  8
    std::vector<cg::Point3> verts = {
        {-bw, 0.0f, -bd}, { bw, 0.0f, -bd}, { bw, 0.0f,  bd}, {-bw, 0.0f,  bd},
        {-tw,    h, -td}, { tw,    h, -td}, { tw,    h,  td}, {-tw,    h,  td},
        { 0.0f, ph, 0.0f}
    };

    std::vector<uint16_t> faces = {
        // Shaft — each face: (BR, BL, TL), (BR, TL, TR) CCW from outside
        1, 0, 4,   1, 4, 5,  // front  (normal -Z)
        1, 5, 6,   1, 6, 2,  // right  (normal +X)
        2, 6, 7,   2, 7, 3,  // back   (normal +Z)
        3, 7, 4,   3, 4, 0,  // left   (normal -X)
        // Pyramidion — (right-base, left-base, apex) CCW from outside
        5, 4, 8,  // front
        6, 5, 8,  // right
        7, 6, 8,  // back
        4, 7, 8,  // left
    };

    return std::make_shared<cg::RTMeshNode>(verts, faces);
}

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
// #if 0  — desert scene restored
    // ==========================================================
    // Moonlit Desert Ruins
    // Camera: (0, 3, -15), looking toward (0, 1.5, 5)  [Y-up]
    //
    // Hierarchy:
    //   Sand floor (large sphere trick)
    //   Obelisk (Bezier patch mesh, BoundingSphere BV)
    //   AABBNode "NearRocks"  — rocks 1-3, z:-5..4,  LOD dist<30
    //   AABBNode "MidRocks"   — rocks 4-6, z: 5..14, LOD dist<32
    //   AABBNode "FarRocks"   — rocks 7-9, z:20..33, spheres only (always LOW)
    // ==========================================================
    auto scene_node = std::make_shared<cg::SceneNode>();

    // ---------- SAND FLOOR (large sphere trick) ----------
    // Warm sandy colour with low-medium specular.
    auto floor_mat = std::make_shared<cg::MaterialNode>();
    floor_mat->set_ambient_and_diffuse(cg::Color4(0.76f, 0.65f, 0.42f, 1.0f)); // sand
    floor_mat->set_specular(cg::Color4(0.07f, 0.06f, 0.04f, 1.0f));
    floor_mat->set_shininess(4.0f);
    floor_mat->set_emission(cg::Color4(0.01f, 0.02f, 0.05f, 1.0f)); // very slight blue moonlit glow
    // Moonlit quartz sparkle: ~3% of grains glint blue-white
    floor_mat->enable_sparkle(0.97f, 80.0f, cg::Color3(0.3f, 0.6f, 1.0f), 3.0f);
    // Height-field floor: ray-marches against the procedural dune surface so
    // dune geometry (silhouettes, cast shadows, parallax) is real, not faked.
    // height_scale=0.7 → dune crests reach ~1.0 wu above base_y=-1 (y≈0).
    auto floor_sphere = std::make_shared<cg::HeightFieldFloorNode>(-1.0f, 1.2f);
    // Micro-ripple bump shader layered on top of the geometric dune surface.
    auto floor_nm = std::make_shared<cg::SandBumpNode>(0.30f, 0.18f, 2.5f, 0.23f, 0.55f);
    floor_nm->add_child(floor_sphere);
    floor_mat->add_child(floor_nm);
    scene_node->add_child(floor_mat);

    // ---------- OBELISK ----------
    // Dark polished granite — high specular for "glistening" under moonlight.
    // Shaft base at y=-1 (sand surface), apex at y=7.5.
    // Neutral white diffuse — obelisk_tex provides the albedo colour.
    auto obelisk_mat = std::make_shared<cg::MaterialNode>();
    obelisk_mat->set_ambient_and_diffuse(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    obelisk_mat->set_specular(cg::Color4(0.8f, 0.85f, 0.9f, 1.0f));  // blue-tinted sheen
    obelisk_mat->set_shininess(180.0f);
    obelisk_mat->set_emission(cg::Color4(0.04f, 0.05f, 0.08f, 1.0f)); // faint blue-white moonlit glow

    // Dark polished granite — fine-grained fBm with near-black tones and subtle
    // blue-grey veining to match the moonlit scene palette.
    auto obelisk_tex = std::make_shared<cg::DesertRockTexture>();
    obelisk_tex->scale          = 0.8f;   // finer grain than rocks
    obelisk_tex->noise_scale    = 10.0f;
    obelisk_tex->stripes_scale  = 8.0f;
    obelisk_tex->texture_detail = 10.0f;
    obelisk_tex->color1         = cg::Color3(0.22f, 0.24f, 0.30f);  // dark blue-grey vein
    obelisk_tex->color2         = cg::Color3(0.05f, 0.06f, 0.08f);  // near-black granite
    obelisk_tex->stripes_color  = cg::Color3(0.02f, 0.02f, 0.04f);  // darkest band

    auto obelisk_xform = std::make_shared<cg::RTTransformNode>();
    obelisk_xform->translate(0.0f, -1.0f, 2.0f); // base on sand, slightly in front
    // Normal map gives the obelisk a subtle stone/granite texture.
    // Strength 0.4 keeps the smooth glistening feel while adding surface detail.
    auto obelisk_nm = std::make_shared<cg::NormalMapNode>("stone_normal.png", 0.4f);
    obelisk_nm->add_child(make_obelisk());
    obelisk_xform->add_child(obelisk_nm);

    // BV: world-space AABB covering obelisk base (-0.6,-1,1.6) to apex (0.6,7.5,2.4)
    auto obelisk_bv = std::make_shared<cg::AABBNode>();
    obelisk_bv->set_name("Obelisk");
    obelisk_bv->set(cg::Point3(-0.65f, -1.1f, 1.55f), cg::Point3(0.65f, 7.6f, 2.45f));
    obelisk_bv->add_child(obelisk_xform);
    obelisk_tex->add_child(obelisk_bv);
    obelisk_mat->add_child(obelisk_tex);
    scene_node->add_child(obelisk_mat);

    // ---------- ROCK MATERIAL (shared by all rock LOD levels) ----------
    // Neutral white diffuse — DesertRockTexture provides the albedo colour.
    auto rock_mat = std::make_shared<cg::MaterialNode>();
    rock_mat->set_ambient_and_diffuse(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    rock_mat->set_specular(cg::Color4(0.20f, 0.20f, 0.18f, 1.0f));
    rock_mat->set_shininess(24.0f);
    rock_mat->set_emission(cg::Color4(0.015f, 0.018f, 0.03f, 1.0f)); // dimmer moonlit glow

    // Helper lambda: wrap a rock mesh in a transform (scale + translate) under rock_mat.
    // burial > 0 sinks the rock below the sand surface for a "partially buried" look.
    // All levels render the full OBJ mesh; lod_threshold marks where a sphere stand-in
    // would have been used had we had a low-poly model.
    auto make_rock_lod = [&](const std::string &obj, float scale,
                              float tx, float tz, float burial,
                              float lod_threshold, float /*sphere_radius*/) {
        auto lod = std::make_shared<cg::LODNode>(obj);
        lod->set_position(cg::Point3(tx, -1.0f - burial, tz));

        auto mesh = load_obj_mesh(obj);
        if (mesh)
        {
            auto xform = std::make_shared<cg::RTTransformNode>();
            xform->translate(tx, -1.0f - burial, tz);
            xform->scale(scale, scale, scale);
            xform->add_child(mesh);

            // Within threshold: full mesh, no LOD switch needed
            lod->add_level(lod_threshold, xform, "high-poly mesh");
            // Beyond threshold: still full mesh — would have been a sphere stand-in
            lod->add_level(1e30f, xform, "would have LOD'd to sphere — rendering full mesh anyway");
        }

        return lod;
    };

    // Procedural rock texture — wraps all AABB groups so every rock
    // (near/mid/far, mesh and sphere LOD levels) uses the same albedo.
    auto rock_tex = std::make_shared<cg::DesertRockTexture>();

    // ---------- NEAR ROCKS (AABBNode: rocks 1-3, z:-5..4) ----------
    auto near_rocks = std::make_shared<cg::AABBNode>();
    near_rocks->set_name("NearRocks");
    near_rocks->set(cg::Point3(-9.0f, -2.0f, -6.0f), cg::Point3(7.0f, 2.0f, 5.0f));
    rock_tex->add_child(near_rocks);

    near_rocks->add_child(make_rock_lod("model/rock1.obj", 4.0f, -3.5f, -3.0f, 0.4f, 30.0f, 0.9f));
    near_rocks->add_child(make_rock_lod("model/rock2.obj", 3.0f,  4.5f, -1.0f, 0.2f, 30.0f, 0.7f));
    near_rocks->add_child(make_rock_lod("model/rock3.obj", 2.5f, -6.0f,  2.5f, 0.1f, 30.0f, 0.6f));

    // ---------- MID ROCKS (AABBNode: rocks 4-6, z:5..14) ----------
    auto mid_rocks = std::make_shared<cg::AABBNode>();
    mid_rocks->set_name("MidRocks");
    mid_rocks->set(cg::Point3(-7.0f, -2.0f, 4.0f), cg::Point3(9.0f, 2.0f, 15.0f));
    rock_tex->add_child(mid_rocks);

    mid_rocks->add_child(make_rock_lod("model/rock4.obj", 3.5f,  5.5f,  7.0f, 0.3f, 32.0f, 0.8f));
    mid_rocks->add_child(make_rock_lod("model/rock5.obj", 2.8f, -3.5f, 10.0f, 0.2f, 32.0f, 0.65f));
    mid_rocks->add_child(make_rock_lod("model/rock6.obj", 4.0f,  2.0f, 13.0f, 0.4f, 32.0f, 0.95f));

    // ---------- FAR ROCKS (AABBNode: rocks 7-9, z:20..33) ----------
    // Previously sphere-only; now rendered as full meshes with a lod_threshold=0
    // so the print always reports "would have LOD'd to sphere."
    auto far_rocks = std::make_shared<cg::AABBNode>();
    far_rocks->set_name("FarRocks");
    far_rocks->set(cg::Point3(-10.0f, -2.0f, 19.0f), cg::Point3(8.0f, 2.0f, 34.0f));
    rock_tex->add_child(far_rocks);

    far_rocks->add_child(make_rock_lod("model/rock7.obj", 4.0f, -7.0f, 22.0f, 0.4f,  0.0f, 1.2f));
    far_rocks->add_child(make_rock_lod("model/rock8.obj", 3.0f,  4.5f, 27.0f, 0.35f, 0.0f, 0.9f));
    far_rocks->add_child(make_rock_lod("model/rock9.obj", 5.0f, -1.5f, 32.0f, 0.75f, 0.0f, 1.5f));

    rock_mat->add_child(rock_tex);
    scene_node->add_child(rock_mat);

    // ---------- MOON LIGHT ----------
    // Low-angle directional light from the upper-left — maximises shadow length.
    // Blue-white colour evokes cool moonlight. w=0 makes it directional.
    auto moon = std::make_shared<cg::LightNode>(0);
    moon->set_position(cg::HPoint3(5.0f, 3.0f, 11.0f, 0.0f)); // directional — upper-left sky, verified in-frame all presets
    moon->set_diffuse(cg::Color4(1.40f, 1.70f, 2.0f, 1.0f));    // boosted for broad fill
    moon->set_specular(cg::Color4(0.80f, 0.90f, 1.0f, 1.0f));
    moon->set_ambient(cg::Color4(0.08f, 0.12f, 0.25f, 1.0f));   // lifted night-sky fill
    moon->enable();
    scene_node->add_child(moon);
    g_lights.push_back(moon.get());

    return scene_node;
// #endif  — desert scene end

}

// Returns perceived luminance of a color
static float luminance(const cg::Color3 &c)
{
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

// Convenience: trace one ray and accumulate the sample counter.
static inline cg::Color3 trace_one(float px, float py)
{
    cg::Ray3 r = g_camera->construct_ray(px, py);
    g_aa_samples += 1;
    return g_ray_tracer->trace_ray(r, g_max_depth, DEPTH_THRESHOLD);
}

// Recursive adaptive sampling for one pixel sub-region.
// (x, y)   — pixel-space top-left of the sub-region
// size      — width/height of the sub-region in pixel units (1.0 = full pixel)
// depth     — current recursion depth (0 = first call per pixel)
// max_depth — stop subdividing at this depth (2 → up to 4^2 = 16 leaf samples)
// threshold — luminance contrast threshold above which we subdivide (~0.1)
// c00..c11  — pre-computed corner colors (passed from parent to avoid re-tracing)
//             c00=(x,y)  c10=(x+size,y)  c01=(x,y+size)  c11=(x+size,y+size)
//
// Corner reuse analysis (max_depth=2, fully subdivided):
//   old: 4 + 4×4 + 16×4 = 84 rays/pixel
//   new: 4 + 5  + 4×5   = 29 rays/pixel  (−65%)
static cg::Color3 adaptive_sample(float x, float y, float size,
                                  int depth, int max_depth, float threshold,
                                  cg::Color3 c00, cg::Color3 c10,
                                  cg::Color3 c01, cg::Color3 c11)
{
    // Contrast check on the four pre-computed corners — no new rays needed here.
    float lmin = luminance(c00), lmax = lmin;
    auto consider = [&](const cg::Color3 &c) {
        float l = luminance(c);
        if (l < lmin) lmin = l;
        if (l > lmax) lmax = l;
    };
    consider(c10); consider(c01); consider(c11);

    if (depth >= max_depth || (lmax - lmin) < threshold)
    {
        return cg::Color3((c00.r + c10.r + c01.r + c11.r) * 0.25f,
                          (c00.g + c10.g + c01.g + c11.g) * 0.25f,
                          (c00.b + c10.b + c01.b + c11.b) * 0.25f);
    }

    // High contrast — compute the 5 new interior samples and recurse into 4 quadrants.
    // Each quadrant inherits 1 parent corner + shares the center with its siblings,
    // so only 5 new rays are needed instead of 4×4 = 16 in the old approach.
    float half = size * 0.5f;
    float xm = x + half, ym = y + half, xr = x + size, yb = y + size;

    cg::Color3 cm0 = trace_one(xm, y );  // top edge midpoint
    cg::Color3 cm1 = trace_one(x,  ym);  // left edge midpoint
    cg::Color3 ccc = trace_one(xm, ym);  // centre
    cg::Color3 cm2 = trace_one(xr, ym);  // right edge midpoint
    cg::Color3 cm3 = trace_one(xm, yb);  // bottom edge midpoint

    // Quadrant corners:  TL=(x,y)  TR=(x+size,y)  BL=(x,y+size)  BR=(x+size,y+size)
    cg::Color3 q0 = adaptive_sample(x,  y,  half, depth+1, max_depth, threshold, c00, cm0, cm1, ccc);
    cg::Color3 q1 = adaptive_sample(xm, y,  half, depth+1, max_depth, threshold, cm0, c10, ccc, cm2);
    cg::Color3 q2 = adaptive_sample(x,  ym, half, depth+1, max_depth, threshold, cm1, ccc, c01, cm3);
    cg::Color3 q3 = adaptive_sample(xm, ym, half, depth+1, max_depth, threshold, ccc, cm2, cm3, c11);

    return cg::Color3((q0.r + q1.r + q2.r + q3.r) * 0.25f,
                      (q0.g + q1.g + q2.g + q3.g) * 0.25f,
                      (q0.b + q1.b + q2.b + q3.b) * 0.25f);
}

void render_rows(int32_t start_row, int32_t end_row, int32_t block_size)
{
    // Sub-pixel offsets for 2x2 supersampling (stratified within pixel)
    static const float offsets[4][2] = {
        {0.25f, 0.25f}, {0.75f, 0.25f},
        {0.25f, 0.75f}, {0.75f, 0.75f}
    };

    // Only supersample on the final pass (block_size == 1) and if AA is enabled.
    const bool supersample = (g_aa_mode == 1) && (block_size == 1);
    const bool adaptive    = (g_aa_mode == 2) && (block_size == 1);

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

                if(adaptive)
                {
                    // Adaptive super-sampling: trace 4 pixel corners once, then
                    // subdivide only high-contrast regions (corners are passed down
                    // so children never retrace them).
                    cg::Color3 c00 = trace_one(fx,        fy       );
                    cg::Color3 c10 = trace_one(fx + 1.0f, fy       );
                    cg::Color3 c01 = trace_one(fx,        fy + 1.0f);
                    cg::Color3 c11 = trace_one(fx + 1.0f, fy + 1.0f);
                    cg::Color3 color = adaptive_sample(fx, fy, 1.0f, 0, 2, 0.1f,
                                                       c00, c10, c01, c11);
                    g_frame_buffer->set(x, y, color, block_size);
                    ++g_aa_pixels;
                }
                else if(supersample)
                {
                    // Uniform 2x2: trace 4 rays per pixel at sub-pixel positions
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
                    // Single ray for preview passes or AA-off final pass
                    cg::Ray3 ray = g_camera->construct_ray(fx, fy);
                    cg::Color3 color = g_ray_tracer->trace_ray(ray, g_max_depth, DEPTH_THRESHOLD);
                    g_frame_buffer->set(x, y, color, block_size);
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistent thread pool
//
// Workers are created once and block on a condition variable between passes.
// The main thread submits row-range tasks, then calls wait() to block until
// all tasks finish before rendering the framebuffer and moving to the next pass.
// This eliminates the per-pass thread create/join overhead of the old design.
// ─────────────────────────────────────────────────────────────────────────────
class ThreadPool
{
public:
    explicit ThreadPool(int n)
    {
        workers_.reserve(n);
        for (int i = 0; i < n; ++i)
            workers_.emplace_back([this]{ worker_loop(); });
    }

    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            stop_ = true;
        }
        cv_work_.notify_all();
        for (auto &w : workers_) w.join();
    }

    // Submit one task.  May be called from any thread.
    void submit(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            queue_.push(std::move(task));
            ++pending_;
        }
        cv_work_.notify_one();
    }

    // Block until every task submitted since the last wait() has finished.
    void wait()
    {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_done_.wait(lk, [this]{ return pending_ == 0; });
    }

    int size() const { return static_cast<int>(workers_.size()); }

private:
    void worker_loop()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(mutex_);
                cv_work_.wait(lk, [this]{ return !queue_.empty() || stop_; });
                if (stop_ && queue_.empty()) return;
                task = std::move(queue_.front());
                queue_.pop();
            }
            task();
            {
                std::lock_guard<std::mutex> lk(mutex_);
                if (--pending_ == 0)
                    cv_done_.notify_all();
            }
        }
    }

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> queue_;
    std::mutex                        mutex_;
    std::condition_variable           cv_work_;  // workers wait here for tasks
    std::condition_variable           cv_done_;  // main thread waits here for pass end
    int                               pending_ = 0;
    bool                              stop_    = false;
};

// Initialised once on the first render, lives until process exit.
static std::unique_ptr<ThreadPool> g_thread_pool;

#ifdef MULTITHREAD

/**
 * Display function, multi-threaded
 */
void display()
{
    // Initialise the pool once — reused across all subsequent renders.
    if (!g_thread_pool)
    {
        int32_t n = static_cast<int32_t>(std::thread::hardware_concurrency()) - 1;
        if (n < 1) n = 1;
        g_thread_pool = std::make_unique<ThreadPool>(n);
    }
    // Reset culling/LOD print throttles so each render shows fresh output
    cg::AABBNode::reset_rt_print_count();
    cg::LODNode::reset_rt_print_count();

    // Reset adaptive AA statistics
    g_aa_samples.store(0);
    g_aa_pixels.store(0);

    // Clear the memory framebuffer
    g_frame_buffer->clear();

    for(int32_t block_size = cg::FB_BLOCK_SIZE; block_size > 0; block_size /= 2)
    {
        const int32_t num_rows = g_image_height / block_size;

        // Submit one task per logical row so the pool self-balances: shadow-heavy
        // rows (mid-scene rocks) and cheap rows (background sky) get interleaved
        // across threads without any thread sitting idle while others finish late.
        for(int32_t row = 0; row < num_rows; ++row)
        {
            g_thread_pool->submit([=]{ render_rows(row, row, block_size); });
        }

        // Block until all tasks for this pass are done, then display.
        g_thread_pool->wait();
        g_frame_buffer->render();
        SDL_GL_SwapWindow(g_sdl_window);
    }

    // Print adaptive AA statistics after the final pass
    if (g_aa_mode == 2)
    {
        int64_t px = g_aa_pixels.load();
        int64_t sa = g_aa_samples.load();
        float avg = (px > 0) ? static_cast<float>(sa) / static_cast<float>(px) : 0.0f;
        std::cout << "Adaptive AA: " << px << " pixels, avg "
                  << avg << " samples/pixel\n";
    }

    // Post-process edge-smoothing pass (all modes)
    g_frame_buffer->anti_alias();
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

    // Post-process edge-smoothing pass (all modes)
    g_frame_buffer->anti_alias();
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

        // Cycle AA mode: 0=off → 1=uniform 2x2 → 2=adaptive → 0
        case SDLK_A:
        {
            g_aa_mode = (g_aa_mode + 1) % 3;
            const char *modes[] = { "OFF", "Uniform 2x2", "Adaptive" };
            std::cout << "AA mode: " << modes[g_aa_mode] << "\n";
            result = cg::EventType::REDRAW;
            break;
        }

        // Toggle soft/hard shadows
        case SDLK_S:
            g_soft_shadows = !g_soft_shadows;
            g_ray_tracer->set_soft_shadows(g_soft_shadows);
            std::cout << "Shadows: " << (g_soft_shadows ? "SOFT (8 rays)" : "HARD (1 ray)") << std::endl;
            result = cg::EventType::REDRAW;
            break;

        // Toggle normal mapping (sand ripples + stone granite detail)
        case SDLK_M:
            g_normal_map_enabled = !g_normal_map_enabled;
            std::cout << "Normal map: " << (g_normal_map_enabled ? "ON" : "OFF") << "\n";
            result = cg::EventType::REDRAW;
            break;

        // Toggle sparkle (moonlit quartz glinting on sand)
        case SDLK_K:
            g_sparkle_enabled = !g_sparkle_enabled;
            std::cout << "Sparkle: " << (g_sparkle_enabled ? "ON" : "OFF") << "\n";
            result = cg::EventType::REDRAW;
            break;

        // Toggle atmospheric glow (additive in-scattering — no extinction)
        case SDLK_G:
            g_glow_enabled = !g_glow_enabled;
            std::cout << "Atmospheric glow: " << (g_glow_enabled ? "ON" : "OFF") << "\n";
            result = cg::EventType::REDRAW;
            break;

        // Camera presets: 1=wide, 2=ground level, 3=close rock
        case SDLK_1: case SDLK_2: case SDLK_3:
        {
            int idx = (event.key.key - SDLK_1);
            const CameraPreset &p = PRESETS[idx];
            g_camera->set_position_and_look_at_pt(p.pos, p.target);
            g_camera->set_view_up(cg::Vector3(0.0f, 1.0f, 0.0f));
            g_ray_tracer->set_view_position(p.pos);
            std::cout << "Camera preset " << idx + 1 << "\n";
            result = cg::EventType::REDRAW;
            break;
        }

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
    std::cout << "Moonlit Desert Ruins — Ray Tracer\n";
    std::cout << "r,R - Roll camera\n";
    std::cout << "p,P - Pitch camera\n";
    std::cout << "h,H - Heading camera\n";
    std::cout << "a   - Toggle anti-aliasing\n";
    std::cout << "s   - Toggle soft/hard shadows\n";
    std::cout << "m   - Toggle normal mapping (sand ripples + stone detail)\n";
    std::cout << "k   - Toggle sparkle (moonlit quartz glinting)\n";
    std::cout << "1   - Wide shot (obelisk + shadow sweep)\n";
    std::cout << "2   - Ground level (sand sparkle + shadow drama)\n";
    std::cout << "3   - Close rock (foreground detail)\n";

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

    // Initialize camera — preset 1 (wide shot)
    const CameraPreset &init = PRESETS[0];
    g_camera->set_position_and_look_at_pt(init.pos, init.target);
    g_camera->set_view_up(cg::Vector3(0.0f, 1.0f, 0.0f));
    g_camera->set_view_volume(
        g_image_width, g_image_height, g_field_of_view, g_near_plane_distance, g_far_plane_distance);

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
