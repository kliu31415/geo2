#version 410 core

layout (location = 0) in vec2 pos_in;
layout (location = 1) in vec2 tex_coord_in;

out vec2 tex_coord;

void main()
{
    gl_Position = vec4(pos_in, 0.0, 1.0);
    tex_coord = tex_coord_in;
}
