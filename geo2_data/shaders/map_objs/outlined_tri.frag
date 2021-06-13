#version 410 core

layout(std140) uniform block1
{
    vec4 data[128*5];
};

flat in int idx;
in vec3 bary_coord;

out vec4 frag_color;

void main()
{   
    if(bary_coord[0] < data[idx+2][0] || bary_coord[1] < data[idx+2][1] || bary_coord[2] < data[idx+2][2])
        frag_color = data[idx + 4];
    else
        frag_color = data[idx + 3]; 
}
