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

flat in int idx;

in vec3 bary_coord;

out vec4 frag_color;

void main()
{
    ivec3 bary_coord_xn = ivec3(trunc(3 * bary_coord));
    int color_select = (bary_coord_xn.x + bary_coord_xn.y + bary_coord_xn.z);
    int subroutineNum = int(data[idx+1][3]);

    if(subroutineNum < 6 && bary_coord[0] < data[idx+1][2]) {
        frag_color = data[idx+3];
    } else if(subroutineNum >= 6 && greaterThan(bary_coord, vec3(data[idx+1][2])) != bvec3(1)) {
        frag_color = data[idx+3];
    } else if((color_select + subroutineNum * int(data[idx+2][1])) % 2 == 1) {
        frag_color = data[idx+4];
    } else {
        frag_color = data[idx+5];
    }
}
