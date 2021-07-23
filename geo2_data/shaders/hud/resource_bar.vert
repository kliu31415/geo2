#version 410 core

//4 vertices in a triangle strip

layout(std140) uniform block1
{
    vec4 data[204*5];
    //[0] = (v0, v3)
    //[1] = (% full, 1 - x border size, 1 - y border size, unused)
    //[2] = (full color)
    //[3] = (empty color)
    //[4] = (border color)
};


flat out int idx;

out vec2 normalized_coord;

void main()
{
    idx = gl_InstanceID*5;
    normalized_coord = vec2(gl_VertexID % 2, gl_VertexID / 2);
    gl_Position = vec4(mix(data[idx].xy, data[idx].zw, normalized_coord), 0, 1);
}
