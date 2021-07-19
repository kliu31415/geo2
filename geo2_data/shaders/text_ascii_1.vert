#version 410 core

//requires 4 vertices in a triangle strip

layout(std140) uniform characters
{
    //requires 4 vertices in a triangle strip
    vec4 data[341*3];
    //[0] = (x, y, w, h)
    //[1] = (font size, character (as int), char tex w% of max, char tex h% of max)
    //[2] = color
};

out vec2 tex_coord;

flat out int idx;

void main()
{
    idx = gl_InstanceID * 3;

    vec2 offset = vec2(gl_VertexID%2, gl_VertexID/2);
    gl_Position = vec4(data[idx].xy + data[idx].zw * offset, 0, 1);
    vec2 upper_left = vec2(0, 1);
    vec2 lower_right = vec2(data[idx+1].z, 1 - data[idx+1].w);
    tex_coord = mix(upper_left, lower_right, offset);
}
