#include "RayTracer/ray_tracer.hpp"
#include "RayTracer/texture_node.hpp"

#include "geometry/geometry.hpp"

#include <cmath>
#include <iostream>
#include <random>

namespace cg
{

RayTracer::RayTracer(std::shared_ptr<SceneNode> scene_root)
{
    scene_root_ = scene_root;

    // Initialize lighting support. Set the global ambient here.
    lighting_.set_ambient(Color3(0.25f, 0.25f, 0.25f));

    // Soft shadow defaults: 8 samples, disc radius 2 world-units, enabled.
    soft_shadows_enabled_ = true;
    shadow_samples_       = 8;
    disc_radius_          = 2.0f;
}

void RayTracer::set_soft_shadows(bool enable)
{
    soft_shadows_enabled_ = enable;
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
    if(!closest.geometry_node) { return Color3(0.02f, 0.04f, 0.08f); }

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
        // Get light position (or directional-light anchor for w=0 lights)
        Point3 light_pos = light->get_position();

        // Compute shadow factor: 0=fully shadowed, 1=fully lit, fraction=soft penumbra
        float sf = shadow_factor(int_pt, light_pos, nearest_object);
        if(sf > 0.0f)
        {
            // Compute diffuse and specular contribution, then scale by shadow factor
            Color3 diffuse, specular;
            lighting_.local_contribution(light, material, int_pt, normal, diffuse, specular);

            color.r += (diffuse.r + specular.r) * sf;
            color.g += (diffuse.g + specular.g) * sf;
            color.b += (diffuse.b + specular.b) * sf;
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

float RayTracer::shadow_factor(const Point3 &int_pt, Point3 &light_pos, SceneNode *current_obj)
{
    // Base direction from hit point toward the light centre
    Vector3 to_light(int_pt, light_pos);
    float   dist_to_light = to_light.norm();
    to_light.normalize();

    // Offset origin to avoid self-intersection
    Point3 shadow_origin = int_pt + to_light * EPSILON;

    const int n = soft_shadows_enabled_ ? shadow_samples_ : 1;

    if(n == 1)
    {
        // Hard shadow: single ray toward the light centre
        Ray3       shadow_ray(shadow_origin, to_light);
        SceneState state;
        state.geometry_node = current_obj;
        return scene_root_->does_intersect_exist(shadow_ray, dist_to_light, state) ? 0.0f : 1.0f;
    }

    // Soft shadow: build an orthonormal disc basis perpendicular to to_light
    Vector3 up(0.0f, 1.0f, 0.0f);
    if(fabsf(to_light.dot(up)) > 0.99f) up = Vector3(1.0f, 0.0f, 0.0f);
    Vector3 disc_u = to_light.cross(up);  disc_u.normalize();
    Vector3 disc_v = to_light.cross(disc_u); disc_v.normalize();

    // Thread-local RNG — safe for the multithreaded render path
    static thread_local std::mt19937                          rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> rand01(0.0f, 1.0f);

    int lit = 0;
    for(int i = 0; i < n; ++i)
    {
        // Uniform disc sample via polar coordinates (no rejection loop needed)
        float angle = rand01(rng) * 2.0f * static_cast<float>(M_PI);
        float r     = disc_radius_ * sqrtf(rand01(rng));
        float ox    = r * cosf(angle);
        float oy    = r * sinf(angle);

        // Offset the light anchor point on the disc plane
        Point3 sample_pos(
            light_pos.x + disc_u.x * ox + disc_v.x * oy,
            light_pos.y + disc_u.y * ox + disc_v.y * oy,
            light_pos.z + disc_u.z * ox + disc_v.z * oy);

        Vector3 to_sample(int_pt, sample_pos);
        float   dist_sample = to_sample.norm();
        to_sample.normalize();

        Ray3       shadow_ray(int_pt + to_sample * EPSILON, to_sample);
        SceneState state;
        state.geometry_node = current_obj;
        if(!scene_root_->does_intersect_exist(shadow_ray, dist_sample, state))
            ++lit;
    }

    return static_cast<float>(lit) / static_cast<float>(n);
}

} // namespace cg
