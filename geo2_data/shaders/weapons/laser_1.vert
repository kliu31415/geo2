#version 410 core

layout(std140) uniform block1
{
    //requires 4 vertices
    vec4 data[256*4];
    //[0] = (v0x, v0y, dx01, dy01)
    //[1] = (dx02, dy02, unused, unused)
    //[2] = color 1
    //[3] = color 2
};

flat out int idx;
out vec3 bary_coord;

void main()
{
    idx = gl_InstanceID*4;

    float x0 = data[idx][0];
    float y0 = data[idx][1];
    float dx01 = data[idx][2];
    float dy01 = data[idx][3];
    float dx02 = data[idx+1][0];
    float dy02 = data[idx+1][1];

    if(gl_VertexID == 0) {
        gl_Position = vec4(x0, y0, 0.0, 1.0);
        bary_coord = vec3(1.0, 0.0, 0.0);
    } else if(gl_VertexID == 1) {
        gl_Position = vec4(x0 + dx01, y0 + dy01, 0.0, 1.0);
        bary_coord = vec3(0.0, 1.0, 0.0);
    } else if(gl_VertexID == 2) {
        gl_Position = vec4(x0 + dx02, y0 + dy02, 0.0, 1.0);
        bary_coord = vec3(0.0, 0.0, 1.0);
    } else {
        gl_Position = vec4(x0 + dx01 + dx02, y0 + dy01 + dy02, 0.0, 1.0);
        bary_coord = vec3(1.0, 0.0, 0.0);
    }
}
