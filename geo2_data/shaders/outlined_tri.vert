#version 410 core

layout(std140) uniform block1
{
    vec4 data[204*5];
    //[0] = (pos0.x, pos0.y, pos1.x, pos1.y)
    //[1] = (pos2.x, pos2.y, unused, unused)
    //[2] = (thickness0, thickness1, thickness2, unused)
    //[3] = inner_color
    //[4] = border_color

    //vertex V opposes side V
    //A point is in the border if for any V, it is at least (1-thickness(V)) away from V in bary coords
    //If you want no border, set its thickness to something negative like -1.0f to prevent graphical artifacts.
};

flat out int idx;
out vec3 bary_coord;

void main()
{
    idx = gl_InstanceID*5;

    if(gl_VertexID == 0) {
        gl_Position = vec4(data[idx+0].xy, 0.0, 1.0);
        bary_coord = vec3(1.0, 0.0, 0.0);
    }
    else if(gl_VertexID == 1) {
        gl_Position = vec4(data[idx+0].zw, 0.0, 1.0);
        bary_coord = vec3(0.0, 1.0, 0.0);
    }
    else {
        gl_Position = vec4(data[idx+1].xy, 0.0, 1.0);
        bary_coord = vec3(0.0, 0.0, 1.0);
    }
}
