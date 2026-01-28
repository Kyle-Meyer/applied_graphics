#include "RayTracer/ray_tracer.hpp"

#include <iostream>

namespace cg
{

RayTracer::RayTracer(std::shared_ptr<SceneNode> scene_root)
{
    scene_root_ = scene_root;

    // Initialize lighting support. Set the global ambient here.

    lighting_.set_ambient(Color3(0.25f, 0.25f, 0.25f));
}

RayTracer::~RayTracer() {}

Color3 RayTracer::trace_ray(Ray3 &initial_ray, int depth, float adaptive_threshold)
{
    Ray ray(initial_ray, depth, adaptive_threshold);
    return trace_ray(ray);
}

Color3 RayTracer::trace_ray(Ray &ray)
{

    // Hint: try moving the origin of the ray slightly along the ray direction to prevent
    // self intersection

    // Traverse the scene graph to find closest intersecting object
    SceneState current_state;
    SceneState closest;
    closest.t_min = 1e30f;  // Initialize to very large value
    closest.geometry_node = nullptr;
    closest.material_node = nullptr;

    scene_root_->find_closest_intersect(ray, current_state, closest);

    // If no object hit, return background value
    if(!closest.geometry_node) { return Color3(0.0f, 0.0f, 0.0f); }

    // Get the nearest object and state
    MaterialNode *material = (MaterialNode *)closest.material_node;
    GeometryNode *nearest_object = (GeometryNode *)closest.geometry_node;

    // Find the intersection point
    Point3 int_pt = ray.intersect(closest.t_min);

    // Get the normal (transform if needed)
    Vector3 normal = nearest_object->get_normal(int_pt);

    // PART 2 TEST: Visualize the normal as a color to prove get_normal() works
    // Map normal from [-1,1] to [0,1] color range
    Color3 color((normal.x + 1.0f) * 0.5f,
                 (normal.y + 1.0f) * 0.5f,
                 (normal.z + 1.0f) * 0.5f);

    // Print debug info for center pixel
    static bool printed = false;
    if(!printed && std::abs(ray.d.x) < 0.01f && std::abs(ray.d.y) < 0.01f) {
        printed = true;
        std::cout << "\n=== PART 2 TEST (Center Ray) ===\n";
        std::cout << "Ray hit sphere: YES\n";
        std::cout << "Intersection point: (" << int_pt.x << ", " << int_pt.y << ", " << int_pt.z << ")\n";
        std::cout << "Normal at intersection: (" << normal.x << ", " << normal.y << ", " << normal.z << ")\n";
        std::cout << "Normal length: " << normal.norm() << " (should be ~1.0)\n";

        Point2 tex = nearest_object->get_texture_coord(int_pt);
        std::cout << "Texture coord: (" << tex.x << ", " << tex.y << ")\n";
        std::cout << "\nExpected: Normal should point from sphere center toward camera\n";
        std::cout << "Sphere center is at (0,0,0), camera at (0,0,10)\n";
        std::cout << "So normal at front should be approximately (0, 0, 1)\n";
        std::cout << "\nColor on screen: Normal mapped to RGB color\n";
        std::cout << "You should see a sphere with color gradient showing normals.\n";
        std::cout << "==================================\n\n";
    }

    // Get the texture color if the intersected object has a texture

    // Get local color contribution. Iterate through the active lights, check if in shadow.

    // Calculate the local color. Modulate texture color.

    // Return if max depth is reached or attenuation is below threshold
    // (do not spawn additional rays)

    // Reverse the normal direction if the ray is inside

    // Spawn a reflected ray if material is reflective - add to color

    // Spawn a transmitted ray if material is transparent - add to color

    // Clamp color
    color.clamp();
    return color;
}

void RayTracer::set_view_position(const Point3 &pos) { lighting_.set_view_position(pos); }

bool RayTracer::in_shadow(const Point3 &int_pt, Point3 &light_pos, SceneNode *current_obj)
{
    // Complete in 605.767.
    return false;
}

} // namespace cg
