#include "RayTracer/ray_tracer.hpp"

#include "geometry/geometry.hpp"

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

    // Check if material exists
    if(!material)
    {
        // No material - return a default gray color
        return Color3(0.5f, 0.5f, 0.5f);
    }

    // Start with ambient contribution
    Color3 color = lighting_.get_ambient(material);

    // Add emission if any
    const Color4 &emission = material->get_emission();
    color.r += emission.r;
    color.g += emission.g;
    color.b += emission.b;

    // Iterate through all lights
    for(LightNode *light : lights_)
    {
        // Get light position
        Point3 light_pos = light->get_position();

        // Check if point is in shadow with respect to this light
        if(!in_shadow(int_pt, light_pos, nearest_object))
        {
            // Not in shadow - compute diffuse and specular contribution
            Color3 diffuse, specular;
            lighting_.local_contribution(light, material, int_pt, normal, diffuse, specular);

            // Add light contribution
            color.r += diffuse.r + specular.r;
            color.g += diffuse.g + specular.g;
            color.b += diffuse.b + specular.b;
        }
    }

    // TODO: Get the texture color if the intersected object has a texture
    // and modulate the color

    // TODO: Return if max depth is reached or attenuation is below threshold
    // (do not spawn additional rays)

    // TODO: Reverse the normal direction if the ray is inside

    // TODO: Spawn a reflected ray if material is reflective - add to color

    // TODO: Spawn a transmitted ray if material is transparent - add to color

    // Clamp color
    color.clamp();
    return color;
}

void RayTracer::set_view_position(const Point3 &pos) { lighting_.set_view_position(pos); }

void RayTracer::add_light(LightNode *light) { lights_.push_back(light); }

bool RayTracer::in_shadow(const Point3 &int_pt, Point3 &light_pos, SceneNode *current_obj)
{
    // Construct a shadow ray from the intersection point toward the light
    Vector3 to_light(int_pt, light_pos);
    float   distance_to_light = to_light.norm();
    to_light.normalize();

    // Offset the ray origin slightly to prevent self-intersection
    Point3 shadow_origin = int_pt + to_light * EPSILON;

    // Create the shadow ray
    Ray3 shadow_ray(shadow_origin, to_light);

    // Set up scene state - store current object so convex objects can skip self-test
    SceneState current_state;
    current_state.geometry_node = current_obj;

    // Check if any object blocks the path to the light
    return scene_root_->does_intersect_exist(shadow_ray, distance_to_light, current_state);
}

} // namespace cg
