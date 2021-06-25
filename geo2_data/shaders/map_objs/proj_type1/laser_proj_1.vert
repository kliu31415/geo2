#version 410 core

layout(std140) uniform block1
{
    //requires 3 vertices
    vec4 data[256*4];
    //[0] = (v0x, v0y, v1x, v1y)
    //[1] = (v2x, v2y, unused, unused)
    //[2] = inner color
    //[3] = outer color
};

flat out int idx;
out vec3 bary_coord;

void main()
{
    idx = gl_InstanceID*4;

    if(gl_VertexID == 0) {
        gl_Position = vec4(data[idx+0][0], data[idx+0][1], 0.0, 1.0);
        bary_coord = vec3(1.0, 0.0, 0.0);
    }
    else if(gl_VertexID == 1) {
        gl_Position = vec4(data[idx+0][2], data[idx+0][3], 0.0, 1.0);
        bary_coord = vec3(0.0, 1.0, 0.0);
    }
    else {
        gl_Position = vec4(data[idx+1][0], data[idx+1][1], 0.0, 1.0);
        bary_coord = vec3(0.0, 0.0, 1.0);
    }
}
