#include "RayTracer/ray_tracer.hpp"

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
    // FOR TESTING ONLY: Delete or comment out the following block
    {
        // Return color for checkerboard pattern that is N-pixels squared
        constexpr float N = 50.0f; // Grid cell dimension
        const Color3    dark_color(0.2f, 0.2f, 0.2f);
        const Color3    light_color(0.8f, 0.8f, 0.8f);
        if((static_cast<int>(ray.o.x / N) + static_cast<int>(ray.o.y / N)) % 2) return dark_color;
        else return light_color;
    }

    // Hint: try moving the origin of the ray slightly along the ray direction to prevent
    // self intersection

    // Traverse the scene graph to find closest intersecting object
    SceneState current_state;
    SceneState closest;
    scene_root_->find_closest_intersect(ray, current_state, closest);

    // If no object hit, return background value
    if(!closest.geometry_node) { return Color3(0.0f, 0.0f, 0.0f); }

    // Get the nearest object and state
    MaterialNode *material = (MaterialNode *)closest.material_node;
    GeometryNode *nearest_object = (GeometryNode *)closest.geometry_node;

    // Find the intersection point
    Point3 int_pt = ray.intersect(closest.t_min);
   
   
    // Get the normal (transform if needed)
    Vector3 normal;

    Color3 color;

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
