#version 410 core

layout(std140) uniform block1
{
    //requires 4 vertices
    vec4 data[256*4];
    //[0] = (v0x, v0y, v1x, v1y)
    //[1] = (v2x, v2y, unused, unused)
    //[2] = color 1
    //[3] = color 2
};

flat out int idx;
out vec2 pos;

void main()
{
    idx = gl_InstanceID*4;

    float x0 = data[idx][0];
    float y0 = data[idx][1];
    float x1 = data[idx][2];
    float y1 = data[idx][3];
    float x2 = data[idx+1][0];
    float y2 = data[idx+1][1];

    if(gl_VertexID == 0) {
        gl_Position = vec4(x0, y0, 0.0, 1.0);
        pos = vec2(-1, -1);
    } else if(gl_VertexID == 1) {
        gl_Position = vec4(x1, y1, 0.0, 1.0);
        pos = vec2(1, -1);
    } else if(gl_VertexID == 2) {
        gl_Position = vec4(x2, y2, 0.0, 1.0);
        pos = vec2(-1, 1);
    } else {
        gl_Position = vec4(x1 + x2 - x0, y1 + y2 - y0, 0.0, 1.0);
        pos = vec2(1, 1);
    }
}
