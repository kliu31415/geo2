#version 410 core

//requires 4 vertices in a triangle strip

layout(std140) uniform characters
{
    //requires 4 vertices in a triangle strip
    vec4 data[341*3];
    //[0] = (x, y, w, h)
    //[1] = (font size, character (as int), character w %, character h %)
    //[2] = color
};

out vec2 tex_coord;

flat out int idx;

void main()
{
    idx = gl_InstanceID * 3;

    vec2 offset_mult = vec2(gl_VertexID%2, gl_VertexID/2);
    gl_Position = vec4(data[idx].xy + data[idx].zw * offset_mult, 0, 1);
    tex_coord = offset_mult * vec2(0.7, 0.7);
}
