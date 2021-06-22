//requires 8 vertices

#version 410 core

layout(std140) uniform block1
{
    vec4 data[113*9];
    //[0] = (x, y, w, h)
    //[1] = interior color
    //[2] = border color
    //[3] = eye color
    //[4] = (rot matrix cos0, rot matrix sin0, rot center x, rot center y)
    //[5] = (eye center x, eye center y, eye radius squared, border thickness)
    //[6] = (x scale, y scale)
    //[7-8] = (x in [0, 1], y offset in [0, 1]) coords
};

flat out int idx;
out vec2 pos;
out float relative_y;
out float y_border_cutoff;

void main()
{
    idx = gl_InstanceID*9;

    float pos_x = data[idx + 7 + (gl_VertexID / 4)][(gl_VertexID & 2)];
    float pos_y = data[idx + 7 + (gl_VertexID / 4)][(gl_VertexID & 2) + 1];

    y_border_cutoff = 1 - (data[idx+6][0]/data[idx+6][1])*2*data[5][3] / pos_y;

    if(gl_VertexID % 2 == 0) {
        pos_y = 0.5 * (1 + pos_y);
        relative_y = 1;
    } else {
        pos_y = 0.5 * (1 - pos_y);
        relative_y = -1;
    }

    pos = vec2(pos_x, pos_y);

    float pos2_x = data[idx+4][0] * (pos_x - data[idx+4][2]) - data[idx+4][1] * (pos_y - data[idx+4][3]);
    float pos2_y = data[idx+4][1] * (pos_x - data[idx+4][2]) + data[idx+4][0] * (pos_y - data[idx+4][3]);

    pos2_x = data[idx][0] + pos2_x * data[idx][2];
    pos2_y = data[idx][1] + pos2_y * data[idx][3];

    gl_Position = vec4(pos2_x, pos2_y, 0.0, 1.0);
}
