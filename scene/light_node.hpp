//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	 David W. Nesbitt
//	File:    light_node.hpp
//	Purpose: Scene graph light node.
//
//============================================================================

#ifndef __SCENE_LIGHT_NODE__HPP__
#define __SCENE_LIGHT_NODE__HPP__

#include "geometry/hpoint3.hpp"
#include "geometry/vector3.hpp"
#include "scene/color4.hpp"
#include "scene/scene_node.hpp"

namespace cg
{

/**
 * Light node. Sets individual light source properties.
 */
class LightNode : public SceneNode
{
  public:
    /**
     * Constructor given the index (light number).
     * @param  idx  Light index.
     */
    LightNode(uint32_t idx);

    /**
     * Enable this light source
     */
    void enable();

    /**
     * Disable this light source
     */
    void disable();

    /**
     * Set ambient light illumination.
     * @param  c  Ambient light color/intensity
     */
    void set_ambient(const Color4 &c);

    /**
     * Set diffuse light illumination.
     * @param  c  Diffuse light color/intensity
     */
    void set_diffuse(const Color4 &c);

    /**
     * Get the diffuse light illumination.
     * @return  Returns the diffuse color setting for this light.
     */
    const Color4 &get_diffuse() const;

    /**
     * Set specular light illumination.
     * @param  c  Specular light color/intensity
     */
    void set_specular(const Color4 &c);

    /**
     * Get the specular light illumination.
     * @return  Returns the specular color setting for this light.
     */
    const Color4 &get_specular() const;

    /**
     * Set the light position. Uses a homogeneous coordinate. If w = 0 the light is
     * directional. Position is set within the Draw method.
     * @param  pos Light position.
     */
    void set_position(const HPoint3 &pos);

    /**
     * Gets the light position
     * @return  Returns the light position.
     */
    const Point3 get_position() const;

    /**
     * Set spotlight parameters.
     * @param  dir     Spotlight direction vector.
     * @param  exp     Spotlight exponent.
     * @param  cutoff  Spotlight cutoff angle in degrees.
     */
    void set_spotlight(const Vector3 &dir, const float exp, const float cutoff);

    /**
     * Set the spotlight direction,
     * @param  dir  Set/update the spotlight direction vector.
     */
    void set_spotlight_direction(const Vector3 &dir);

    /**
     * Disable spotlight (turns back into point light source).
     */
    void turn_off_spotlight();

    /**
     * Query to determine if this is a spotlight.
     * @return  Returns true if this is a spotlight, false if not.
     */
    bool is_spotlight() const;

    /**
     * Get the spotlight direction.
     * @return  Returns the spotlight direction (vector)
     */
    const Vector3 &get_spotlight_direction() const;

    /**
     * Get the spotlight exponent.
     * @return  Returns the spotlight exponent.
     */
    float get_spotlight_exponent() const;

    /**
     * Get the cos of the spotlight cutoff angle.
     * @return  Returns the cos of the spotlight cutoff angle.
     */
    float get_spotlight_cutoff() const;

    /**
     * Set attenuation factors.
     * @param  constant  Constant factor.
     * @param  linear    Linear factor.
     * @param  quadratic Quadratic factor.
     */
    void set_attenuation(float constant, float linear, float quadratic);

    /**
     * Query if attenuation is enabled.
     * @return  Returns true if attenuation is set.
     */
    bool is_attenuation_enabled() const;

    /**
     * Get attenuation coefficient given a distance.
     * @param  d   Distance
     * @return  Returns attenuation factor (clamped to 0,1).
     */
    float get_attenuation(float d) const;

    /**
     * Draw. Sets the light properties if enabled. Note that only position
     * is set within the Draw method - since it needs to be transformed by
     * the current matrix.
     * @param  scene_state  Current scene state.
     */
    void draw(SceneState &scene_state) override;

  protected:
    bool     enabled_;
    bool     is_spotlight_;
    bool     attenuate_;
    uint32_t index_;
    Color4   ambient_;
    Color4   diffuse_;
    Color4   specular_;
    Vector3  spot_direction_;
    float    spot_cutoff_;
    float    spot_exponent_;
    float    const_atten_;
    float    lin_atten_;
    float    quad_atten_;

    // Light position as a homogeneous coordinate. If w = 0 the light is directional
    HPoint3 position_;
};

} // namespace cg

#endif
