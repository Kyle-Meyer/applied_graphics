#ifndef __RAYTRACER_SHADER_SRC_HPP__
#define __RAYTRACER_SHADER_SRC_HPP__

const char *tex_vert =
    R"(
#version 410 core
layout (location = 0) in vec2 vtx_position;
layout (location = 1) in vec2 vtx_texture;
layout (location = 0) smooth out vec2 tex_coord;
void main() 
{
    tex_coord = vtx_texture;
    gl_Position = vec4(vtx_position, 0.0, 1.0);
}
)";

const char *tex_frag =
    R"(
#version 410 core
layout (location = 0) smooth in vec2 tex_coord;
layout (location = 0) out vec4 frag_color;
uniform	sampler2D tex_image;
void main() 
{
    frag_color = texture(tex_image, tex_coord);
}
)";

#endif
