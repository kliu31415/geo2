#version 410 core

layout(std140) uniform block1
{
    vec4 data[128*2];
};

flat out int idx;

void main()
{
    idx = gl_InstanceID*2;

    if(gl_VertexID == 0)
        gl_Position = vec4(data[idx][0], data[idx][1], 0.0, 1.0);
    else if(gl_VertexID == 1)
        gl_Position = vec4(data[idx][2], data[idx][1], 0.0, 1.0);
    else if(gl_VertexID == 2)
        gl_Position = vec4(data[idx][0], data[idx][3], 0.0, 1.0);
    else
        gl_Position = vec4(data[idx][2], data[idx][3], 0.0, 1.0);
}
