#version 410 core

layout(std140) uniform block1
{
    //uses 3 vertices

    //the body should be a regular hexagon with v0 as the center
    //6 triangles in a fan shape should be sent in for the body
    //the wings should each be a triangles

    //for the body, subroutine # should be in [0, 5], and for the wings, [6, 7]

    vec4 data[170*6];
    //[0] = (v0, v1)
    //[1] = (v2, border cutoff, subroutine)
    //[2] = (num triangle levels, alternate (0 or 1), unused, unused)
    //[3] = border color
    //[4] = color 1
    //[5] = color 2
};

flat out int idx;

out vec3 bary_coord;

void main()
{
    idx = gl_InstanceID*6;

    if(gl_VertexID == 0) {
        gl_Position = vec4(data[idx].xy, 0, 1);
        bary_coord = vec3(1, 0, 0);
    } else if(gl_VertexID == 1) {
        gl_Position = vec4(data[idx].zw, 0, 1);
        bary_coord = vec3(0, 1, 0);
    } else {
        gl_Position = vec4(data[idx+1].xy, 0, 1);
        bary_coord = vec3(0, 0, 1);
    }
}
