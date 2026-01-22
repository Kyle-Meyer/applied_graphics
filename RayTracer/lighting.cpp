#include "RayTracer/lighting.hpp"

#include <cmath>

namespace cg
{

Lighting::Lighting() { ambient_.set(1.0f, 1.0f, 1.0f); }

void Lighting::set_ambient(const Color3 &color) { ambient_ = color; }

Color3 Lighting::get_ambient(const MaterialNode *material_node) const
{
    return material_node->get_ambient() * ambient_;
}

void Lighting::set_view_position(const Point3 &pos) { view_position_ = pos; }

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
        // Take dot product of vector from the light to the point (-L) with
        // the spotlight dir.
        float V_dot_D = -L.dot(light->get_spotlight_direction());

        // Set attenuation to 0 if outside the spotlight cutoff angle
        // otherwise multiply attenuation time spotlight factor
        if(V_dot_D < light->get_spotlight_cutoff()) { att = 0.0f; }
        else { att *= std::max<float>(std::pow(V_dot_D, light->get_spotlight_exponent()), 0.0f); }
    }

    if(att == 0.0f)
    {
        diffuse.set(0.0f, 0.0f, 0.0f);
        specular.set(0.0f, 0.0f, 0.0f);
        return;
    }

    // Calculate diffuse component. Recalculate N_dot_L (since L is now a unit
    // length vector)
    float N_dot_L = normal.dot(L);
    diffuse = light->get_diffuse() * material_node->get_diffuse() * (N_dot_L * att);

    // Calculate specular component
    if(material_node->get_shininess() > 0.0f)
    {
        // Construct halfway vector
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

} // namespace cg
