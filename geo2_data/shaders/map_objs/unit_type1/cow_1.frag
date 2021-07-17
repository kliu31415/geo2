#version 410 core

#define M_PI 3.14159265358979323846264

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

flat in int idx;

flat in vec4 rand[6];

in vec2 bary_coord;
in vec2 real_coord;

out vec4 frag_color;

void main()
{   
    //if(abs(bary_coord.x) > data[idx+1][2] || abs(bary_coord.y) > data[idx+1][3])
    if(greaterThan(abs(bary_coord), data[idx+1].zw) != bvec2(0, 0)) {
        frag_color = data[idx+7];
    } else {
        vec4 real_x = vec4(real_coord.x);
        vec4 real_y = vec4(real_coord.y);

        vec4 res = rand[4]*cos(15*rand[0] * (real_x + rand[1])) + rand[5]*cos(15*rand[2] * (real_y + rand[3]));

        if(res.x + res.y + res.z + res.w > 0)
            frag_color = data[idx+6];
        else
            frag_color = data[idx+5];
    }
}
