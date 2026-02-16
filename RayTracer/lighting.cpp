#include "RayTracer/lighting.hpp"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace cg
{

Lighting::Lighting() : use_cook_torrance_(true) { ambient_.set(1.0f, 1.0f, 1.0f); }

void Lighting::set_ambient(const Color3 &color) { ambient_ = color; }

Color3 Lighting::get_ambient(const MaterialNode *material_node) const
{
    return material_node->get_ambient() * ambient_;
}

void Lighting::set_view_position(const Point3 &pos) { view_position_ = pos; }

void Lighting::set_use_cook_torrance(bool enable) { use_cook_torrance_ = enable; }

// GGX/Trowbridge-Reitz normal distribution function
static float distribution_ggx(float N_dot_H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float denom = N_dot_H * N_dot_H * (a2 - 1.0f) + 1.0f;
    return a2 / (static_cast<float>(M_PI) * denom * denom);
}

// Schlick-GGX geometry function (single direction)
static float geometry_schlick_ggx(float N_dot_X, float roughness)
{
    float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    return N_dot_X / (N_dot_X * (1.0f - k) + k);
}

// Smith's method combining geometry obstruction and shadowing
static float geometry_smith(float N_dot_V, float N_dot_L, float roughness)
{
    return geometry_schlick_ggx(N_dot_V, roughness) *
           geometry_schlick_ggx(N_dot_L, roughness);
}

// Schlick's approximation for Fresnel reflectance
static Color3 fresnel_schlick(float cos_theta, const Color3 &F0)
{
    float one_minus = 1.0f - cos_theta;
    float one_minus5 = one_minus * one_minus * one_minus * one_minus * one_minus;
    return Color3(F0.r + (1.0f - F0.r) * one_minus5,
                  F0.g + (1.0f - F0.g) * one_minus5,
                  F0.b + (1.0f - F0.b) * one_minus5);
}

void Lighting::local_contribution(LightNode          *light,
                                  const MaterialNode *material_node,
                                  const Point3       &int_pt,
                                  const Vector3      &normal,
                                  Color3             &diffuse,
                                  Color3             &specular) const
{
    // Construct vector to the light source
    Vector3 L = (light->get_position() - int_pt);

    // If surface is oriented away from light there is no diffuse
    // or specular contribution
    if(normal.dot(L) < 0.0f)
    {
        diffuse.set(0.0f, 0.0f, 0.0f);
        specular.set(0.0f, 0.0f, 0.0f);
        return;
    }

    // Normalize L (find its length d for attenuation calculation)
    float d = L.norm();
    L *= (1.0f / d);

    // Find attenuation
    float att = 1.0f;
    if(light->is_attenuation_enabled()) { att = light->get_attenuation(d); }

    // Multiply attenuation with spotlight effect
    if(light->is_spotlight())
    {
        float V_dot_D = -L.dot(light->get_spotlight_direction());
        if(V_dot_D < light->get_spotlight_cutoff()) { att = 0.0f; }
        else { att *= std::max<float>(std::pow(V_dot_D, light->get_spotlight_exponent()), 0.0f); }
    }

    if(att == 0.0f)
    {
        diffuse.set(0.0f, 0.0f, 0.0f);
        specular.set(0.0f, 0.0f, 0.0f);
        return;
    }

    float N_dot_L = normal.dot(L);

    if(use_cook_torrance_ && material_node->get_shininess() > 0.0f)
    {
        // Cook-Torrance BRDF
        Vector3 V = (view_position_ - int_pt).normalize();
        Vector3 H = (L + V).normalize();

        float N_dot_V = std::max(normal.dot(V), 0.001f);
        float N_dot_H = std::max(normal.dot(H), 0.0f);
        float H_dot_V = std::max(H.dot(V), 0.0f);

        // Map Phong shininess to roughness: higher shininess = lower roughness
        float roughness = std::sqrt(2.0f / (material_node->get_shininess() + 2.0f));
        roughness = std::max(roughness, 0.05f); // clamp to avoid singularities

        // F0: base reflectance from the material's specular color
        const Color4 &spec = material_node->get_specular();
        Color3 F0(spec.r, spec.g, spec.b);

        // Evaluate Cook-Torrance terms
        float  D = distribution_ggx(N_dot_H, roughness);
        float  G = geometry_smith(N_dot_V, N_dot_L, roughness);
        Color3 F = fresnel_schlick(H_dot_V, F0);

        // Specular: DGF / (4 * N·L * N·V)
        float denom = 4.0f * N_dot_L * N_dot_V;
        specular = Color3(F.r * D * G / denom,
                          F.g * D * G / denom,
                          F.b * D * G / denom);

        // Scale by light color and attenuation
        const Color4 &lspec = light->get_specular();
        specular = Color3(specular.r * lspec.r * att * N_dot_L,
                          specular.g * lspec.g * att * N_dot_L,
                          specular.b * lspec.b * att * N_dot_L);

        // Diffuse: energy conservation - reduce diffuse by Fresnel
        const Color4 &ldiff = light->get_diffuse();
        const Color4 &mdiff = material_node->get_diffuse();
        float kd_r = (1.0f - F.r);
        float kd_g = (1.0f - F.g);
        float kd_b = (1.0f - F.b);
        diffuse = Color3(ldiff.r * mdiff.r * kd_r * N_dot_L * att,
                         ldiff.g * mdiff.g * kd_g * N_dot_L * att,
                         ldiff.b * mdiff.b * kd_b * N_dot_L * att);
    }
    else
    {
        // Phong-Blinn fallback
        diffuse = light->get_diffuse() * material_node->get_diffuse() * (N_dot_L * att);

        if(material_node->get_shininess() > 0.0f)
        {
            Vector3 V = (view_position_ - int_pt).normalize();
            Vector3 H = (L + V).normalize();
            float   half = std::max<float>(normal.dot(H), 0.0f);
            if(half > 0.01f)
            {
                specular = light->get_specular() * material_node->get_specular() *
                           (att * std::pow(half, material_node->get_shininess()));
            }
            else { specular = {0.0f, 0.0f, 0.0f}; }
        }
        else { specular = {0.0f, 0.0f, 0.0f}; }
    }
}

} // namespace cg
