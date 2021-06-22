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
    //[7-8] = (x, y) coords
};

flat in int idx;
in vec2 pos;
in float relative_y;
in float y_border_cutoff;

out vec4 frag_color;

void main()
{   
    float border_thickness = data[idx+5][3];

    if(pos.x<border_thickness || pos.x>1-border_thickness || abs(relative_y) > y_border_cutoff)
        frag_color = data[idx+2];
    else {
        vec2 dist_vec = vec2(data[idx+6][0]*(pos.x - data[idx+5][0]), data[idx+6][1] * 0.5*(2*abs(0.5 - pos.y) - data[idx+5][1]));
        if(dot(dist_vec, dist_vec) < data[idx+5][2])
            frag_color = data[idx+3];
        else
            frag_color = vec4(data[idx+1].rgb * 0.2*(3 + 2*sin(data[idx+6][0]*pos.x)), data[idx+1].a);
    }
}
