#version 410 core

//2 vertex line
layout(std140) uniform block1
{
    vec4 data[512*2];
    //[0] = (x, y, z, w)
    //[1] = color
};

flat out int idx;

void main()
{
    idx = gl_InstanceID * 2;

    if(gl_VertexID == 0)
        gl_Position = vec4(data[idx].xy, 0, 1);
    else
        gl_Position = vec4(data[idx].zw, 0, 1);
}
