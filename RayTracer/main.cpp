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
#include "RayTracer/lighting.hpp"
#include "RayTracer/material_node.hpp"
#include "RayTracer/ray_tracer.hpp"
#include "RayTracer/rt_sphere_node.hpp"
#include "RayTracer/rt_quad_node.hpp"
#include "RayTracer/rt_mesh_node.hpp"

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

    // // Red sphere at origin
    // auto material = std::make_shared<cg::MaterialNode>();
    // material->set_ambient_and_diffuse(cg::Color4(0.8f, 0.2f, 0.2f, 1.0f));
    // material->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    // material->set_shininess(64.0f);
    // auto sphere = std::make_shared<cg::RTSphereNode>(cg::Point3(0.0f, 0.0f, 0.0f), 0.5f);
    // material->add_child(std::static_pointer_cast<cg::SceneNode>(sphere));
    // scene_node->add_child(material);

    // Add a floor using a very large sphere (appears nearly flat)
    auto floor_material = std::make_shared<cg::MaterialNode>();
    floor_material->set_ambient_and_diffuse(cg::Color4(0.4f, 0.4f, 0.5f, 1.0f));
    floor_material->set_specular(cg::Color4(0.3f, 0.3f, 0.3f, 1.0f));
    floor_material->set_shininess(16.0f);

    // Large sphere with center far below, surface at y = -1
    // Radius 1000, center at (0, -1001, 0) puts the top of the sphere at y = -1
    auto floor = std::make_shared<cg::RTSphereNode>(cg::Point3(0.0f, -1001.0f, 0.0f), 1000.0f);
    floor_material->add_child(std::static_pointer_cast<cg::SceneNode>(floor));
    scene_node->add_child(floor_material);

    // // Mirror ball (reflective)
    auto mirror_material = std::make_shared<cg::MaterialNode>();
    mirror_material->set_ambient_and_diffuse(cg::Color4(0.1f, 0.1f, 0.1f, 1.0f));
    mirror_material->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    mirror_material->set_shininess(128.0f);
    mirror_material->set_global_reflectivity(0.9f, 0.9f, 0.9f);
    auto mirror_sphere = std::make_shared<cg::RTSphereNode>(
        cg::Point3(-1.5f, 0.0f, 0.0f), 0.5f);
    mirror_material->add_child(std::static_pointer_cast<cg::SceneNode>(mirror_sphere));
    scene_node->add_child(mirror_material);

    // // Green sphere
    // auto green_material = std::make_shared<cg::MaterialNode>();
    // green_material->set_ambient_and_diffuse(cg::Color4(0.2f, 0.8f, 0.2f, 1.0f));
    // green_material->set_specular(cg::Color4(0.5f, 0.5f, 0.5f, 1.0f));
    // green_material->set_shininess(32.0f);
    // auto green_sphere = std::make_shared<cg::RTSphereNode>(
    //     cg::Point3(1.5f, 0.0f, 3.0f), 0.4f);
    // green_material->add_child(std::static_pointer_cast<cg::SceneNode>(green_sphere));
    // scene_node->add_child(green_material);

    // // Glass ball (transparent/refractive)
    // auto glass_material = std::make_shared<cg::MaterialNode>();
    // glass_material->set_ambient_and_diffuse(cg::Color4(0.1f, 0.1f, 0.15f, 1.0f));
    // glass_material->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    // glass_material->set_shininess(128.0f);
    // glass_material->set_global_transmission(0.95f, 0.95f, 0.95f);
    // glass_material->set_index_of_refraction(1.0f);
    // auto glass_sphere = std::make_shared<cg::RTSphereNode>(
    //     cg::Point3(1.2f, 0.0f, 0.0f), 0.5f);
    // glass_material->add_child(std::static_pointer_cast<cg::SceneNode>(glass_sphere));
    // scene_node->add_child(glass_material);

    // ========== PLANAR SURFACES (2 walls) ==========

    // Back wall (blue-ish)
    auto back_wall_material = std::make_shared<cg::MaterialNode>();
    back_wall_material->set_ambient_and_diffuse(cg::Color4(0.3f, 0.3f, 0.6f, 1.0f));
    back_wall_material->set_specular(cg::Color4(0.2f, 0.2f, 0.2f, 1.0f));
    back_wall_material->set_shininess(8.0f);

    auto back_wall = std::make_shared<cg::RTQuadNode>(
        cg::Point3(5.0f, -1.0f, 8.0f),    // bottom-right
        cg::Point3(-5.0f, -1.0f, 8.0f),   // bottom-left
        cg::Point3(-5.0f, 4.0f, 8.0f),    // top-left
        cg::Point3(5.0f, 4.0f, 8.0f));    // top-right (reversed for normal toward -Z)
    back_wall_material->add_child(std::static_pointer_cast<cg::SceneNode>(back_wall));
    scene_node->add_child(back_wall_material);

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
    scene_node->add_child(left_wall_material);

    // ========== TRIANGLE MESH OBJECTS (3 meshes) ==========

    // Pyramid vertices (shared across faces)
    cg::Point3 pv0(-3.0f, -1.0f, 2.0f);   // base front-left
    cg::Point3 pv1(-2.0f, -1.0f, 2.0f);   // base front-right
    cg::Point3 pv2(-2.0f, -1.0f, 3.0f);   // base back-right
    cg::Point3 pv3(-3.0f, -1.0f, 3.0f);   // base back-left
    cg::Point3 pv4(-2.5f, 0.5f, 2.5f);    // apex

    // Front face (facing -Z) - RED
    auto front_mat = std::make_shared<cg::MaterialNode>();
    front_mat->set_ambient_and_diffuse(cg::Color4(1.0f, 0.0f, 0.0f, 1.0f));
    front_mat->set_specular(cg::Color4(0.3f, 0.3f, 0.3f, 1.0f));
    front_mat->set_shininess(16.0f);
    auto front_face = std::make_shared<cg::RTMeshNode>(
        std::vector<cg::Point3>{pv1, pv0, pv4},
        std::vector<uint16_t>{0, 1, 2});
    front_mat->add_child(front_face);
    scene_node->add_child(front_mat);

    // Right face (facing +X) - GREEN
    auto right_mat = std::make_shared<cg::MaterialNode>();
    right_mat->set_ambient_and_diffuse(cg::Color4(0.0f, 1.0f, 0.0f, 1.0f));
    right_mat->set_specular(cg::Color4(0.3f, 0.3f, 0.3f, 1.0f));
    right_mat->set_shininess(16.0f);
    auto right_face = std::make_shared<cg::RTMeshNode>(
        std::vector<cg::Point3>{pv2, pv1, pv4},
        std::vector<uint16_t>{0, 1, 2});
    right_mat->add_child(right_face);
    scene_node->add_child(right_mat);

    // Back face (facing +Z) - BLUE
    auto back_mat = std::make_shared<cg::MaterialNode>();
    back_mat->set_ambient_and_diffuse(cg::Color4(0.0f, 0.0f, 1.0f, 1.0f));
    back_mat->set_specular(cg::Color4(0.3f, 0.3f, 0.3f, 1.0f));
    back_mat->set_shininess(16.0f);
    auto back_face = std::make_shared<cg::RTMeshNode>(
        std::vector<cg::Point3>{pv3, pv2, pv4},
        std::vector<uint16_t>{0, 1, 2});
    back_mat->add_child(back_face);
    scene_node->add_child(back_mat);

    // Left face (facing -X) - YELLOW
    auto left_mat = std::make_shared<cg::MaterialNode>();
    left_mat->set_ambient_and_diffuse(cg::Color4(1.0f, 1.0f, 0.0f, 1.0f));
    left_mat->set_specular(cg::Color4(0.3f, 0.3f, 0.3f, 1.0f));
    left_mat->set_shininess(16.0f);
    auto left_face = std::make_shared<cg::RTMeshNode>(
        std::vector<cg::Point3>{pv0, pv3, pv4},
        std::vector<uint16_t>{0, 1, 2});
    left_mat->add_child(left_face);
    scene_node->add_child(left_mat);

    // // Mesh 2: Simple box/cube (cyan) - reflective
    // auto box_material = std::make_shared<cg::MaterialNode>();
    // box_material->set_ambient_and_diffuse(cg::Color4(0.1f, 0.5f, 0.5f, 1.0f));
    // box_material->set_specular(cg::Color4(0.8f, 0.8f, 0.8f, 1.0f));
    // box_material->set_shininess(64.0f);
    // box_material->set_global_reflectivity(0.3f, 0.3f, 0.3f);
    // float bx = 2.5f, by = -1.0f, bz = 4.0f;
    // float bs = 0.8f;
    // std::vector<cg::Point3> box_verts = {
    //     cg::Point3(bx, by, bz),
    //     cg::Point3(bx + bs, by, bz),
    //     cg::Point3(bx + bs, by + bs, bz),
    //     cg::Point3(bx, by + bs, bz),
    //     cg::Point3(bx, by, bz + bs),
    //     cg::Point3(bx + bs, by, bz + bs),
    //     cg::Point3(bx + bs, by + bs, bz + bs),
    //     cg::Point3(bx, by + bs, bz + bs)
    // };
    // std::vector<uint16_t> box_faces = {
    //     0, 1, 2,  0, 2, 3,
    //     5, 4, 7,  5, 7, 6,
    //     4, 0, 3,  4, 3, 7,
    //     1, 5, 6,  1, 6, 2,
    //     3, 2, 6,  3, 6, 7,
    //     4, 5, 1,  4, 1, 0
    // };
    // auto box_mesh = std::make_shared<cg::RTMeshNode>(box_verts, box_faces);
    // box_material->add_child(std::static_pointer_cast<cg::SceneNode>(box_mesh));
    // scene_node->add_child(box_material);

    // Mesh 3: Simple wedge/ramp (purple)
    auto wedge_material = std::make_shared<cg::MaterialNode>();
    wedge_material->set_ambient_and_diffuse(cg::Color4(0.6f, 0.2f, 0.6f, 1.0f));
    wedge_material->set_specular(cg::Color4(0.4f, 0.4f, 0.4f, 1.0f));
    wedge_material->set_shininess(16.0f);

    std::vector<cg::Point3> wedge_verts = {
        cg::Point3(-4.0f, -1.0f, 5.0f),   // 0 front-left bottom
        cg::Point3(-3.0f, -1.0f, 5.0f),   // 1 front-right bottom
        cg::Point3(-3.0f, -1.0f, 7.0f),   // 2 back-right bottom
        cg::Point3(-4.0f, -1.0f, 7.0f),   // 3 back-left bottom
        cg::Point3(-4.0f, 0.0f, 7.0f),    // 4 back-left top
        cg::Point3(-3.0f, 0.0f, 7.0f)     // 5 back-right top
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
    wedge_material->add_child(std::static_pointer_cast<cg::SceneNode>(wedge_mesh));
    scene_node->add_child(wedge_material);

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
    for(int32_t row = start_row; row <= end_row; ++row)
    {
        int32_t y = row * block_size;
        for(int32_t x = 0; x < g_image_width; x += block_size)
        {
            // Check if framebuffer has been set for this pixel
            if(!g_frame_buffer->set(x, y))
            {
                // Construct a ray through the specified pixel and find the color
                // using the recursive ray tracer
                cg::Ray3 ray =
                    g_camera->construct_ray(static_cast<float>(x), static_cast<float>(y));
                cg::Color3 color = g_ray_tracer->trace_ray(ray, g_max_depth, DEPTH_THRESHOLD);
                g_frame_buffer->set(x, y, color, block_size);
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
