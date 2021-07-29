#version 410 core

layout(std140) uniform block1
{
    //requires 4 vertices in a triangle strip
    //there should usually be 5 instances
    vec4 data[128*8];
    //[0] = (x0, y0, x1, y1)
    //[1] = (x2, y2, x_border_cutoff, y_border_cutoff)
    //[2] = (v0 bary x, v0 bary y, v3 bary x, v3 bary y)
    //[3] = (x0 real pos, y0 real pos, x3 real pos, y3 real pos)
    //[4] = (seed1, seed2, seed3, seed4) (all seeds should be in [0, 1])
    //[5] = inner color
    //[6] = spot color
    //[7] = border color
};

flat out int idx;

flat out vec4 rand[6];

out vec2 bary_coord;
out vec2 real_coord;

vec4 next_rand(vec4 x)
{
    vec4 res = mod(vec4(18.385592) * (vec4(1.3579) + cos(63.2356 * x + 5.732859)), vec4(1));
    return res;
}

void main()
{
    idx = gl_InstanceID*8;

    rand[0] = data[idx+4];
    for(int i=1; i<6; i++)
        rand[i] = next_rand(rand[i-1]);

    if(gl_VertexID == 0) {
        gl_Position = vec4(data[idx].xy, 0.0, 1.0);
        bary_coord = data[idx+2].xy;
        real_coord = data[idx+3].xy;
    } else if(gl_VertexID == 1) {
        gl_Position = vec4(data[idx].zw, 0.0, 1.0);
        bary_coord = data[idx+2].zy;
        real_coord = data[idx+3].zy;
    } else if(gl_VertexID == 2) {
        gl_Position = vec4(data[idx+1].xy, 0.0, 1.0);
        bary_coord = data[idx+2].xw;
        real_coord = data[idx+3].xw;
    } else {
        gl_Position = vec4(data[idx+1][0] + data[idx][2] - data[idx][0],
                           data[idx+1][1] + data[idx][3] - data[idx][1], 0.0, 1.0);
        bary_coord = data[idx+2].zw;
        real_coord = data[idx+3].zw;
    }
}
