#version 410 core

layout(std140) uniform block1
{
    vec4 data[128*2];
};

flat in int idx;

out vec4 frag_color;

void main()
{   
    frag_color = data[idx+1];
}
