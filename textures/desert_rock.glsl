// ============================================================
//  Desert Rock Procedural Material
//  Translated from Blender node graph
//
//  Uniforms map to the Group node inputs:
//    u_scale, u_color1, u_color2, u_stripes_color,
//    u_noise_scale, u_stripes_scale, u_texture_detail,
//    u_texture_detail2, u_roughness,
//    u_noise_bump_strength, u_stripes_bump_strength,
//    u_displacement_strength
// ============================================================

#version 330 core

// --- Outputs ------------------------------------------------
out vec4 FragColor;

// --- Varyings -----------------------------------------------
in vec3 v_position;     // object-space position
in vec3 v_normal;       // world-space normal
in vec2 v_uv;

// --- Uniforms (Group node inputs) ---------------------------
uniform float u_scale               = 1.0;
uniform vec3  u_color1              = vec3(0.832, 0.271, 0.068);
uniform vec3  u_color2              = vec3(0.075, 0.012, 0.007);
uniform vec3  u_stripes_color       = vec3(0.046, 0.008, 0.004);
uniform float u_noise_scale         = 16.0;
uniform float u_stripes_scale       = 13.0;
uniform float u_texture_detail      = 15.0;
uniform float u_texture_detail2     = 0.5;   // "Textre Detail 2" — clamped, feeds noise roughness
uniform float u_roughness           = 1.0;
uniform float u_noise_bump_strength = 0.1;
uniform float u_stripes_bump_strength = 0.3;
uniform float u_displacement_strength = 0.06;

// --- Lighting (simple) --------------------------------------
uniform vec3  u_light_dir   = normalize(vec3(0.5, 1.0, 0.8));
uniform vec3  u_view_pos    = vec3(0.0, 0.0, 5.0);

// ============================================================
//  Noise helpers
//  Blender's Noise Texture is based on Perlin/fBm.
//  We implement a reasonable fBm approximation.
// ============================================================

// Hash (from Dave_Hoskins / ShaderToy)
vec3 hash3(vec3 p) {
    p = fract(p * vec3(443.8975, 397.2973, 491.1871));
    p += dot(p, p.yxz + 19.19);
    return fract((p.xxy + p.yxx) * p.zyx);
}

float hash1(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

// Smooth noise
float vnoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(mix(hash1(i + vec3(0,0,0)), hash1(i + vec3(1,0,0)), u.x),
                   mix(hash1(i + vec3(0,1,0)), hash1(i + vec3(1,1,0)), u.x), u.y),
               mix(mix(hash1(i + vec3(0,0,1)), hash1(i + vec3(1,0,1)), u.x),
                   mix(hash1(i + vec3(0,1,1)), hash1(i + vec3(1,1,1)), u.x), u.y), u.z);
}

// fBm — mirrors Blender's Noise Texture (detail = octave count, lacunarity=2, offset=0, gain=1)
float fbm(vec3 p, float detail, float roughness, float lacunarity, float distortion) {
    // Distortion: warp p by a noise vector before sampling
    if (distortion > 0.0) {
        vec3 warp = vec3(
            vnoise(p + vec3(1.7, 9.2, 5.4)),
            vnoise(p + vec3(8.3, 2.8, 1.2)),
            vnoise(p + vec3(3.1, 6.5, 7.9))
        ) * 2.0 - 1.0;
        p += warp * distortion;
    }

    float value      = 0.0;
    float amplitude  = 1.0;
    float frequency  = 1.0;
    float max_val    = 0.0;
    int   octaves    = int(clamp(detail, 1.0, 16.0));

    for (int i = 0; i < octaves; i++) {
        value    += vnoise(p * frequency) * amplitude;
        max_val  += amplitude;
        amplitude *= roughness;
        frequency *= lacunarity;
    }
    // Fractional last octave blend
    float frac_octave = fract(detail);
    if (frac_octave > 0.0) {
        value   += vnoise(p * frequency) * amplitude * frac_octave;
        max_val += amplitude * frac_octave;
    }

    return value / max_val;  // normalised [0,1]
}

// ============================================================
//  ColorRamp helpers  (linear ramps approximated from defaults)
//  Blender's default ColorRamp is black→white unless changed.
//  Since the JSON doesn't export ramp stops, we use identity.
//  Swap these functions out once you know your actual stop values.
// ============================================================

// ColorRamp (main noise → roughness path): identity black→white
float colorRamp_main(float t) {
    return clamp(t, 0.0, 1.0);
}

// ColorRamp.001 (stripes noise → bump/mix factor): identity
float colorRamp_stripes(float t) {
    return clamp(t, 0.0, 1.0);
}

// ============================================================
//  RGB Curves  (Noise → ColorRamp)
//  Default RGB Curves in Blender = identity (diagonal line).
//  Replace with a custom curve if you modified it.
// ============================================================
float rgbCurves(float v) {
    return clamp(v, 0.0, 1.0);
}

// ============================================================
//  HSV Value scale  (Blender HueSaturationValue node)
//  hue=0.5 (no shift), sat=1.0 (no change), value = u_roughness
// ============================================================
float hsvValueScale(float v, float value_scale) {
    return clamp(v * value_scale, 0.0, 1.0);
}

// ============================================================
//  Bump normal approximation
//  We approximate bump by finite-difference of the height field.
// ============================================================
vec3 bumpNormal(vec3 N, vec3 p, float height, float strength, float eps) {
    // Finite differences in object space tangent directions
    vec3 tangent   = normalize(cross(N, vec3(0.0, 1.0, 0.0)));
    if (length(tangent) < 0.01) tangent = normalize(cross(N, vec3(1.0, 0.0, 0.0)));
    vec3 bitangent = normalize(cross(N, tangent));

    float h0 = height;
    // We can't re-evaluate the full scene here, so we use a pre-baked
    // bump via screen-space derivative trick — or just perturb N directly.
    // For a true implementation, pass dFdx/dFdy of the height field.
    vec3 perturbed = N + strength * (tangent * dFdx(height) + bitangent * dFdy(height)) * 10.0;
    return normalize(mix(N, normalize(perturbed), clamp(strength, 0.0, 1.0)));
}

// ============================================================
//  Main
// ============================================================
void main() {

    // -----------------------------------------------------------
    // 1. Texture Coordinate (Object space) → Mapping (apply scale)
    //    Mapping node: Scale = vec3(u_scale)  [uniform scale]
    // -----------------------------------------------------------
    vec3 obj_coord = v_position * u_scale;

    // -----------------------------------------------------------
    // 2. Mapping.001: Scale Z by 2  →  feed Noise Texture (main)
    //    Mapping.002: Scale Z by 10 →  feed Noise Texture.001 (stripes)
    // -----------------------------------------------------------
    vec3 coord_main    = obj_coord * vec3(1.0, 1.0, 2.0);
    vec3 coord_stripes = obj_coord * vec3(1.0, 1.0, 10.0);

    // -----------------------------------------------------------
    // 3. Noise Texture (main)
    //    Scale=u_noise_scale, Detail=u_texture_detail,
    //    Roughness=clamp(u_texture_detail2,0,1), Lacunarity=2,
    //    Distortion=0
    // -----------------------------------------------------------
    float noise_roughness = clamp(u_texture_detail2, 0.0, 1.0);  // Clamp node
    float noise_main = fbm(
        coord_main * u_noise_scale,
        u_texture_detail,
        noise_roughness,
        2.0,
        0.0
    );

    // -----------------------------------------------------------
    // 4. Noise Texture.001 (stripes)
    //    Scale=u_stripes_scale, Detail=u_texture_detail,
    //    Roughness=0.8, Lacunarity=2, Distortion=0.4
    // -----------------------------------------------------------
    float noise_stripes = fbm(
        coord_stripes * u_stripes_scale,
        u_texture_detail,
        0.8,
        2.0,
        0.4
    );

    // -----------------------------------------------------------
    // 5. RGB Curves on main noise → Reroute.001 → ColorRamp
    // -----------------------------------------------------------
    float curved        = rgbCurves(noise_main);
    float ramp_main     = colorRamp_main(curved);     // → ColorRamp
    float ramp_stripes  = colorRamp_stripes(noise_stripes); // → ColorRamp.001

    // -----------------------------------------------------------
    // 6. Color mixing
    //    Mix:     lerp(Color1, Color2, noise_main)  → result_AB
    //    Mix.001: lerp(StripeColor, result_AB, ramp_stripes) → base_color
    // -----------------------------------------------------------
    vec3 result_AB   = mix(u_color1, u_color2, noise_main);
    vec3 base_color  = mix(u_stripes_color, result_AB, ramp_stripes);

    // -----------------------------------------------------------
    // 7. Roughness via HSV Value node
    //    Input: ColorRamp (ramp_main), Value scale = u_roughness
    // -----------------------------------------------------------
    float mat_roughness = hsvValueScale(ramp_main, u_roughness);

    // -----------------------------------------------------------
    // 8. Bump normals (chained)
    //    Bump:     height = ramp_main,    strength = u_noise_bump_strength
    //    Bump.001: height = ramp_stripes, strength = u_stripes_bump_strength
    //              normal input = output of Bump
    // -----------------------------------------------------------
    vec3 N = normalize(v_normal);
    N = bumpNormal(N, v_position, ramp_main,    u_noise_bump_strength,   0.005);
    N = bumpNormal(N, v_position, ramp_stripes, u_stripes_bump_strength, 0.005);

    // -----------------------------------------------------------
    // 9. Displacement (output only — applied on CPU/tessellation side)
    //    height = ramp_main, midlevel=0.7, scale=u_displacement_strength
    //    Shown here as a visualisation hint only.
    // -----------------------------------------------------------
    // float displacement = (ramp_main - 0.7) * u_displacement_strength;

    // -----------------------------------------------------------
    // 10. Simple PBR lighting (Principled BSDF approximation)
    //     Metallic=0, IOR=1.45 → specular ~F0=0.04
    // -----------------------------------------------------------
    vec3  light_dir  = normalize(u_light_dir);
    vec3  view_dir   = normalize(u_view_pos - v_position);
    vec3  half_vec   = normalize(light_dir + view_dir);

    float NdotL = max(dot(N, light_dir), 0.0);
    float NdotH = max(dot(N, half_vec),  0.0);
    float NdotV = max(dot(N, view_dir),  0.0);

    // GGX specular
    float alpha  = mat_roughness * mat_roughness;
    float alpha2 = alpha * alpha;
    float denom  = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
    float D      = alpha2 / (3.14159265 * denom * denom);

    float k      = (mat_roughness + 1.0);
    k = (k * k) / 8.0;
    float G_L    = NdotL / (NdotL * (1.0 - k) + k);
    float G_V    = NdotV / (NdotV * (1.0 - k) + k);
    float G      = G_L * G_V;

    float F0     = 0.04;  // non-metallic
    float F      = F0 + (1.0 - F0) * pow(1.0 - max(dot(half_vec, view_dir), 0.0), 5.0);

    float specular = (D * G * F) / max(4.0 * NdotL * NdotV, 0.001);

    vec3 diffuse   = base_color / 3.14159265;
    vec3 radiance  = vec3(1.0);  // white light
    vec3 color     = (diffuse + specular) * radiance * NdotL;

    // Ambient
    color += base_color * 0.03;

    // Gamma correction
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
