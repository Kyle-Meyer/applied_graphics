#include "RayTracer/ray_tracer.hpp"
#include "RayTracer/texture_node.hpp"

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
    current_state.texture_node = nullptr;
    current_state.transform_required = false;
    current_state.inverse_matrix.set_identity();
    current_state.normal_matrix.set_identity();

    SceneState closest;
    closest.t_min = 1e30f;  // Initialize to very large value
    closest.geometry_node = nullptr;
    closest.material_node = nullptr;
    closest.texture_node = nullptr;
    closest.transform_required = false;

    scene_root_->find_closest_intersect(ray, current_state, closest);

    // If no object hit, return background value
    if(!closest.geometry_node) { return Color3(0.0f, 0.0f, 0.0f); }

    // Get the nearest object and state
    MaterialNode *material = (MaterialNode *)closest.material_node;
    GeometryNode *nearest_object = (GeometryNode *)closest.geometry_node;

    // Find the intersection point (in world space)
    Point3 int_pt = ray.intersect(closest.t_min);

    // Get the normal in object space
    Vector3 normal = nearest_object->get_normal(int_pt);

    // Transform normal to world space if object had modeling transforms
    if (closest.transform_required)
    {
        // The normal matrix (transpose of inverse) transforms normals correctly
        normal = closest.normal_matrix * normal;
        normal.normalize();
    }

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

    // Get texture color if the intersected object has a texture and modulate
    if (closest.texture_node)
    {
        TextureNode *texture = static_cast<TextureNode *>(closest.texture_node);

        // Try procedural (world-space) first
        Color3 tex_color = texture->get_color(int_pt);

        // If procedural returns black, try UV-based texture coordinates
        if (tex_color.r == 0.0f && tex_color.g == 0.0f && tex_color.b == 0.0f)
        {
            Point2 tex_pt = nearest_object->get_texture_coord(int_pt);
            TextureCoord2 tex_coord(tex_pt.x, tex_pt.y);
            tex_color = texture->get_color(tex_coord);
        }

        // Modulate color by texture (multiply)
        color.r *= tex_color.r;
        color.g *= tex_color.g;
        color.b *= tex_color.b;
    }

    // Return if max depth is reached or attenuation is below threshold
    // (do not spawn additional rays)
    if(ray.recursion_level_ <= 0 || ray.below_threshold())
    {
       color.clamp();
       return color;
    }

    // Spawn a reflected ray if material is reflective - add to color
    if (material->is_reflective())
    {
        // Compute reflection direction: R = D - 2(D·N)N
        // This formula works correctly regardless of which side we hit from
        Vector3 reflect_dir = ray.d - normal * (2.0f * ray.d.dot(normal));
        reflect_dir.normalize();

        // Create reflected ray origin slightly offset from surface
        Point3 reflect_origin = int_pt + reflect_dir * EPSILON;

        // Create reflected ray with decremented recursion level
        Ray3 reflected_ray3(reflect_origin, reflect_dir);
        Ray reflected_ray(reflected_ray3, ray.recursion_level_ - 1, ray.threshold_);

        // Recursively trace reflected ray
        Color3 reflected_color = trace_ray(reflected_ray);

        // Add reflected contribution weighted by reflectivity
        const Color3& reflectivity = material->get_global_reflectivity();
        color.r += reflected_color.r * reflectivity.r;
        color.g += reflected_color.g * reflectivity.g;
        color.b += reflected_color.b * reflectivity.b;
    }

    // Spawn a transmitted ray if material is transparent - add to color
    if (material->is_transparent())
    {
        bool total_internal_reflection = false;
        Ray refracted_ray = ray.get_refracted_ray(int_pt, normal, material, total_internal_reflection);

        if (!total_internal_reflection)
        {
            // Recursively trace refracted ray
            Color3 refracted_color = trace_ray(refracted_ray);

            // Add refracted contribution weighted by transmission coefficients
            const Color3& transmission = material->get_global_transmission();
            color.r += refracted_color.r * transmission.r;
            color.g += refracted_color.g * transmission.g;
            color.b += refracted_color.b * transmission.b;
        }
        else
        {
            // Total internal reflection - treat as reflection
            Vector3 reflect_dir = ray.d - normal * (2.0f * ray.d.dot(normal));
            reflect_dir.normalize();
            Point3 reflect_origin = int_pt + reflect_dir * EPSILON;

            Ray3 reflected_ray3(reflect_origin, reflect_dir);
            Ray reflected_ray(reflected_ray3, ray.recursion_level_ - 1, ray.threshold_);

            Color3 reflected_color = trace_ray(reflected_ray);

            const Color3& transmission = material->get_global_transmission();
            color.r += reflected_color.r * transmission.r;
            color.g += reflected_color.g * transmission.g;
            color.b += reflected_color.b * transmission.b;
        }
    }

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
