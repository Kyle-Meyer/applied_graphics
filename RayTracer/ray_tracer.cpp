#include "RayTracer/ray_tracer.hpp"
#include "RayTracer/texture_node.hpp"
#include "RayTracer/normal_map_node.hpp"

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
    current_state.normal_map_node = nullptr;
    current_state.transform_required = false;
    current_state.inverse_matrix.set_identity();
    current_state.normal_matrix.set_identity();

    SceneState closest;
    closest.t_min = 1e30f;  // Initialize to very large value
    closest.geometry_node = nullptr;
    closest.material_node = nullptr;
    closest.texture_node = nullptr;
    closest.normal_map_node = nullptr;
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

    // Apply normal map perturbation if this object has a normal map
    if (closest.normal_map_node)
    {
        NormalMapNode *nm = static_cast<NormalMapNode *>(closest.normal_map_node);

        // Get UV coords and object-space tangent from geometry
        Point2  uv = nearest_object->get_texture_coord(int_pt);
        Vector3 T  = nearest_object->get_tangent(int_pt);

        // Transform tangent to world space (same transform as normals)
        if (closest.transform_required)
        {
            T = closest.normal_matrix * T;
        }

        // Gram-Schmidt re-orthogonalize T against the world-space normal
        T = T - normal * normal.dot(T);
        float tlen = T.norm();
        if (tlen > 1e-6f)
        {
            T = T * (1.0f / tlen);
            Vector3 B  = normal.cross(T);   // bitangent
            Vector3 tn = nm->sample(uv, int_pt);  // tangent-space normal from map

            // Rotate tangent-space normal into world space via TBN
            normal = (T * tn.x + B * tn.y + normal * tn.z);
            normal.normalize();
        }
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

    // Iterate through all lights, tracking average shadow factor for sparkle
    float avg_shadow = 0.0f;
    int   light_count = 0;
    for(LightNode *light : lights_)
    {
        // Get light position (or directional-light anchor for w=0 lights)
        Point3 light_pos = light->get_position();

        // Compute shadow factor: 0=fully shadowed, 1=fully lit, fraction=soft penumbra
        float sf = shadow_factor(int_pt, light_pos, nearest_object);
        avg_shadow += sf;
        ++light_count;
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
    if (light_count > 1)
        avg_shadow /= static_cast<float>(light_count);

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

    // Additive sparkle effect (moonlit quartz glinting) — sand and similar materials.
    // Each "sparkle grain" is a micro-facet with a randomly tilted normal.  It only
    // glints when the half-vector between the light and the camera aligns with that
    // tilted normal — making the effect view-dependent (camera-position-sensitive).
    if (material->has_sparkle() && avg_shadow > 0.0f && !lights_.empty())
    {
        Point2 uv   = nearest_object->get_texture_coord(int_pt);
        float  freq = material->sparkle_frequency();
        float  su   = uv.x * freq;
        float  sv   = uv.y * freq;

        // Hash 0: selects which UV cells contain a glinting quartz facet
        auto fract = [](float x){ return x - std::floor(x); };
        float h0 = fract(std::fabs(std::sin(su * 12.9898f + sv * 78.233f)  * 43758.5453f));

        if (h0 >= material->sparkle_threshold())
        {
            // Hash 1 & 2: stable per-grain tilt direction and magnitude
            float h1 = fract(std::fabs(std::sin(su *  4.1414f + sv *  2.7183f) * 31415.9265f));
            float h2 = fract(std::fabs(std::sin(su *  2.3606f + sv *  1.6180f) * 27182.8182f));

            // Build a tangent frame so we can tilt the normal in a random direction
            Vector3 T(1.0f, 0.0f, 0.0f);
            if (std::fabs(normal.dot(T)) > 0.9f)
                T = Vector3(0.0f, 0.0f, 1.0f);
            T = T - normal * normal.dot(T);
            T = T * (1.0f / T.norm());
            Vector3 B = normal.cross(T);

            // Tilt the grain's micro-mirror normal by up to ~30° in a random direction
            float tilt_phi = h1 * 2.0f * static_cast<float>(M_PI);
            Vector3 tilt_axis = T * std::cos(tilt_phi) + B * std::sin(tilt_phi);
            float   tilt_mag  = h2 * 0.55f;   // 0..0.55 radians (~31° max)
            Vector3 N_grain   = normal + tilt_axis * std::sin(tilt_mag);
            float   ng_len    = N_grain.norm();
            if (ng_len < 1e-6f) goto sparkle_done;
            N_grain = N_grain * (1.0f / ng_len);

            {
                // View direction toward camera
                Vector3 V(-ray.d.x, -ray.d.y, -ray.d.z);

                // Light direction toward moon (first light only)
                Point3  light_pos = lights_[0]->get_position();
                Vector3 L(int_pt, light_pos);
                L.normalize();

                // Half-vector: grain glints when N_grain aligns with H
                Vector3 H   = V + L;
                float   hlen = H.norm();
                if (hlen > 1e-6f)
                {
                    H = H * (1.0f / hlen);
                    float spec      = std::max(0.0f, N_grain.dot(H));
                    // Two-term lobe: broad dim glow for off-angle grains,
                    // tight bright peak for well-aligned grains.
                    float intensity = (0.08f * std::pow(spec, 3.0f)
                                      + std::pow(spec, 80.0f))
                                    * material->sparkle_intensity()
                                    * avg_shadow;
                    if (intensity > 1e-4f)
                    {
                        Color3 sc = material->sparkle_color();
                        color.r += sc.r * intensity;
                        color.g += sc.g * intensity;
                        color.b += sc.b * intensity;
                    }
                }
            }
        }
        sparkle_done:;
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
    const float TWO_PI    = 2.0f * static_cast<float>(M_PI);
    const float inv_n     = 1.0f / static_cast<float>(n);
    for(int i = 0; i < n; ++i)
    {
        // Stratified disc sampling: divide the angular range into n equal sectors
        // and jitter within each sector.  This spreads samples evenly around the
        // disc and removes the clumping that causes coarse-dirt granularity.
        float angle = (static_cast<float>(i) + rand01(rng)) * inv_n * TWO_PI;
        float r     = disc_radius_ * sqrtf((static_cast<float>(i) + rand01(rng)) * inv_n);
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
