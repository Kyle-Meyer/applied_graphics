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
#include "RayTracer/image_texture.hpp"
#include "RayTracer/lighting.hpp"
#include "RayTracer/material_node.hpp"
#include "RayTracer/ray_tracer.hpp"
#include "RayTracer/rt_sphere_node.hpp"
#include "RayTracer/rt_quad_node.hpp"
#include "RayTracer/rt_mesh_node.hpp"
#include "RayTracer/rt_transform_node.hpp"
#include "scene/bounding_aabb_node.hpp"

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
int32_t g_image_width = 1280;
int32_t g_image_height = 960;

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
int32_t     g_max_depth = 15;
const float DEPTH_THRESHOLD = 0.025f;

// Ray Tracer
cg::RayTracer *g_ray_tracer = 0;

// Lights (need to keep pointers to add to ray tracer)
std::vector<cg::LightNode *> g_lights;

// Scene construction
std::shared_ptr<cg::SceneNode> g_scene_root;

std::shared_ptr<cg::SceneNode> construct_scene(std::shared_ptr<cg::CameraNode> camera)
{
    // 605.767 - Student to define. Create a scene graph to describe your scene
    auto scene_node = std::make_shared<cg::SceneNode>();

    // ========== FRONT SPHERES (wrapped in AABB for culling) ==========
    // All 3 spheres are near z=0, clustered in x from -3.5 to 0.2
    auto front_spheres_aabb = std::make_shared<cg::AABBNode>();
    front_spheres_aabb->set(
        cg::Point3(-3.5f, -0.5f, -0.5f),   // min corner
        cg::Point3(0.2f, 0.5f, 0.5f)       // max corner
    );

    // Marble sphere (moved left to reveal mesh objects)
    auto material = std::make_shared<cg::MaterialNode>();
    material->set_ambient_and_diffuse(cg::Color4(0.9f, 0.9f, 0.9f, 1.0f));  // White base for marble texture
    material->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    material->set_shininess(64.0f);

    // Marble texture using Perlin noise turbulence
    auto marble_texture = cg::FunctionalTexture::marble(
        cg::Color3(0.95f, 0.95f, 0.95f),  // White base
        cg::Color3(0.3f, 0.08f, 0.08f),   // Dark red veins
        3.0f,                               // Scale (frequency)
        5.0f);                              // Turbulence strength

    auto sphere = std::make_shared<cg::RTSphereNode>(cg::Point3(-1.5f, 0.0f, 0.0f), 0.5f);
    marble_texture->add_child(sphere);
    material->add_child(marble_texture);
    front_spheres_aabb->add_child(material);

    // Add a floor using a very large sphere (appears nearly flat) with checkerboard texture
    auto floor_material = std::make_shared<cg::MaterialNode>();
    floor_material->set_ambient_and_diffuse(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));  // White base for texture
    floor_material->set_specular(cg::Color4(0.3f, 0.3f, 0.3f, 1.0f));
    floor_material->set_shininess(16.0f);

    // Procedural checkerboard texture using FunctionalTexture
    auto floor_texture = cg::FunctionalTexture::checkerboard(
        cg::Color3(0.9f, 0.9f, 0.9f),   // Light gray
        cg::Color3(0.2f, 0.2f, 0.2f),   // Dark gray
        1.0f);                           // Scale

    // Large sphere with center far below, surface at y = -1
    // Radius 1000, center at (0, -1001, 0) puts the top of the sphere at y = -1
    auto floor = std::make_shared<cg::RTSphereNode>(cg::Point3(0.0f, -1001.0f, 0.0f), 1000.0f);
    floor_texture->add_child(floor);
    floor_material->add_child(floor_texture);
    scene_node->add_child(floor_material);

    // Mirror ball (reflective, moved left)
    auto mirror_material = std::make_shared<cg::MaterialNode>();
    mirror_material->set_ambient_and_diffuse(cg::Color4(0.1f, 0.1f, 0.1f, 1.0f));
    mirror_material->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    mirror_material->set_shininess(128.0f);
    mirror_material->set_global_reflectivity(0.9f, 0.9f, 0.9f);
    auto mirror_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(-3.0f, 0.0f, 0.0f), 0.5f);
    mirror_material->add_child(std::static_pointer_cast<cg::SceneNode>(mirror_sphere));
    front_spheres_aabb->add_child(mirror_material);

    // Glass ball (transparent/refractive, moved left)
    auto glass_material = std::make_shared<cg::MaterialNode>();
    glass_material->set_ambient_and_diffuse(cg::Color4(0.1f, 0.1f, 0.15f, 1.0f));
    glass_material->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    glass_material->set_shininess(128.0f);
    glass_material->set_global_transmission(0.95f, 0.95f, 0.95f);
    glass_material->set_index_of_refraction(.83f);
    auto glass_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(-0.3f, 0.0f, 0.0f), 0.5f);
    glass_material->add_child(std::static_pointer_cast<cg::SceneNode>(glass_sphere));
    front_spheres_aabb->add_child(glass_material);

    // Add the front spheres bounding box to the scene
    scene_node->add_child(front_spheres_aabb);

    // Green sphere (behind glass) - added to mesh bounding box since it's within those bounds
    auto green_material = std::make_shared<cg::MaterialNode>();
    green_material->set_ambient_and_diffuse(cg::Color4(0.2f, 0.8f, 0.2f, 1.0f));
    green_material->set_specular(cg::Color4(0.5f, 0.5f, 0.5f, 1.0f));
    green_material->set_shininess(32.0f);
    auto green_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(0.0f, 0.0f, 3.0f), 0.4f);
    green_material->add_child(std::static_pointer_cast<cg::SceneNode>(green_sphere));

    // ========== PLANAR SURFACES (2 walls, wrapped in AABB) ==========
    auto walls_aabb = std::make_shared<cg::AABBNode>();
    walls_aabb->set(
        cg::Point3(-5.0f, -1.0f, -2.0f),   // min corner
        cg::Point3(5.0f, 4.0f, 8.0f)       // max corner
    );

    // Back wall with image texture
    auto back_wall_material = std::make_shared<cg::MaterialNode>();
    back_wall_material->set_ambient_and_diffuse(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));  // White base for texture
    back_wall_material->set_specular(cg::Color4(0.2f, 0.2f, 0.2f, 1.0f));
    back_wall_material->set_shininess(8.0f);

    // Image texture from textures/ directory
    auto wall_texture = std::make_shared<cg::ImageTexture>("floor_tiles.jpg");

    auto back_wall = std::make_shared<cg::RTQuadNode>(
        cg::Point3(5.0f, -1.0f, 8.0f),    // bottom-right
        cg::Point3(-5.0f, -1.0f, 8.0f),   // bottom-left
        cg::Point3(-5.0f, 4.0f, 8.0f),    // top-left
        cg::Point3(5.0f, 4.0f, 8.0f));    // top-right (reversed for normal toward -Z)
    wall_texture->add_child(back_wall);
    back_wall_material->add_child(wall_texture);
    walls_aabb->add_child(back_wall_material);

    // Left wall (tan/beige)
    auto left_wall_material = std::make_shared<cg::MaterialNode>();
    left_wall_material->set_ambient_and_diffuse(cg::Color4(0.6f, 0.5f, 0.4f, 1.0f));
    left_wall_material->set_specular(cg::Color4(0.2f, 0.2f, 0.2f, 1.0f));
    left_wall_material->set_shininess(8.0f);

    auto left_wall = std::make_shared<cg::RTQuadNode>(
        cg::Point3(-5.0f, -1.0f, 8.0f),   // bottom-back
        cg::Point3(-5.0f, -1.0f, -2.0f),  // bottom-front
        cg::Point3(-5.0f, 4.0f, -2.0f),   // top-front
        cg::Point3(-5.0f, 4.0f, 8.0f));   // top-back (reversed for normal toward +X)
    left_wall_material->add_child(std::static_pointer_cast<cg::SceneNode>(left_wall));
    walls_aabb->add_child(left_wall_material);

    scene_node->add_child(walls_aabb);

    // ========== TRIANGLE MESH OBJECTS + GREEN SPHERE (wrapped in AABB) ==========
    // All meshes and the green sphere are wrapped in a parent AABBNode for hierarchical culling
    // Green sphere at (0,0,3) r=0.4 falls within these bounds

    // Create parent bounding box that encompasses all mesh objects and green sphere
    auto mesh_bounding_box = std::make_shared<cg::AABBNode>();
    mesh_bounding_box->set(
        cg::Point3(-5.0f, -1.5f, 1.0f),   // min corner
        cg::Point3(4.0f, 1.0f, 8.0f)      // max corner
    );

    // Add green sphere to this group (it's spatially within these bounds)
    mesh_bounding_box->add_child(green_material);

    // Mesh 1: Yellow pyramid - defined at origin, transformed to position
    // Rough gold metallic - low shininess gives high roughness in Cook-Torrance,
    // producing a broad, soft specular highlight (vs. Phong's sharp falloff)
    auto pyramid_material = std::make_shared<cg::MaterialNode>();
    pyramid_material->set_ambient_and_diffuse(cg::Color4(0.9f, 0.8f, 0.1f, 1.0f));
    pyramid_material->set_specular(cg::Color4(0.8f, 0.7f, 0.3f, 1.0f));  // Gold-tinted specular (metallic)
    pyramid_material->set_shininess(8.0f);  // Low shininess = high roughness for Cook-Torrance

    // Unit pyramid centered at origin with base at y=0 and apex at y=1
    std::vector<cg::Point3> pyramid_verts = {
        cg::Point3(-0.5f, 0.0f, -0.5f),   // 0: base front-left
        cg::Point3( 0.5f, 0.0f, -0.5f),   // 1: base front-right
        cg::Point3( 0.5f, 0.0f,  0.5f),   // 2: base back-right
        cg::Point3(-0.5f, 0.0f,  0.5f),   // 3: base back-left
        cg::Point3( 0.0f, 1.0f,  0.0f)    // 4: apex
    };
    std::vector<uint16_t> pyramid_faces = {
        // 4 triangular sides (CCW winding for outward normals)
        1, 0, 4,  // front face
        2, 1, 4,  // right face
        3, 2, 4,  // back face
        0, 3, 4,  // left face
        // base (2 triangles, CCW from below)
        0, 1, 2,  0, 2, 3
    };
    auto pyramid_mesh = std::make_shared<cg::RTMeshNode>(pyramid_verts, pyramid_faces);

    // Transform: scale by 1.5, rotate 15 degrees, translate to (-2.5, -1, 2.5)
    auto pyramid_transform = std::make_shared<cg::RTTransformNode>();
    pyramid_transform->translate(-2.5f, -1.0f, 2.5f);
    pyramid_transform->rotate_y(15.0f);
    pyramid_transform->scale(1.5f, 1.5f, 1.5f);
    pyramid_transform->add_child(pyramid_mesh);
    pyramid_material->add_child(pyramid_transform);
    mesh_bounding_box->add_child(pyramid_material);  // Add to bounding box instead of scene

    // Mesh 2: Cyan box/cube - reflective, defined at origin, transformed to position
    auto box_material = std::make_shared<cg::MaterialNode>();
    box_material->set_ambient_and_diffuse(cg::Color4(0.1f, 0.5f, 0.5f, 1.0f));
    box_material->set_specular(cg::Color4(0.8f, 0.8f, 0.8f, 1.0f));
    box_material->set_shininess(64.0f);
    box_material->set_global_reflectivity(0.3f, 0.3f, 0.3f);

    // Unit cube centered at origin from -0.5 to 0.5
    std::vector<cg::Point3> box_verts = {
        cg::Point3(-0.5f, -0.5f, -0.5f),  // 0: front-bottom-left
        cg::Point3( 0.5f, -0.5f, -0.5f),  // 1: front-bottom-right
        cg::Point3( 0.5f,  0.5f, -0.5f),  // 2: front-top-right
        cg::Point3(-0.5f,  0.5f, -0.5f),  // 3: front-top-left
        cg::Point3(-0.5f, -0.5f,  0.5f),  // 4: back-bottom-left
        cg::Point3( 0.5f, -0.5f,  0.5f),  // 5: back-bottom-right
        cg::Point3( 0.5f,  0.5f,  0.5f),  // 6: back-top-right
        cg::Point3(-0.5f,  0.5f,  0.5f)   // 7: back-top-left
    };
    std::vector<uint16_t> box_faces = {
        0, 1, 2,  0, 2, 3,  // front
        5, 4, 7,  5, 7, 6,  // back
        4, 0, 3,  4, 3, 7,  // left
        1, 5, 6,  1, 6, 2,  // right
        3, 2, 6,  3, 6, 7,  // top
        4, 5, 1,  4, 1, 0   // bottom
    };
    auto box_mesh = std::make_shared<cg::RTMeshNode>(box_verts, box_faces);

    // Transform: scale to 0.8, rotate 30 degrees, translate to (2.9, -0.6, 4.4)
    auto box_transform = std::make_shared<cg::RTTransformNode>();
    box_transform->translate(2.9f, -0.6f, 4.4f);
    box_transform->rotate_y(30.0f);
    box_transform->scale(0.8f, 0.8f, 0.8f);
    box_transform->add_child(box_mesh);
    box_material->add_child(box_transform);
    mesh_bounding_box->add_child(box_material);  // Add to bounding box instead of scene

    // Mesh 3: Purple wedge/ramp - defined at origin, transformed to position
    auto wedge_material = std::make_shared<cg::MaterialNode>();
    wedge_material->set_ambient_and_diffuse(cg::Color4(0.6f, 0.2f, 0.6f, 1.0f));
    wedge_material->set_specular(cg::Color4(0.4f, 0.4f, 0.4f, 1.0f));
    wedge_material->set_shininess(16.0f);

    // Unit wedge: base from -0.5 to 0.5 in x/z, height 1, slope from front to back
    std::vector<cg::Point3> wedge_verts = {
        cg::Point3(-0.5f, 0.0f, -0.5f),   // 0 front-left bottom
        cg::Point3( 0.5f, 0.0f, -0.5f),   // 1 front-right bottom
        cg::Point3( 0.5f, 0.0f,  0.5f),   // 2 back-right bottom
        cg::Point3(-0.5f, 0.0f,  0.5f),   // 3 back-left bottom
        cg::Point3(-0.5f, 1.0f,  0.5f),   // 4 back-left top
        cg::Point3( 0.5f, 1.0f,  0.5f)    // 5 back-right top
    };
    std::vector<uint16_t> wedge_faces = {
        // Bottom (facing -Y, CCW from below)
        0, 1, 2,  0, 2, 3,
        // Back vertical face (facing +Z, CCW from back)
        2, 5, 4,  2, 4, 3,
        // Slope face (facing -Z/+Y, CCW from front-above)
        1, 0, 4,  1, 4, 5,
        // Left triangle (facing -X, CCW from left)
        3, 4, 0,
        // Right triangle (facing +X, CCW from right)
        1, 5, 2
    };
    auto wedge_mesh = std::make_shared<cg::RTMeshNode>(wedge_verts, wedge_faces);

    // Transform: scale by (1, 1, 2), rotate -20 degrees, translate to (-3.5, -1, 6)
    auto wedge_transform = std::make_shared<cg::RTTransformNode>();
    wedge_transform->translate(-3.5f, -1.0f, 6.0f);
    wedge_transform->rotate_y(-20.0f);
    wedge_transform->scale(1.0f, 1.0f, 2.0f);
    wedge_transform->add_child(wedge_mesh);
    wedge_material->add_child(wedge_transform);
    mesh_bounding_box->add_child(wedge_material);  // Add to bounding box instead of scene

    // Add the bounding box (containing all meshes) to the scene
    scene_node->add_child(mesh_bounding_box);

    // Single strong directional light - positioned high and at an angle
    // so each face of pyramid/wedge receives different illumination
    auto light = std::make_shared<cg::LightNode>(0);
    light->set_position(cg::HPoint3(4.0f, 6.0f, -1.0f, 1.0f));  // High, front-right
    light->set_diffuse(cg::Color4(1.2f, 1.2f, 1.1f, 1.0f));     // Strong white light (>1 for intensity)
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

    // Only supersample on the final pass (block_size == 1).
    // Coarser passes are just progressive previews that get overwritten.
    const bool supersample = (block_size == 1);

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
    g_frame_buffer->anti_alias();
    SDL_GL_SwapWindow(g_sdl_window);
}

#else

/**
 * Display function, single-threaded
 */
void display()
{
    // Clear the memory framebuffer
    g_frame_buffer->clear();

    for(int32_t block_size = cg::FB_BLOCK_SIZE; block_size > 0; block_size /= 2)
    {
        render_rows(0, g_image_height+1, block_size);
        
        g_frame_buffer->render();
        SDL_GL_SwapWindow(g_sdl_window);
    }

    // Final scene - simple anti-aliasing (without shooting additional rays)
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
    // Move camera much further back to see the sphere better
    g_camera->set_position_and_look_at_pt(cg::Point3(7.0f, 0.5f, -5.0f), cg::Point3(0.0f, 0.0f, 0.0f));
    g_camera->set_view_up(cg::Vector3(0.0f, 1.0f, 0.0f));  // Y is up
    // Initialize view volume for ray tracing
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
